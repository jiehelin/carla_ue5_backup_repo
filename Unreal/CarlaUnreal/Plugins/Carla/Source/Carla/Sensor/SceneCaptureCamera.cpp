// Copyright (c) 2026 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla/Sensor/SceneCaptureCamera.h"
#include "Carla.h"
#include "Carla/Game/CarlaEngine.h"
#include "Carla/Luma/LumaMultiViewSubsystem.h"
#include "Carla/Luma/LumaViewHandle.h"
#include <chrono>

#include <util/ue-header-guard-begin.h>
#include "Actor/ActorBlueprintFunctionLibrary.h"
#include "Camera/CameraTypes.h"
#include "Kismet/GameplayStatics.h"
#include "RenderingThread.h"
#include <util/ue-header-guard-end.h>

namespace
{
  constexpr uint64 LumaDebugLogIntervalFrames = 60u;

  bool ShouldLogLumaDebug(const FString &SensorName, const uint64 Frame)
  {
    static TMap<FString, uint64> LastLogFrameBySensor;
    uint64 &LastFrame = LastLogFrameBySensor.FindOrAdd(SensorName);
    if (Frame < LastFrame + LumaDebugLogIntervalFrames)
    {
      return false;
    }
    LastFrame = Frame;
    return true;
  }
}

FActorDefinition ASceneCaptureCamera::GetSensorDefinition()
{
    constexpr bool bEnableModifyingPostProcessEffects = true;
    return UActorBlueprintFunctionLibrary::MakeCameraDefinition(
        TEXT("rgb"),
        bEnableModifyingPostProcessEffects);
}

ASceneCaptureCamera::ASceneCaptureCamera(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AddPostProcessingMaterial(
        TEXT("Material'/Carla/PostProcessingMaterials/PhysicLensDistortion.PhysicLensDistortion'"));
}

void ASceneCaptureCamera::BeginPlay()
{
  Super::BeginPlay();

  if (UWorld *World = GetWorld())
  {
    if (ULumaMultiViewSubsystem *LumaSubsystem = World->GetSubsystem<ULumaMultiViewSubsystem>())
    {
      LumaViewHandle = LumaSubsystem->AcquireHandle(this);
    }
  }
}

void ASceneCaptureCamera::OnFirstClientConnected()
{
}

void ASceneCaptureCamera::OnLastClientDisconnected()
{
}

void ASceneCaptureCamera::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  if (UWorld *World = GetWorld())
  {
    if (ULumaMultiViewSubsystem *LumaSubsystem = World->GetSubsystem<ULumaMultiViewSubsystem>())
    {
      LumaSubsystem->ReleaseHandle(this);
    }
  }
  LumaViewHandle = nullptr;

  Super::EndPlay(EndPlayReason);
}

