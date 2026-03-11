clone

```bush
git clone -b ue5-dev https://github.com/carla-simulator/carla.git CarlaUE5
```

### Set up the environment

```
cd CarlaUE5
./CarlaSetup.sh --interactive
```

如果构建失败

删除上次构建

```
cd /workspace/CarlaUE5
cd /home/xiehelin/Documents/CARLA_UE5/CARLA_UE5_Docker/CarlaUE5
rm -rf Build
rm -rf Unreal/CarlaUnreal/Intermediate
rm -rf Unreal/CarlaUnreal/Saved

cd /workspace/UnrealEngine5_carla
cd /home/xiehelin/Documents/CARLA_UE5/CARLA_UE5_Docker/UnrealEngine5_carla
rm -rf Engine/Intermediate
```

add Path

```
export CARLA_UNREAL_ENGINE_PATH=/home/xiehelin/Documents/CARLA_UE5/CARLA_UE5_Docker/UnrealEngine5_carla
```

重试

```
cd CarlaUE5
./CarlaSetup.sh --interactive
```

### Launch the editor:

```
cmake --build Build --target launch
```

### Build a package with CARLA UE5

```
cmake --build Build --target package
```

- Run the package

  ```
  cd ~/CarlaUE5/Build/Package/Carla-0.10.0-Linux-Shipping/Linux
  ./CarlaUnreal.sh
  ```

- run the native ROS2 interface

  ```
  ./CarlaUnreal.sh --ros2
  ```


Import OpenDrive 

```bush
cd /home/xiehelin/Documents/CARLA_UE5/CARLA_UE5_Docker/CarlaUE5
python Util/Tools/Import.py --package MyPackage2
```

