// Copyright (c) 2026 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla/Luma/LumaMultiViewSubsystem.h"
#include "Carla.h"
#include "Carla/Luma/LumaViewHandle.h"

#include <util/ue-header-guard-begin.h>
#include "Carla/Sensor/SceneCaptureCamera.h"
#include "EngineUtils.h"
#include "LumaScene.h"
#include <util/ue-header-guard-end.h>

const FName ULumaMultiViewSubsystem::CloneTagName(TEXT("CarlaLumaClone"));

void ULumaMultiViewSubsystem::Deinitialize()
{
  for (TPair<TWeakObjectPtr<ASceneCaptureCamera>, TObjectPtr<ULumaViewHandle>> &Pair : ViewHandles)
  {
    if (Pair.Value)
    {
      Pair.Value->DestroyOwnedScene();
    }
  }

  ViewHandles.Empty();
  ViewStates.Empty();
  CachedTemplateScene = nullptr;
}

ULumaViewHandle *ULumaMultiViewSubsystem::AcquireHandle(ASceneCaptureCamera *Sensor)
{
  if (!Sensor)
  {
    return nullptr;
  }

  const TWeakObjectPtr<ASceneCaptureCamera> SensorKey(Sensor);

  if (TObjectPtr<ULumaViewHandle> *ExistingHandle = ViewHandles.Find(SensorKey))
  {
    return ExistingHandle->Get();
  }

  ULumaViewHandle *Handle = NewObject<ULumaViewHandle>(this);
  check(Handle != nullptr);

  const FGuid ViewId = FGuid::NewGuid();
  Handle->Initialize(Sensor, ViewId);

  ALumaScene *TemplateScene = FindTemplateScene();
  if (!TemplateScene)
  {
    UE_LOG(
        LogCarla,
        Warning,
        TEXT("LumaMultiViewSubsystem: no ALumaScene template found. Sensor '%s' will run without Luma scene binding."),
        *Sensor->GetName());
  }
  else
  {
    ALumaScene *ClonedScene = SpawnSceneCloneFromTemplate(TemplateScene, Sensor);
    if (ClonedScene)
    {
      Handle->BindScene(ClonedScene, true);
    }
  }

  ViewHandles.Add(SensorKey, Handle);
  ViewStates.Add(ViewId, FLumaViewState{});
  return Handle;
}

void ULumaMultiViewSubsystem::ReleaseHandle(ASceneCaptureCamera *Sensor)
{
  if (!Sensor)
  {
    return;
  }

  const TWeakObjectPtr<ASceneCaptureCamera> SensorKey(Sensor);
  TObjectPtr<ULumaViewHandle> Handle;
  if (!ViewHandles.RemoveAndCopyValue(SensorKey, Handle) || !Handle)
  {
    return;
  }

  ViewStates.Remove(Handle->GetViewId());
  Handle->DestroyOwnedScene();
}

bool ULumaMultiViewSubsystem::UpdateViewState(ASceneCaptureCamera *Sensor, const FLumaViewState &ViewState)
{
  if (!Sensor)
  {
    return false;
  }

  const TWeakObjectPtr<ASceneCaptureCamera> SensorKey(Sensor);
  TObjectPtr<ULumaViewHandle> *HandlePtr = ViewHandles.Find(SensorKey);
  if (!HandlePtr || !HandlePtr->Get())
  {
    return false;
  }

  ULumaViewHandle *Handle = HandlePtr->Get();
  ViewStates.Add(Handle->GetViewId(), ViewState);
  Handle->ApplyViewState(ViewState);
  return true;
}

ALumaScene *ULumaMultiViewSubsystem::FindTemplateScene()
{
  if (CachedTemplateScene.IsValid())
  {
    return CachedTemplateScene.Get();
  }

  UWorld *World = GetWorld();
  if (!World)
  {
    return nullptr;
  }

  for (TActorIterator<ALumaScene> It(World); It; ++It)
  {
    ALumaScene *Scene = *It;
    if (!Scene || Scene->ActorHasTag(CloneTagName))
    {
      continue;
    }

    CachedTemplateScene = Scene;
    return Scene;
  }

  return nullptr;
}

ALumaScene *ULumaMultiViewSubsystem::SpawnSceneCloneFromTemplate(ALumaScene *TemplateScene, ASceneCaptureCamera *Sensor) const
{
  if (!TemplateScene || !Sensor)
  {
    return nullptr;
  }

  UWorld *World = GetWorld();
  if (!World)
  {
    return nullptr;
  }

  FActorSpawnParameters SpawnParams;
  SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  SpawnParams.ObjectFlags = RF_Transient;
  SpawnParams.Template = TemplateScene;
  SpawnParams.Name = *FString::Printf(TEXT("CarlaLumaScene_%s"), *Sensor->GetName());

  ALumaScene *Clone = World->SpawnActor<ALumaScene>(TemplateScene->GetClass(), TemplateScene->GetActorTransform(), SpawnParams);
  if (!Clone)
  {
    UE_LOG(
        LogCarla,
        Warning,
        TEXT("LumaMultiViewSubsystem: failed to spawn ALumaScene clone for sensor '%s'."),
        *Sensor->GetName());
    return nullptr;
  }

  Clone->Tags.AddUnique(CloneTagName);
  return Clone;
}
