// Copyright (c) 2026 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla/Luma/LumaViewHandle.h"
#include "Carla.h"
#include "Carla/Game/CarlaEngine.h"

#include <util/ue-header-guard-begin.h>
#include "Carla/Sensor/SceneCaptureCamera.h"
#include "LumaScene.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"
#include <util/ue-header-guard-end.h>

namespace
{
  static const FName NAME_LumaViewEnable(TEXT("LumaViewEnable"));
  static const FName NAME_LumaViewOrigin(TEXT("LumaViewOrigin"));
  static const FName NAME_LumaViewRotation(TEXT("LumaViewRotation"));
  static const FName NAME_LumaViewFOV(TEXT("LumaViewFOV"));
  static const FName NAME_LumaViewWidth(TEXT("LumaViewWidth"));
  static const FName NAME_LumaViewHeight(TEXT("LumaViewHeight"));
  static const FName NAME_LumaWorldToClipRow0(TEXT("LumaWorldToClipRow0"));
  static const FName NAME_LumaWorldToClipRow1(TEXT("LumaWorldToClipRow1"));
  static const FName NAME_LumaWorldToClipRow2(TEXT("LumaWorldToClipRow2"));
  static const FName NAME_LumaWorldToClipRow3(TEXT("LumaWorldToClipRow3"));
  static const FName NAME_LumaClipToWorldRow0(TEXT("LumaClipToWorldRow0"));
  static const FName NAME_LumaClipToWorldRow1(TEXT("LumaClipToWorldRow1"));
  static const FName NAME_LumaClipToWorldRow2(TEXT("LumaClipToWorldRow2"));
  static const FName NAME_LumaClipToWorldRow3(TEXT("LumaClipToWorldRow3"));
  static const FName NAME_LumaWorldToViewRow0(TEXT("LumaWorldToViewRow0"));
  static const FName NAME_LumaWorldToViewRow1(TEXT("LumaWorldToViewRow1"));
  static const FName NAME_LumaWorldToViewRow2(TEXT("LumaWorldToViewRow2"));
  static const FName NAME_LumaWorldToViewRow3(TEXT("LumaWorldToViewRow3"));
  static const FName NAME_LumaViewToWorldRow0(TEXT("LumaViewToWorldRow0"));
  static const FName NAME_LumaViewToWorldRow1(TEXT("LumaViewToWorldRow1"));
  static const FName NAME_LumaViewToWorldRow2(TEXT("LumaViewToWorldRow2"));
  static const FName NAME_LumaViewToWorldRow3(TEXT("LumaViewToWorldRow3"));

  // Backward-compatibility with existing marketplace Niagara graphs that still
  // branch on ViewOverride* parameters.
  static const FName NAME_ViewOverrideEnabled(TEXT("ViewOverrideEnabled"));
  static const FName NAME_ViewOverrideEnable(TEXT("ViewOverrideEnable"));
  static const FName NAME_ViewOverrideOrigin(TEXT("ViewOverrideOrigin"));
  static const FName NAME_ViewOverrideRotation(TEXT("ViewOverrideRotation"));

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
} // namespace

void ULumaViewHandle::Initialize(ASceneCaptureCamera *InSensor, const FGuid &InViewId)
{
  Sensor = InSensor;
  ViewId = InViewId;
}

void ULumaViewHandle::BindScene(ALumaScene *InScene, bool bInOwnsScene)
{
  Scene = InScene;
  bOwnsScene = bInOwnsScene;
}

void ULumaViewHandle::SetEnabled(bool bInEnabled)
{
  bEnabled = bInEnabled;
}

