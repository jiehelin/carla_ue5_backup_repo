# Luma 3DGS 多视角显示说明

## 问题
左右后三个相机视角无法显示 3DGS 点云，只有前向视角（与编辑器 Spectator 同步的视角）能正确显示。

## 原因
Luma 3DGS 的渲染依赖主视口（Spectator/PlayerController）的视角，用于做 view-dependent 的渲染或裁剪。原先 `SceneCaptureSensor` 在 PrePhysTick 中把 Spectator 同步到**每个**相机，导致多个相机互相覆盖，只有最后一个相机的视角会被 Luma 使用。

## 已做修改

### 1. SceneCaptureSensor.cpp（CARLA 插件）
- **PrePhysTick**：移除 Spectator 同步，避免多相机互相覆盖
- **PostPhysTick**：在每个相机**捕获前**将 Spectator 同步到该相机，使 Luma 在捕获时使用正确的视角

这样每次 SceneCapture 执行时，主视口都指向当前相机，Luma 应能为四个方向正确渲染 3DGS。

### 2. manual_control.py（Python）
- Spectator 由 Python 按前向相机位姿显式计算并更新，保证编辑器主窗口视角固定为前视

## 若仍无效

若修改后左右后视角仍无 3DGS，可能是：

1. **渲染顺序**：SceneCapture 可能在渲染线程异步执行，Spectator 在捕获时已被其它相机覆盖。此时需要让四个相机的捕获严格顺序执行（串行），或让 Luma 使用当前渲染视图而非主视口。

2. **Luma 插件内部**：Luma 的 Gaussian Splat 材质/Niagara 可能硬编码使用主视口的相机。需要在 Luma 插件中检查：
   - 材质是否使用 `SceneView.TranslatedViewProjectionMatrix` 等当前视图参数
   - 是否错误使用了 `GetWorld()->GetFirstPlayerController()` 或类似接口获取视角

3. **Luma 的 Crop/Cull**：若启用了 Crop Box 或 Cull Box，可能按主视口视角裁剪，导致部分视角下点云被裁掉。可尝试关闭这些选项测试。

## 重新编译
修改 SceneCaptureSensor.cpp 后需要重新编译 CARLA：

```bash
# 在 CARLA_UE5 工程目录下
# 使用 Unreal 编辑器打开工程并编译，或使用 build 脚本
```
