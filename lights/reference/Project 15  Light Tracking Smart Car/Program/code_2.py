from microbit import *
from keyes_MiniCar import *
minicar = MiniCar()

while True:
    LDR_L = pin1.read_analog()
    LDR_R = pin0.read_analog()
    if LDR_L > 650 and LDR_R > 650:
        minicar.Motor_L(1, 70)
        minicar.Motor_R(1, 70)
    elif LDR_L > 650 and LDR_R <= 650:
        minicar.Motor_L(0, 70)
        minicar.Motor_R(1, 70)
    elif LDR_L <= 650 and LDR_R > 650:
        minicar.Motor_L(1, 70)
        minicar.Motor_R(0, 70)
    else:
        minicar.Motor_stop()
