# Add 3dgs plugin in carlaUE5

## Check version

```bush
cd /home/xiehelin/Documents/CARLA_UE5/CARLA_UE5_Docker/UnrealEngine5_carla/Engine/Build
cat Build.version
```

```
{
	"MajorVersion": 5,
	"MinorVersion": 5,
	"PatchVersion": 0,
	"Changelist": 0,
	"CompatibleChangelist": 0,
	"IsLicenseeVersion": 0,
	"IsPromotedBuild": 0,
	"BranchName": "UE5"
}
```

## Download luma ai source code

[https://lumalabs.ai/ue](https://lumalabs.ai/ue)

![image-20260305085637715](/home/xiehelin/Documents/GitHub_Lib/Note/carlaUE5/add_3dgs.assets/image-20260305085637715.png)

Extract the compressed file LumaAI_2023_25_12_marketplace_5.0.zip and copy its contents to the **Unreal/CarlaUnreal/Plugins** path under the CARLA root directory.

![image-20260305085814794](/home/xiehelin/Documents/GitHub_Lib/Note/carlaUE5/add_3dgs.assets/image-20260305085814794.png)

change source code

- use c++ 20，same as UE5

```bush
LumaEditor.build.cs
		CppStandard = CppStandardVersion.Cpp17;
```

```bush
LumaEditor.build.cs
		// CppStandard = CppStandardVersion.Cpp17;
		CppStandard = CppStandardVersion.Cpp20;
```

- 3DGS data precision: 16-bit floating point → 32-bit floating point

  LumaScene.h

  LumaSplatLoader.cpp

  LoaderUtils.cpp

![image-20260305094427102](/home/xiehelin/Documents/GitHub_Lib/Note/carlaUE5/add_3dgs.assets/image-20260305094427102.png)

- Adjusting Gaussian block partitioning logic

  ```bush
  CarlaUE5/Unreal/CarlaUnreal/Plugins/LumaAI_2023_25_12_marketplace_5.0/Source/LumaEditor/Private/Loaders/LumaSplatLoader.cpp
  ```

  原版用 “先按 iteration 填、再补 0、再按 x/y 再调一次 lambda” 的复杂循环；

  改版改成单次 for 按 flat_index 遍历，未命中时填 0，并统一用 TArray<float> 和 InitializeF32GaussianTextureDataPerTexel。

  有一处原版把 gaussian.scale 赋给变量名 Rot 的 bug，改版改成变量名 Scale 并正确写 scale 分量。

  原因： 在支持 32 位精度的同时，简化逻辑、修掉错误的变量命名，避免 scale 被当 rotation 用导致显示异常。

![image-20260305094939692](/home/xiehelin/Documents/GitHub_Lib/Note/carlaUE5/add_3dgs.assets/image-20260305094939692.png)

- lumaScene Runtime close

  ![image-20260305095253119](/home/xiehelin/Documents/GitHub_Lib/Note/carlaUE5/add_3dgs.assets/image-20260305095253119.png)

用 C++20 编译通过 → 用 32 位浮点提升 3DGS 清晰度 → 关掉纯 Runtime 限制，让点云在编辑器和 CARLA 相机里都能被渲染。

## Import a new map from Roadrunner

open Roadrunner and create a new road , click file -> export -> carla filmbox it can be generate .xodr .fbx file 

the next is put those file in carla main folder , import folder as show picture below:

![image-20260305100712230](/home/xiehelin/Documents/GitHub_Lib/Note/carlaUE5/add_3dgs.assets/image-20260305100712230.png)

run import.py 

```bush
cd CarlaUE5/Util/Tools
# activate conda env 
python Import.py --package MyPackage2
```

the parmater --package MyPackage2 

folder name <MyPackage2> need same as roadrunner export file name , it must be same!!!

After the Python script finishes running, you will see the ue5 assets in folder

![image-20260305101319024](/home/xiehelin/Documents/GitHub_Lib/Note/carlaUE5/add_3dgs.assets/image-20260305101319024.png)

## Compile with ue5

Launch the editor:

```bush
cmake --build Build --target launch
```

![image-20260305101527552](/home/xiehelin/Documents/GitHub_Lib/Note/carlaUE5/add_3dgs.assets/image-20260305101527552.png)

need click yes and keep going the compile

![image-20260305104829035](/home/xiehelin/Documents/GitHub_Lib/Note/carlaUE5/add_3dgs.assets/image-20260305104829035.png)

## Add 3dgs (.ply) in ue5 as assets

Open the imported OpenDrive level
![image-20260309092017511](/home/xiehelin/Documents/GitHub_Lib/Note/carlaUE5/add_3dgs.assets/image-20260309092017511.png)

Remove all built-in lighting content, add the BP_Carla_Sky blueprint in the Assets section, not in Place Actors, and use the default lighting for the default scene.Default lighting is here
![image-20260309092203517](/home/xiehelin/Documents/GitHub_Lib/Note/carlaUE5/add_3dgs.assets/image-20260309092203517.png)

Add to Outliner

![image-20260309092253570](/home/xiehelin/Documents/GitHub_Lib/Note/carlaUE5/add_3dgs.assets/image-20260309092253570.png)

## Put ply file in editor windows

Adding a trained PLY GS point cloud to the asset will automatically trigger the Luma import process.![image-20260309092405373](/home/xiehelin/Documents/GitHub_Lib/Note/carlaUE5/add_3dgs.assets/image-20260309092405373.png)

![image-20260309092701817](/home/xiehelin/Documents/GitHub_Lib/Note/carlaUE5/add_3dgs.assets/image-20260309092701817.png)

This outputs a blueprint for particle effects, exporting the GS point cloud as particle effects to Unreal Engine 5.

Baked is the original color baking; use it if you don't want ambient light to affect particle colors.

Baked NO TAA: Particles with original baked colors without edge anti-aliasing.

Dynamic: Particles with TAA anti-aliasing that change with ambient light.

Dynamic NO TAA: Particles without anti-aliasing and with dynamic ambient light.

![image-20260309093151942](/home/xiehelin/Documents/GitHub_Lib/Note/carlaUE5/add_3dgs.assets/image-20260309093151942.png)

You can adjust the ambient light color and intensity (lux).

## Adjust carla post processing

When running the scene using the Python API, you need to check the following function in your script, which adds the display from the UE5 RGB camera to the PyGame window:

```python
    def get_post_process_profile(self, map_name: str) -> str:
        if "Town10HD_Opt" in map_name:
            return "Town10HD_Opt"
        if "MyPackage2" in map_name:
            return "MyPackage2_Opt"
        return "Default"
```

## Run carla pythonAPI and show ply in pygame windows

```bush
cd /home/xiehelin/Documents/CARLA_UE5/CARLA_UE5_Docker/CarlaUE5/PythonAPI/examples
python manual_control.py
```

## 调整luma渲染管线，增加视角
