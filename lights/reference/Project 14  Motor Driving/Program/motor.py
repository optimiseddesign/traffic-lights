from microbit import *
from keyes_MiniCar import *
minicar = MiniCar()

while True:
    #Forward
    minicar.Motor_L(1, 70)
    minicar.Motor_R(1, 70)
    sleep(1000)
    #back
    minicar.Motor_L(0, 70)
    minicar.Motor_R(0, 70)
    sleep(1000)
    #turn left
    minicar.Motor_L(0, 70)
    minicar.Motor_R(1, 70)
    sleep(1000)
    #turn right
    minicar.Motor_L(1, 70)
    minicar.Motor_R(0, 70)
    sleep(1000)
    minicar.Motor_stop()
    sleep(1000)

