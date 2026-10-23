#include "MiniCar.h"

MiniCar::MiniCar(MicroBitI2C& i2cInterface) : i2c(&i2cInterface) {
    // Constructor - store reference to I2C interface
}

int MiniCar::init() {
    // Initialize by stopping all motors
    return Motor_stop();
}

int MiniCar::setPWM(uint8_t channel, uint8_t value) {
    // Send PWM command to I2C controller
    uint8_t data[2] = {channel, value};
    return i2c->write(I2C_ADDRESS << 1, data, 2, false);
}

int MiniCar::Motor_L(uint8_t direction, uint8_t speed) {
    // Left motor uses PWM channels 1 & 2
    // Matches Python: Motor_L(self, stateR, right1)
    int result = DEVICE_OK;
    
    if (direction == 1) {
        // Forward: channel 1 = 0, channel 2 = speed
        result = setPWM(1, 0);
        if (result != DEVICE_OK) return result;
        result = setPWM(2, speed);
    } else {
        // Backward: channel 1 = speed, channel 2 = 0
        result = setPWM(1, speed);
        if (result != DEVICE_OK) return result;
        result = setPWM(2, 0);
    }
    
    return result;
}

int MiniCar::Motor_R(uint8_t direction, uint8_t speed) {
    // Right motor uses PWM channels 3 & 4
    // Matches Python: Motor_R(self, stateL, left1)
    int result = DEVICE_OK;
    
    if (direction == 1) {
        // Forward: channel 3 = speed, channel 4 = 0
        result = setPWM(3, speed);
        if (result != DEVICE_OK) return result;
        result = setPWM(4, 0);
    } else {
        // Backward: channel 3 = 0, channel 4 = speed
        result = setPWM(3, 0);
        if (result != DEVICE_OK) return result;
        result = setPWM(4, speed);
    }
    
    return result;
}

int MiniCar::Motor_stop() {
    // Stop both motors by setting all channels to 0
    // Matches Python: Motor_stop(self)
    int result = DEVICE_OK;
    
    result = setPWM(1, 0);
    if (result != DEVICE_OK) return result;
    result = setPWM(2, 0);
    if (result != DEVICE_OK) return result;
    result = setPWM(3, 0);
    if (result != DEVICE_OK) return result;
    result = setPWM(4, 0);
    
    return result;
}

int MiniCar::setLeftRGB(uint8_t red, uint8_t green, uint8_t blue) {
    // Left RGB LED uses PWM channels: 8=red, 7=green, 6=blue
    // Matches Python: left_red(8), left_green(7), left_blue(6)
    int result = DEVICE_OK;
    
    result = setPWM(8, red);
    if (result != DEVICE_OK) return result;
    result = setPWM(7, green);
    if (result != DEVICE_OK) return result;
    result = setPWM(6, blue);
    
    return result;
}

int MiniCar::setRightRGB(uint8_t red, uint8_t green, uint8_t blue) {
    // Right RGB LED uses PWM channels: 9=red, 10=green, 5=blue
    // Matches Python: right_red(9), right_green(10), right_blue(5)
    int result = DEVICE_OK;
    
    result = setPWM(9, red);
    if (result != DEVICE_OK) return result;
    result = setPWM(10, green);
    if (result != DEVICE_OK) return result;
    result = setPWM(5, blue);
    
    return result;
}

int MiniCar::led_off() {
    // Turn off all LEDs by setting all RGB channels to 255 (inverted logic)
    // Matches Python: led_off(self)
    int result = DEVICE_OK;
    
    result = setLeftRGB(255, 255, 255);
    if (result != DEVICE_OK) return result;
    result = setRightRGB(255, 255, 255);
    
    return result;
}