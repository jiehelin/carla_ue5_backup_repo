#!/usr/bin/env python3

"""CARLA manual driving with 4 independent RGB cameras in a 2x2 grid.

This script is intentionally lightweight and follows the manual_control style:
- Spawn one ego vehicle
- Attach 4 RGB sensors: front/left/right/rear
- Render all 4 streams at the same time (no view polling/switching)
- Allow per-camera resolution and FOV from CLI
"""

import argparse
import glob
import os
import random
import sys
import weakref

import numpy as np
import pygame


try:
    sys.path.append(
        glob.glob(
            "../carla/dist/carla-*%d.%d-%s.egg"
            % (
                sys.version_info.major,
                sys.version_info.minor,
                "win-amd64" if os.name == "nt" else "linux-x86_64",
            )
        )[0]
    )
except IndexError:
    pass

import carla
from carla import ColorConverter as cc


CAMERA_ORDER = ["front", "rear", "left", "right"]
CAMERA_LABELS = {
    "front": "Front",
    "rear": "Rear",
    "left": "Left",
    "right": "Right",
}


def parse_resolution(value):
    chunks = value.lower().split("x")
    if len(chunks) != 2:
        raise argparse.ArgumentTypeError("resolution must look like 1280x720")
    try:
        width = int(chunks[0])
        height = int(chunks[1])
    except ValueError as exc:
        raise argparse.ArgumentTypeError("resolution must be integerxinteger") from exc
    if width <= 0 or height <= 0:
        raise argparse.ArgumentTypeError("resolution must be positive")
    return width, height


class FourCamRig:
    def __init__(self, world, parent_actor, camera_cfg, gamma=2.2):
        self.world = world
        self.parent_actor = parent_actor
        self.camera_cfg = camera_cfg
        self.gamma = gamma
        self.sensors = {}
        self.surfaces = {}
        self.font = pygame.font.Font(pygame.font.get_default_font(), 20)
        self._spawn_all()

    def _camera_transforms(self):
        bbox = self.parent_actor.bounding_box.extent
        base_x = bbox.x + 0.8
        base_z = bbox.z + 0.7
        return {
            "front": carla.Transform(
                carla.Location(x=base_x, y=0.0, z=base_z), carla.Rotation(yaw=0.0)
            ),
            "rear": carla.Transform(
                carla.Location(x=-base_x, y=0.0, z=base_z), carla.Rotation(yaw=180.0)
            ),
            "left": carla.Transform(
                carla.Location(x=0.0, y=-0.7, z=base_z), carla.Rotation(yaw=-90.0)
            ),
            "right": carla.Transform(
                carla.Location(x=0.0, y=0.7, z=base_z), carla.Rotation(yaw=90.0)
            ),
        }

    def _spawn_all(self):
        bp_lib = self.world.get_blueprint_library()
        cam_bp = bp_lib.find("sensor.camera.rgb")
        transforms = self._camera_transforms()
        post_process_profile = self.get_post_process_profile()
        weak_self = weakref.ref(self)

        for cam_name in CAMERA_ORDER:
            cfg = self.camera_cfg[cam_name]
            bp = cam_bp
            bp.set_attribute("image_size_x", str(cfg["width"]))
            bp.set_attribute("image_size_y", str(cfg["height"]))
            bp.set_attribute("fov", str(cfg["fov"]))
            if bp.has_attribute("gamma"):
                bp.set_attribute("gamma", str(self.gamma))
            if bp.has_attribute("post_process_profile"):
                bp.set_attribute("post_process_profile", post_process_profile)
            sensor = self.world.spawn_actor(bp, transforms[cam_name], attach_to=self.parent_actor)
            sensor.listen(
                lambda image, name=cam_name: FourCamRig._on_image(weak_self, name, image)
            )
            self.sensors[cam_name] = sensor

    @staticmethod
    def get_post_process_profile() -> str:
        return "Town10HD_Opt"

    @staticmethod
    def _on_image(weak_self, cam_name, image):
        self = weak_self()
        if self is None:
            return
        image.convert(cc.Raw)
        array = np.frombuffer(image.raw_data, dtype=np.uint8)
        array = np.reshape(array, (image.height, image.width, 4))
        array = array[:, :, :3][:, :, ::-1]
        self.surfaces[cam_name] = pygame.surfarray.make_surface(array.swapaxes(0, 1))

    def render(self, display):
        width, height = display.get_size()
        half_w = width // 2
        half_h = height // 2
        slots = {
            "front": (0, 0),
            "rear": (half_w, 0),
            "left": (0, half_h),
            "right": (half_w, half_h),
        }

        display.fill((0, 0, 0))
        for cam_name in CAMERA_ORDER:
            pos = slots[cam_name]
            surf = self.surfaces.get(cam_name)
            if surf is not None:
                scaled = pygame.transform.smoothscale(surf, (half_w, half_h))
                display.blit(scaled, pos)
            label = self.font.render(CAMERA_LABELS[cam_name], True, (255, 255, 255))
            display.blit(label, (pos[0] + 12, pos[1] + 8))

    def destroy(self):
        for sensor in self.sensors.values():
            if sensor is not None:
                sensor.stop()
                sensor.destroy()
        self.sensors.clear()
        self.surfaces.clear()