void ULumaViewHandle::ApplyViewState(const FLumaViewState &ViewState)
{
  ALumaScene *LumaScene = Scene.Get();
  if (!LumaScene)
  {
    return;
  }

  const bool bViewEnabled = bEnabled && ViewState.bEnable;
  const FVector RotationEuler = ViewState.Rotation.Euler();
  const uint64 Frame = FCarlaEngine::GetFrameCounter();

  for (UNiagaraComponent *NiagaraComponent : LumaScene->LumaNiagaraComponents)
  {
    if (!NiagaraComponent)
    {
      continue;
    }

    NiagaraComponent->SetVariableBool(NAME_LumaViewEnable, bViewEnabled);
    NiagaraComponent->SetVariableVec3(NAME_LumaViewOrigin, ViewState.Origin);
    NiagaraComponent->SetVariableVec3(NAME_LumaViewRotation, RotationEuler);
    NiagaraComponent->SetVariableFloat(NAME_LumaViewFOV, ViewState.FOV);
    NiagaraComponent->SetVariableInt(NAME_LumaViewWidth, ViewState.Width);
    NiagaraComponent->SetVariableInt(NAME_LumaViewHeight, ViewState.Height);
    NiagaraComponent->SetVariableVec4(NAME_LumaWorldToClipRow0, ViewState.LumaWorldToClipRow0);
    NiagaraComponent->SetVariableVec4(NAME_LumaWorldToClipRow1, ViewState.LumaWorldToClipRow1);
    NiagaraComponent->SetVariableVec4(NAME_LumaWorldToClipRow2, ViewState.LumaWorldToClipRow2);
    NiagaraComponent->SetVariableVec4(NAME_LumaWorldToClipRow3, ViewState.LumaWorldToClipRow3);
    NiagaraComponent->SetVariableVec4(NAME_LumaClipToWorldRow0, ViewState.LumaClipToWorldRow0);
    NiagaraComponent->SetVariableVec4(NAME_LumaClipToWorldRow1, ViewState.LumaClipToWorldRow1);
    NiagaraComponent->SetVariableVec4(NAME_LumaClipToWorldRow2, ViewState.LumaClipToWorldRow2);
    NiagaraComponent->SetVariableVec4(NAME_LumaClipToWorldRow3, ViewState.LumaClipToWorldRow3);
    NiagaraComponent->SetVariableVec4(NAME_LumaWorldToViewRow0, ViewState.LumaWorldToViewRow0);
    NiagaraComponent->SetVariableVec4(NAME_LumaWorldToViewRow1, ViewState.LumaWorldToViewRow1);
    NiagaraComponent->SetVariableVec4(NAME_LumaWorldToViewRow2, ViewState.LumaWorldToViewRow2);
    NiagaraComponent->SetVariableVec4(NAME_LumaWorldToViewRow3, ViewState.LumaWorldToViewRow3);
    NiagaraComponent->SetVariableVec4(NAME_LumaViewToWorldRow0, ViewState.LumaViewToWorldRow0);
    NiagaraComponent->SetVariableVec4(NAME_LumaViewToWorldRow1, ViewState.LumaViewToWorldRow1);
    NiagaraComponent->SetVariableVec4(NAME_LumaViewToWorldRow2, ViewState.LumaViewToWorldRow2);
    NiagaraComponent->SetVariableVec4(NAME_LumaViewToWorldRow3, ViewState.LumaViewToWorldRow3);
    NiagaraComponent->SetVariableBool(NAME_ViewOverrideEnabled, bViewEnabled);
    NiagaraComponent->SetVariableBool(NAME_ViewOverrideEnable, bViewEnabled);
    NiagaraComponent->SetVariableVec3(NAME_ViewOverrideOrigin, ViewState.Origin);
    NiagaraComponent->SetVariableVec3(NAME_ViewOverrideRotation, RotationEuler);

    if (!HasUserParameter(NiagaraComponent, NAME_LumaViewEnable))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaViewEnable);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaViewOrigin))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaViewOrigin);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaViewRotation))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaViewRotation);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaViewFOV))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaViewFOV);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaViewWidth))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaViewWidth);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaViewHeight))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaViewHeight);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaWorldToClipRow0))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaWorldToClipRow0);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaWorldToClipRow1))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaWorldToClipRow1);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaWorldToClipRow2))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaWorldToClipRow2);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaWorldToClipRow3))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaWorldToClipRow3);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaClipToWorldRow0))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaClipToWorldRow0);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaClipToWorldRow1))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaClipToWorldRow1);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaClipToWorldRow2))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaClipToWorldRow2);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaClipToWorldRow3))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaClipToWorldRow3);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaWorldToViewRow0))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaWorldToViewRow0);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaWorldToViewRow1))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaWorldToViewRow1);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaWorldToViewRow2))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaWorldToViewRow2);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaWorldToViewRow3))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaWorldToViewRow3);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaViewToWorldRow0))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaViewToWorldRow0);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaViewToWorldRow1))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaViewToWorldRow1);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaViewToWorldRow2))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaViewToWorldRow2);
    if (!HasUserParameter(NiagaraComponent, NAME_LumaViewToWorldRow3))
      WarnMissingParameterOnce(NiagaraComponent, NAME_LumaViewToWorldRow3);

    if (ShouldLogLumaDebug(Sensor.IsValid() ? Sensor->GetName() : TEXT("null"), Frame))
    {
      const bool bHasNewEnable = HasUserParameter(NiagaraComponent, NAME_LumaViewEnable);
      const bool bHasOldEnable = HasUserParameter(NiagaraComponent, NAME_ViewOverrideEnabled) || HasUserParameter(NiagaraComponent, NAME_ViewOverrideEnable);
      const bool bHasOldOrigin = HasUserParameter(NiagaraComponent, NAME_ViewOverrideOrigin);
      const bool bHasOldRotation = HasUserParameter(NiagaraComponent, NAME_ViewOverrideRotation);
      const UNiagaraSystem *SystemAsset = NiagaraComponent->GetAsset();
      UE_LOG(
          LogCarla,
          Warning,
          TEXT("LumaDebug[ApplyViewState] sensor='%s' frame=%llu scene='%s' niagara='%s' asset='%s' enabled=%s has_new_enable=%s has_old_enable=%s has_old_origin=%s has_old_rot=%s origin=(%.2f,%.2f,%.2f) fov=%.2f size=%dx%d"),
          Sensor.IsValid() ? *Sensor->GetName() : TEXT("null"),
          Frame,
          *LumaScene->GetName(),
          *NiagaraComponent->GetName(),
          SystemAsset ? *SystemAsset->GetPathName() : TEXT("null"),
          bViewEnabled ? TEXT("true") : TEXT("false"),
          bHasNewEnable ? TEXT("true") : TEXT("false"),
          bHasOldEnable ? TEXT("true") : TEXT("false"),
          bHasOldOrigin ? TEXT("true") : TEXT("false"),
          bHasOldRotation ? TEXT("true") : TEXT("false"),
          ViewState.Origin.X,
          ViewState.Origin.Y,
          ViewState.Origin.Z,
          ViewState.FOV,
          ViewState.Width,
          ViewState.Height);
    }
  }
}

