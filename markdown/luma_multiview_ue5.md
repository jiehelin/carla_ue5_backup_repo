# UE5 Luma Multi-View in CARLA (4 RGB Sensors, No Python Polling)

## Goal
Implement per-sensor isolated Luma view rendering in CARLA UE5:
- `front/left/right/rear` remain 4 independent RGB sensors.
- Each sensor owns its own Luma view state and Luma scene clone.
- No global spectator override and no Python-side viewpoint polling.

## Implemented Architecture

### 1. Per-sensor view state
Added `FLumaViewState` in Carla plugin:
- `bEnable`
- `Origin`
- `Rotation`
- `FOV`
- `Width`
- `Height`

### 2. Multi-view subsystem
Added `ULumaMultiViewSubsystem : UWorldSubsystem`:
- `TMap<FGuid, FLumaViewState> ViewStates`
- `TMap<TWeakObjectPtr<ASceneCaptureCamera>, TObjectPtr<ULumaViewHandle>> ViewHandles`

Capabilities:
- `AcquireHandle(ASceneCaptureCamera*)`
- `ReleaseHandle(ASceneCaptureCamera*)`
- `UpdateViewState(ASceneCaptureCamera*, const FLumaViewState&)`

Behavior:
- Each sensor gets a stable `ViewId` for its lifetime.
- Subsystem finds one placed `ALumaScene` template actor from world.
- Subsystem spawns one `ALumaScene` clone per sensor (non-shared).

### 3. View handle
Added `ULumaViewHandle`:
- Holds `ViewId`, bound sensor, bound `ALumaScene`, ownership flag.
- Applies view state to component-level Niagara parameters.

Applied Niagara parameter names:
- `LumaViewEnable`
- `LumaViewOrigin`
- `LumaViewRotation`
- `LumaViewFOV`
- `LumaViewWidth`
- `LumaViewHeight`

Missing parameter policy:
- Does not interrupt render.
- Logs warning once per `(component, parameter)` pair.

### 4. RGB sensor integration point
Integrated only in `ASceneCaptureCamera` (RGB):
- `BeginPlay`: acquire handle from `ULumaMultiViewSubsystem`.
- `PostPhysTick`: build current sensor `FLumaViewState` from capture component and apply before capture.
- `EndPlay`: release handle and destroy owned Luma clone through subsystem.

### 5. Removal of old global path
Removed old `ASceneCaptureSensor` global path used for Luma:
- Removed spectator synchronization before capture.
- Removed per-capture `FlushRenderingCommands()` serialization path.

This ensures no global camera override is used in multi-view Luma flow.

## Engine/Plugin dependency changes
- Carla module private dependencies include:
  - `Niagara`
  - `LumaRuntime`
- Carla plugin dependency includes:
  - `LumaAIPlugin`

## Runtime assumptions
- Level has at least one non-clone `ALumaScene` actor as template.
- This implementation currently targets RGB sensors (`ASceneCaptureCamera`) only.

## Quick Validation Checklist
1. Spawn 4 RGB sensors (`front/left/right/rear`) with independent render targets.
2. Confirm all 4 outputs include 3DGS content simultaneously.
3. Change one sensor FOV or resolution; only that sensor output changes.
4. Destroy one sensor; others continue rendering normally.
5. Confirm editor/spectator view no longer controls Luma sensor output.

## Changed Files (Code)
- `Unreal/CarlaUnreal/Plugins/Carla/Source/Carla/Luma/LumaViewHandle.h`
- `Unreal/CarlaUnreal/Plugins/Carla/Source/Carla/Luma/LumaViewHandle.cpp`
- `Unreal/CarlaUnreal/Plugins/Carla/Source/Carla/Luma/LumaMultiViewSubsystem.h`
- `Unreal/CarlaUnreal/Plugins/Carla/Source/Carla/Luma/LumaMultiViewSubsystem.cpp`
- `Unreal/CarlaUnreal/Plugins/Carla/Source/Carla/Sensor/SceneCaptureCamera.h`
- `Unreal/CarlaUnreal/Plugins/Carla/Source/Carla/Sensor/SceneCaptureCamera.cpp`
- `Unreal/CarlaUnreal/Plugins/Carla/Source/Carla/Sensor/SceneCaptureSensor.cpp`
- `Unreal/CarlaUnreal/Plugins/Carla/Source/Carla/Carla.Build.cs`
- `Unreal/CarlaUnreal/Plugins/Carla/Carla.uplugin`
