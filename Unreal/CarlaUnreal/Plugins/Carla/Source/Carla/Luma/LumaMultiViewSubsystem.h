// Copyright (c) 2026 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "CoreMinimal.h"
#include "Carla/Luma/LumaViewHandle.h"

#include <util/ue-header-guard-begin.h>
#include "Subsystems/WorldSubsystem.h"
#include <util/ue-header-guard-end.h>

#include "LumaMultiViewSubsystem.generated.h"

class ASceneCaptureCamera;
class ALumaScene;
class ULumaViewHandle;

UCLASS()
class CARLA_API ULumaMultiViewSubsystem : public UWorldSubsystem
{
  GENERATED_BODY()

public:
  void Deinitialize() override;

  ULumaViewHandle *AcquireHandle(ASceneCaptureCamera *Sensor);

  void ReleaseHandle(ASceneCaptureCamera *Sensor);

  bool UpdateViewState(ASceneCaptureCamera *Sensor, const FLumaViewState &ViewState);

private:
  ALumaScene *FindTemplateScene();

  ALumaScene *SpawnSceneCloneFromTemplate(ALumaScene *TemplateScene, ASceneCaptureCamera *Sensor) const;

  TMap<FGuid, FLumaViewState> ViewStates;

  TMap<TWeakObjectPtr<ASceneCaptureCamera>, TObjectPtr<ULumaViewHandle>> ViewHandles;

  TWeakObjectPtr<ALumaScene> CachedTemplateScene;

  static const FName CloneTagName;
};