def build_camera_cfg(args):
    cfg = {}
    for cam in CAMERA_ORDER:
        res = getattr(args, f"{cam}_res")
        fov = getattr(args, f"{cam}_fov")
        cfg[cam] = {"width": res[0], "height": res[1], "fov": fov}
    return cfg


def pick_vehicle_blueprint(world, pattern):
    bp_lib = world.get_blueprint_library()
    vehicles = bp_lib.filter(pattern)
    if not vehicles:
        fallback = bp_lib.filter("vehicle.*")
        if not fallback:
            raise RuntimeError(
                f"No vehicle blueprint matches '{pattern}', and no fallback 'vehicle.*' blueprint exists"
            )
        print(
            f"WARNING: No vehicle blueprint matches '{pattern}', fallback to random 'vehicle.*'"
        )
        vehicles = fallback
    bp = random.choice(vehicles)
    if bp.has_attribute("role_name"):
        bp.set_attribute("role_name", "hero")
    return bp


def spawn_ego_vehicle(world, pattern, spawn_index=1):
    vehicle_bp = pick_vehicle_blueprint(world, pattern)
    spawn_points = world.get_map().get_spawn_points()
    if not spawn_points:
        raise RuntimeError("No spawn points on map")

    if spawn_index >= 0:
        if spawn_index >= len(spawn_points):
            raise RuntimeError(
                f"spawn-index {spawn_index} out of range, total spawn points={len(spawn_points)}"
            )
        vehicle = world.try_spawn_actor(vehicle_bp, spawn_points[spawn_index])
        if vehicle is not None:
            print(f"Spawned ego at fixed spawn index {spawn_index}")
            return vehicle
        raise RuntimeError(f"Failed to spawn ego at fixed spawn index {spawn_index}")

    random.shuffle(spawn_points)
    for sp in spawn_points:
        vehicle = world.try_spawn_actor(vehicle_bp, sp)
        if vehicle is not None:
            return vehicle
    raise RuntimeError("Failed to spawn ego vehicle")


def apply_vehicle_control(vehicle, reverse=False):
    keys = pygame.key.get_pressed()
    control = carla.VehicleControl()
    control.throttle = 0.8 if keys[pygame.K_w] or keys[pygame.K_UP] else 0.0
    control.brake = 1.0 if keys[pygame.K_s] or keys[pygame.K_DOWN] else 0.0
    control.steer = 0.0
    if keys[pygame.K_a] or keys[pygame.K_LEFT]:
        control.steer = -0.5
    if keys[pygame.K_d] or keys[pygame.K_RIGHT]:
        control.steer = 0.5
    control.hand_brake = bool(keys[pygame.K_SPACE])
    control.reverse = bool(reverse)
    control.gear = -1 if control.reverse else 1
    vehicle.apply_control(control)


