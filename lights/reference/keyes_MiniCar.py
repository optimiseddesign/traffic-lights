# from microbit import pin1, pin2, sleep, i2c
from microbit import *
import ustruct
import machine
from time import sleep_us, ticks_us
distance = 0

class MiniCar(object):
    def __init__(self):
        self.add = 0x30
        self.set_all_pwm(0)
        self.led_off()

    def set_pwm(self, channel, value):
        i2c.write(self.add, bytearray([channel, value & 0xFF]), repeat=False)

    def set_all_pwm(self, value):
        i2c.write(self.add, bytearray([0x01, value & 0xFF]), repeat=False)
        i2c.write(self.add, bytearray([0x02, value & 0xFF]), repeat=False)
        i2c.write(self.add, bytearray([0x03, value & 0xFF]), repeat=False)
        i2c.write(self.add, bytearray([0x04, value & 0xFF]), repeat=False)

    def Motor_L(self, stateR, right1):
        if (stateR == 0):
            self.set_pwm(1, right1)
            self.set_pwm(2, 0)
        elif (stateR == 1):
            self.set_pwm(1, 0)
            self.set_pwm(2, right1)

    def Motor_R(self, stateL, left1):
        if (stateL == 0):
            self.set_pwm(3, 0)
            self.set_pwm(4, left1)
        elif (stateL == 1):
            self.set_pwm(3, left1)
            self.set_pwm(4, 0)

    def Motor_stop(self):
        self.Motor_L(1 , 0)
        self.Motor_R(1 , 0)

    def left_red(self , red):
        self.set_pwm(8 , red)

    def left_green(self ,  green):
        self.set_pwm(7 , green)

    def left_blue(self ,  blue):
        self.set_pwm(6 , blue)

    def right_red(self , red):
        self.set_pwm(9 , red)

    def right_green(self , green):
        self.set_pwm(10 , green)

    def right_blue(self , blue):
        self.set_pwm(5 , blue)

    def led_off(self):
        self.left_red(255)
        self.left_green(255)
        self.left_blue(255)
        self.right_red(255)
        self.right_green(255)
        self.right_blue(255)

    def get_distance(self):
        pin14.write_digital(0)
        sleep_us(2)
        pin14.write_digital(1)
        sleep_us(10)
        pin14.write_digital(0)

        t = machine.time_pulse_us(pin15 , 1 , 35000)
        if (t <= 0 and self.lastEchoDuration >= 0) :
            t = self.lastEchoDuration

        self.lastEchoDuration = t
        return round(t * 0.013)



