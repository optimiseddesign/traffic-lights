from microbit import *
from keyes_MiniCar import *
minicar = MiniCar()

while True:
    distance = 0
    distance = minicar.get_distance()
    if distance > 10:
        minicar.Motor_L(1, 70)
        minicar.Motor_R(1, 70)
        sleep(50)
    else:
        minicar.Motor_L(0, 70)
        minicar.Motor_R(1, 70)
        sleep(50)
