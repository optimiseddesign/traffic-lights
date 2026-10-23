from microbit import *
from keyes_MiniCar import *
minicar = MiniCar()
distance = 0

while True:
    distance = minicar.get_distance()
    print("distance:", distance)
    sleep(1000)