void ULumaViewHandle::DestroyOwnedScene()
{
  ALumaScene *OwnedScene = Scene.Get();
  if (bOwnsScene && OwnedScene && !OwnedScene->IsActorBeingDestroyed())
  {
    OwnedScene->Destroy();
  }
  Scene = nullptr;
  bOwnsScene = false;
}

bool ULumaViewHandle::HasUserParameter(const UNiagaraComponent *NiagaraComponent, const FName &ParameterName) const
{
  if (!NiagaraComponent)
  {
    return false;
  }

  const UNiagaraSystem *System = NiagaraComponent->GetAsset();
  if (!System)
  {
    return false;
  }

  TArray<FNiagaraVariable> UserParameters;
  System->GetExposedParameters().GetUserParameters(UserParameters);

  const FString RawName = ParameterName.ToString();
  const FString UserName = FString::Printf(TEXT("User.%s"), *RawName);
  for (const FNiagaraVariable &Parameter : UserParameters)
  {
    const FString Candidate = Parameter.GetName().ToString();
    if (Candidate.Equals(RawName, ESearchCase::IgnoreCase) || Candidate.Equals(UserName, ESearchCase::IgnoreCase))
    {
      return true;
    }
  }
  return false;
}

void ULumaViewHandle::WarnMissingParameterOnce(const UNiagaraComponent *NiagaraComponent, const FName &ParameterName)
{
  if (!NiagaraComponent)
  {
    return;
  }

  const FString Key = NiagaraComponent->GetPathName() + TEXT("|") + ParameterName.ToString();
  if (MissingParameterWarnings.Contains(Key))
  {
    return;
  }

  MissingParameterWarnings.Add(Key);
  UE_LOG(
      LogCarla,
      Warning,
      TEXT("Luma Niagara missing user parameter '%s' on component '%s' (sensor='%s', view_id=%s)."),
      *ParameterName.ToString(),
      *NiagaraComponent->GetPathName(),
      Sensor.IsValid() ? *Sensor->GetName() : TEXT("null"),
      *ViewId.ToString(EGuidFormats::DigitsWithHyphensLower));
}