void ASceneCaptureCamera::PostPhysTick(UWorld *World, ELevelTick TickType, float DeltaSeconds)
{
  TRACE_CPUPROFILER_EVENT_SCOPE(ASceneCaptureCamera::PostPhysTick);

  if (World)
  {
    ULumaMultiViewSubsystem *LumaSubsystem = World->GetSubsystem<ULumaMultiViewSubsystem>();
    if (LumaSubsystem)
    {
      if (!LumaViewHandle)
      {
        LumaViewHandle = LumaSubsystem->AcquireHandle(this);
      }

      FLumaViewState ViewState;
      ViewState.bEnable = true;
      if (const USceneCaptureComponent2D_CARLA *CaptureComponent = GetCaptureComponent())
      {
        ViewState.Origin = CaptureComponent->GetComponentLocation();
        ViewState.Rotation = CaptureComponent->GetComponentRotation();
        ViewState.FOV = CaptureComponent->FOVAngle;

        FMinimalViewInfo MinimalViewInfo;
        const_cast<USceneCaptureComponent2D_CARLA *>(CaptureComponent)->GetCameraView(DeltaSeconds, MinimalViewInfo);

        TOptional<FMatrix> CustomProjectionMatrix;
        if (CaptureComponent->bUseCustomProjectionMatrix)
        {
          CustomProjectionMatrix = CaptureComponent->CustomProjectionMatrix;
        }

        FMatrix ViewMatrix;
        FMatrix ProjectionMatrix;
        FMatrix ViewProjectionMatrix;
        UGameplayStatics::CalculateViewProjectionMatricesFromMinimalView(
            MinimalViewInfo,
            CustomProjectionMatrix,
            ViewMatrix,
            ProjectionMatrix,
            ViewProjectionMatrix);

        const FMatrix WorldToClip = ViewProjectionMatrix;
        const FMatrix ClipToWorld = ViewProjectionMatrix.Inverse();
        const FMatrix WorldToView = ViewMatrix;
        const FMatrix ViewToWorld = ViewMatrix.Inverse();

        // Niagara HLSL side builds float4x4(...) and uses mul(M, v), which is
        // effectively column-vector convention. Pack matrix columns so shader
        // doesn't need per-call transpose and view-space math stays consistent.
        auto MatrixColumnToVector4 = [](const FMatrix &Matrix, int32 ColumnIndex) -> FVector4
        {
          return FVector4(
              static_cast<float>(Matrix.M[0][ColumnIndex]),
              static_cast<float>(Matrix.M[1][ColumnIndex]),
              static_cast<float>(Matrix.M[2][ColumnIndex]),
              static_cast<float>(Matrix.M[3][ColumnIndex]));
        };

        ViewState.LumaWorldToClipRow0 = MatrixColumnToVector4(WorldToClip, 0);
        ViewState.LumaWorldToClipRow1 = MatrixColumnToVector4(WorldToClip, 1);
        ViewState.LumaWorldToClipRow2 = MatrixColumnToVector4(WorldToClip, 2);
        ViewState.LumaWorldToClipRow3 = MatrixColumnToVector4(WorldToClip, 3);
        ViewState.LumaClipToWorldRow0 = MatrixColumnToVector4(ClipToWorld, 0);
        ViewState.LumaClipToWorldRow1 = MatrixColumnToVector4(ClipToWorld, 1);
        ViewState.LumaClipToWorldRow2 = MatrixColumnToVector4(ClipToWorld, 2);
        ViewState.LumaClipToWorldRow3 = MatrixColumnToVector4(ClipToWorld, 3);
        ViewState.LumaWorldToViewRow0 = MatrixColumnToVector4(WorldToView, 0);
        ViewState.LumaWorldToViewRow1 = MatrixColumnToVector4(WorldToView, 1);
        ViewState.LumaWorldToViewRow2 = MatrixColumnToVector4(WorldToView, 2);
        ViewState.LumaWorldToViewRow3 = MatrixColumnToVector4(WorldToView, 3);
        ViewState.LumaViewToWorldRow0 = MatrixColumnToVector4(ViewToWorld, 0);
        ViewState.LumaViewToWorldRow1 = MatrixColumnToVector4(ViewToWorld, 1);
        ViewState.LumaViewToWorldRow2 = MatrixColumnToVector4(ViewToWorld, 2);
        ViewState.LumaViewToWorldRow3 = MatrixColumnToVector4(ViewToWorld, 3);
      }
      else
      {
        ViewState.Origin = GetActorLocation();
        ViewState.Rotation = GetActorRotation();
        ViewState.FOV = GetFOVAngle();
      }
      ViewState.Width = static_cast<int32>(GetImageWidth());
      ViewState.Height = static_cast<int32>(GetImageHeight());

      const bool bUpdateOk = LumaSubsystem->UpdateViewState(this, ViewState);
      const uint64 Frame = FCarlaEngine::GetFrameCounter();
      if (ShouldLogLumaDebug(GetName(), Frame))
      {
        UE_LOG(
            LogCarla,
            Warning,
            TEXT("LumaDebug[CameraPostPhysTick] sensor='%s' frame=%llu update_ok=%s loc=(%.2f,%.2f,%.2f) rot=(%.2f,%.2f,%.2f) fov=%.2f size=%dx%d w2c_row0=(%.5f,%.5f,%.5f,%.5f)"),
            *GetName(),
            Frame,
            bUpdateOk ? TEXT("true") : TEXT("false"),
            ViewState.Origin.X,
            ViewState.Origin.Y,
            ViewState.Origin.Z,
            ViewState.Rotation.Pitch,
            ViewState.Rotation.Yaw,
            ViewState.Rotation.Roll,
            ViewState.FOV,
            ViewState.Width,
            ViewState.Height,
            ViewState.LumaWorldToClipRow0.X,
            ViewState.LumaWorldToClipRow0.Y,
            ViewState.LumaWorldToClipRow0.Z,
            ViewState.LumaWorldToClipRow0.W);
      }
    }
  }

  Super::PostPhysTick(World, TickType, DeltaSeconds);
  
  ENQUEUE_RENDER_COMMAND(MeasureTime)
  (
    [](auto &InRHICmdList)
    {
      std::chrono::time_point<std::chrono::high_resolution_clock> Time = 
          std::chrono::high_resolution_clock::now();
      auto Duration = std::chrono::duration_cast< std::chrono::milliseconds >(Time.time_since_epoch());
      uint64_t Milliseconds = Duration.count();
      FString ProfilerText = FString("(Render)Frame: ") + FString::FromInt(FCarlaEngine::GetFrameCounter()) + 
          FString(" Time: ") + FString::FromInt(Milliseconds);
      TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*ProfilerText);
    }
  );

  if (!AreClientsListening())
      return;

  auto FrameIndex = FCarlaEngine::GetFrameCounter();
  ImageUtil::ReadSensorImageDataAsyncFColor(*this, [this, FrameIndex](
    TArrayView<const FColor> Pixels,
    FIntPoint Size) -> bool
  {
    SendDataToClient(*this, Pixels, FrameIndex);
    return true;
  });
}

#ifdef CARLA_HAS_GBUFFER_API
void ASceneCaptureCamera::SendGBufferTextures(FGBufferRequest& GBuffer)
{
    SendGBufferTexturesInternal(*this, GBuffer);
}
#endif
