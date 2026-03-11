import carla
import time

client = carla.Client("localhost",2000)
world = client.get_world()

while True:
    time.sleep(0.01)