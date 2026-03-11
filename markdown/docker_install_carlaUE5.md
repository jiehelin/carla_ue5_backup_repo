# 构建docker

在docker 容器中安装carla ue5 

1. cmake 3.28
2. 不能是root用户
3. 加载gpu + x11
4. local utf8
5. 容器内开代理

重新使用dockerfile 构建镜像

```bush
docker build -t carla-ue5-builder:latest .
```

运行容器

```bush
docker run -it --gpus all \
    --network host \
    -e DISPLAY=$DISPLAY \
    -e http_proxy="http://127.0.0.1:7890" \
    -e https_proxy="http://127.0.0.1:7890" \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v ~/Documents/CARLA_UE5/CARLA_UE5_Docker/CarlaUE5:/workspace/CarlaUE5 \
    -v ~/Documents/CARLA_UE5/CARLA_UE5_Docker/UnrealEngine5_carla:/workspace/UnrealEngine5_carla \
    --name carla_build \
    carla-ue5-builder:latest
    
docker run -it \
    --gpus all \
    --network host \
    --shm-size=32g \
    --ulimit memlock=-1 \
    --ulimit stack=67108864 \
    -e DISPLAY=$DISPLAY \
    -e http_proxy="http://127.0.0.1:7890" \
    -e https_proxy="http://127.0.0.1:7890" \
    -e HTTP_PROXY="http://127.0.0.1:7890" \
    -e HTTPS_PROXY="http://127.0.0.1:7890" \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v ~/Documents/CARLA_UE5/CARLA_UE5_Docker/CarlaUE5:/workspace/CarlaUE5 \
    -v ~/Documents/CARLA_UE5/CARLA_UE5_Docker/UnrealEngine5_carla:/workspace/UnrealEngine5_carla \
    --name carla_build \
    carla-ue5-builder:latest
    
docker run -it --rm \
    --name carla_build \
    --gpus all \
    --cpus=12 \
    --memory=60g \
    --shm-size=32g \
    --ulimit memlock=-1 \
    --ulimit stack=67108864 \
    --network host \
    -e DISPLAY=$DISPLAY \
    -e http_proxy="http://127.0.0.1:7890" \
    -e https_proxy="http://127.0.0.1:7890" \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v ~/Documents/CARLA_UE5/CARLA_UE5_Docker/CarlaUE5:/workspace/CarlaUE5 \
    -v ~/Documents/CARLA_UE5/CARLA_UE5_Docker/UnrealEngine5_carla:/workspace/UnrealEngine5_carla \
    carla-ue5-builder:latest
    
    
docker run -it --rm \
    --name carla_build \
    --gpus all \
    --cpus=10 \
    --shm-size=32g \
    --ulimit memlock=-1 \
    --ulimit stack=67108864 \
    --network host \
    --user $(id -u):$(id -g) \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v ~/Documents/CARLA_UE5/CARLA_UE5_Docker/CarlaUE5:/workspace/CarlaUE5 \
    -v ~/Documents/CARLA_UE5/CARLA_UE5_Docker/UnrealEngine5_carla:/workspace/UnrealEngine5_carla \
    carla-ue5-builder:latest

```

验证代理成功？

```
echo $http_proxy
echo $https_proxy
curl -I https://github.com  # 可以返回 HTTP 头
```

进入容器后：手动安装这个

```bush
sudo apt-get update
sudo apt-get install -y ucf libnss3 libgtk-3-0
```

添加path

```bush
export CARLA_UNREAL_ENGINE_PATH=/workspace/UnrealEngine5_carla
export CARLA_UNREAL_ENGINE_PATH=/home/xiehelin/Documents/CARLA_UE5/CARLA_UE5_Docker/UnrealEngine5_carla

```

编译环境

之前先删除上次构建

````bush
cd /workspace/CarlaUE5
cd /home/xiehelin/Documents/CARLA_UE5/CARLA_UE5_Docker/CarlaUE5
rm -rf Build
rm -rf Unreal/CarlaUnreal/Intermediate
rm -rf Unreal/CarlaUnreal/Saved

cd /workspace/UnrealEngine5_carla
cd /home/xiehelin/Documents/CARLA_UE5/CARLA_UE5_Docker/UnrealEngine5_carla
rm -rf Engine/Intermediate
````

```bush
cd CarlaUE5
./CarlaSetup.sh --interactive
```

启动编译器

```bush
cmake --build Build --target launch
```

进入ue5之后，使用新的终端进入docker

```bush
docker exec -it -u carla_user carla_build bash
```

用不用docker都行，帧数都比较低
