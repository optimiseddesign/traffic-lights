from microbit import *

while True:
    LDR_L = pin1.read_analog()
    LDR_R = pin0.read_analog()
    print("LDR_L:", LDR_L)
    print("LDR_R:", LDR_R)
    sleep(1000)
