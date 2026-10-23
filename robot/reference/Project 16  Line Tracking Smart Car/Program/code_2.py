from microbit import *
from keyes_MiniCar import *
minicar = MiniCar()
pin12.set_pull(pin12.PULL_UP)
pin13.set_pull(pin13.PULL_UP)
sensor_L = 0
sensor_R = 0

while True:
    sensor_L = pin13.read_digital()
    sensor_R = pin12.read_digital()
    print("sensor_L:", sensor_L)
    print("sensor_R:", sensor_R)
    if sensor_L == 1 and sensor_R == 1:
        minicar.Motor_L(1, 100)
        minicar.Motor_R(1, 100)
    elif sensor_L == 0 and sensor_R == 1:
        minicar.Motor_L(1, 100)
        minicar.Motor_R(0, 100)
    elif sensor_L == 1 and sensor_R == 0:
        minicar.Motor_L(0, 100)
        minicar.Motor_R(1, 100)
    else:
        minicar.Motor_stop()
