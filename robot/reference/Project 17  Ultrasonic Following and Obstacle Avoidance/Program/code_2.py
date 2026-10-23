from microbit import *
from keyes_MiniCar import *
minicar = MiniCar()

while True:
    distance = 0
    distance = minicar.get_distance()
    if distance >= 10 and distance <= 30:
        minicar.Motor_L(1, 100)
        minicar.Motor_R(1, 100)
        sleep(50)
    elif distance <= 6:
        minicar.Motor_L(0, 100)
        minicar.Motor_R(0, 100)
        sleep(50)
    elif (distance > 6 and distance < 10) or distance > 30:
        minicar.Motor_stop()
        sleep(50)
