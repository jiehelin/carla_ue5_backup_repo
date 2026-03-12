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

  UPROPERTY()
  FVector4 LumaWorldToClipRow0 = FVector4(1.f, 0.f, 0.f, 0.f);

  UPROPERTY()
  FVector4 LumaWorldToClipRow1 = FVector4(0.f, 1.f, 0.f, 0.f);

  UPROPERTY()
  FVector4 LumaWorldToClipRow2 = FVector4(0.f, 0.f, 1.f, 0.f);

  UPROPERTY()
  FVector4 LumaWorldToClipRow3 = FVector4(0.f, 0.f, 0.f, 1.f);

  UPROPERTY()
  FVector4 LumaClipToWorldRow0 = FVector4(1.f, 0.f, 0.f, 0.f);

  UPROPERTY()
  FVector4 LumaClipToWorldRow1 = FVector4(0.f, 1.f, 0.f, 0.f);

  UPROPERTY()
  FVector4 LumaClipToWorldRow2 = FVector4(0.f, 0.f, 1.f, 0.f);

  UPROPERTY()
  FVector4 LumaClipToWorldRow3 = FVector4(0.f, 0.f, 0.f, 1.f);

  UPROPERTY()
  FVector4 LumaWorldToViewRow0 = FVector4(1.f, 0.f, 0.f, 0.f);

  UPROPERTY()
  FVector4 LumaWorldToViewRow1 = FVector4(0.f, 1.f, 0.f, 0.f);

  UPROPERTY()
  FVector4 LumaWorldToViewRow2 = FVector4(0.f, 0.f, 1.f, 0.f);

  UPROPERTY()
  FVector4 LumaWorldToViewRow3 = FVector4(0.f, 0.f, 0.f, 1.f);

  UPROPERTY()
  FVector4 LumaViewToWorldRow0 = FVector4(1.f, 0.f, 0.f, 0.f);

  UPROPERTY()
  FVector4 LumaViewToWorldRow1 = FVector4(0.f, 1.f, 0.f, 0.f);

  UPROPERTY()
  FVector4 LumaViewToWorldRow2 = FVector4(0.f, 0.f, 1.f, 0.f);

  UPROPERTY()
  FVector4 LumaViewToWorldRow3 = FVector4(0.f, 0.f, 0.f, 1.f);
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
