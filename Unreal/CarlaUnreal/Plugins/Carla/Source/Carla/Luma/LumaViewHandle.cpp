// Copyright (c) 2026 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla/Luma/LumaViewHandle.h"
#include "Carla.h"

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