def run(args):
    pygame.init()
    pygame.font.init()
    display = pygame.display.set_mode((args.width, args.height), pygame.HWSURFACE | pygame.DOUBLEBUF)
    pygame.display.set_caption("CARLA 4-Camera RGB View")

    client = carla.Client(args.host, args.port)
    client.set_timeout(20.0)
    world = client.get_world()

    original_settings = world.get_settings()
    ego = None
    cam_rig = None
    clock = pygame.time.Clock()

    try:
        if args.sync:
            settings = world.get_settings()
            settings.synchronous_mode = True
            if settings.fixed_delta_seconds is None:
                settings.fixed_delta_seconds = 0.05
            world.apply_settings(settings)

        ego = spawn_ego_vehicle(world, args.filter, args.spawn_index)
        ego.set_autopilot(args.autopilot)
        cam_rig = FourCamRig(
            world,
            ego,
            build_camera_cfg(args),
            gamma=args.gamma,
        )

        running = True
        reverse_mode = False
        while running:
            if args.sync:
                world.tick()
            else:
                world.wait_for_tick()
            clock.tick_busy_loop(60)

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.KEYUP and event.key == pygame.K_ESCAPE:
                    running = False
                elif event.type == pygame.KEYUP and event.key == pygame.K_q:
                    reverse_mode = not reverse_mode
                    print(f"Reverse mode: {'ON' if reverse_mode else 'OFF'}")

            if not args.autopilot:
                apply_vehicle_control(ego, reverse=reverse_mode)

            cam_rig.render(display)
            pygame.display.flip()
    finally:
        if cam_rig is not None:
            cam_rig.destroy()
        if ego is not None:
            ego.destroy()
        world.apply_settings(original_settings)
        pygame.quit()


def main():
    parser = argparse.ArgumentParser(description="CARLA manual_control style 4-camera viewer")
    parser.add_argument("--host", default="127.0.0.1", help="CARLA host IP")
    parser.add_argument("--port", default=2000, type=int, help="CARLA TCP port")
    parser.add_argument("--width", default=1600, type=int, help="window width")
    parser.add_argument("--height", default=900, type=int, help="window height")
    parser.add_argument("--filter", default="vehicle.*", help="vehicle blueprint filter")
    parser.add_argument("--autopilot", action="store_true", help="enable autopilot for ego vehicle")
    parser.add_argument("--sync", dest="sync", action="store_true", help="run in synchronous mode")
    parser.add_argument("--async", dest="sync", action="store_false", help="run in asynchronous mode")
    parser.set_defaults(sync=True)
    parser.add_argument("--gamma", type=float, default=2.2, help="camera gamma")
    parser.add_argument(
        "--spawn-index",
        type=int,
        default=1,
        help="fixed spawn point index (set -1 for random)",
    )

    parser.add_argument("--front-res", type=parse_resolution, default=(640, 360), help="front resolution, e.g. 1280x720")
    parser.add_argument("--rear-res", type=parse_resolution, default=(640, 360), help="rear resolution, e.g. 1280x720")
    parser.add_argument("--left-res", type=parse_resolution, default=(640, 360), help="left resolution, e.g. 1280x720")
    parser.add_argument("--right-res", type=parse_resolution, default=(640, 360), help="right resolution, e.g. 1280x720")

    parser.add_argument("--front-fov", type=float, default=90.0, help="front camera FOV")
    parser.add_argument("--rear-fov", type=float, default=90.0, help="rear camera FOV")
    parser.add_argument("--left-fov", type=float, default=90.0, help="left camera FOV")
    parser.add_argument("--right-fov", type=float, default=90.0, help="right camera FOV")

    args = parser.parse_args()
    run(args)


if __name__ == "__main__":
    main()
