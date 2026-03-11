// Copyright (c) 2026 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "CoreMinimal.h"

#include <util/ue-header-guard-begin.h>
#include "UObject/Object.h"
#include <util/ue-header-guard-end.h>

#include "LumaViewHandle.generated.h"

class ASceneCaptureCamera;
class ALumaScene;
class UNiagaraComponent;

USTRUCT()
struct FLumaViewState
{
  GENERATED_BODY()

  UPROPERTY()
  bool bEnable = true;

  UPROPERTY()
  FVector Origin = FVector::ZeroVector;

  UPROPERTY()
  FRotator Rotation = FRotator::ZeroRotator;

  UPROPERTY()
  float FOV = 90.0f;

  UPROPERTY()
  int32 Width = 800;

  UPROPERTY()
  int32 Height = 600;
};

UCLASS()
class CARLA_API ULumaViewHandle : public UObject
{
  GENERATED_BODY()

public:
  void Initialize(ASceneCaptureCamera *InSensor, const FGuid &InViewId);

  const FGuid &GetViewId() const
  {
    return ViewId;
  }

  ASceneCaptureCamera *GetSensor() const
  {
    return Sensor.Get();
  }

  void BindScene(ALumaScene *InScene, bool bInOwnsScene);

  ALumaScene *GetScene() const
  {
    return Scene.Get();
  }

  bool OwnsScene() const
  {
    return bOwnsScene;
  }

  void SetEnabled(bool bInEnabled);

  void ApplyViewState(const FLumaViewState &ViewState);

  void DestroyOwnedScene();

private:
  bool HasUserParameter(const UNiagaraComponent *NiagaraComponent, const FName &ParameterName) const;

  void WarnMissingParameterOnce(const UNiagaraComponent *NiagaraComponent, const FName &ParameterName);

  TWeakObjectPtr<ASceneCaptureCamera> Sensor;
  TWeakObjectPtr<ALumaScene> Scene;
  FGuid ViewId;
  bool bEnabled = true;
  bool bOwnsScene = false;
  TSet<FString> MissingParameterWarnings;
};
