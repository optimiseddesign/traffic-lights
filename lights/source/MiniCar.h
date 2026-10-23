#ifndef MINICAR_H
#define MINICAR_H

#include "MicroBit.h"

/**
 * Simple motor control class for Keyes MiniCar robot
 * Based on the Python keyes_MiniCar.py reference implementation
 */
class MiniCar {
private:
    static const uint8_t I2C_ADDRESS = 0x30;  // I2C address of PWM controller
    MicroBitI2C* i2c;                          // Reference to I2C interface
    
    /**
     * Set PWM value for a specific channel
     * @param channel PWM channel (1-4 for motors)
     * @param value PWM value (0-255)
     * @return DEVICE_OK on success, error code on failure
     */
    int setPWM(uint8_t channel, uint8_t value);

public:
    /**
     * Constructor
     * @param i2cInterface Reference to micro:bit I2C interface
     */
    MiniCar(MicroBitI2C& i2cInterface);
    
    /**
     * Initialize - stop all motors
     * @return DEVICE_OK on success
     */
    int init();
    
    /**
     * Control left motor (matches Python Motor_L function)
     * @param direction 1 = forward, 0 = backward  
     * @param speed Motor speed (0-255)
     * @return DEVICE_OK on success
     */
    int Motor_L(uint8_t direction, uint8_t speed);
    
    /**
     * Control right motor (matches Python Motor_R function)
     * @param direction 1 = forward, 0 = backward
     * @param speed Motor speed (0-255) 
     * @return DEVICE_OK on success
     */
    int Motor_R(uint8_t direction, uint8_t speed);
    
    /**
     * Stop all motors (matches Python Motor_stop function)
     * @return DEVICE_OK on success
     */
    int Motor_stop();
    
    /**
     * Control left RGB LED (matches Python left_red, left_green, left_blue functions)
     * @param red Red component (0-255, 0=brightest, 255=off)
     * @param green Green component (0-255, 0=brightest, 255=off)
     * @param blue Blue component (0-255, 0=brightest, 255=off)
     * @return DEVICE_OK on success
     */
    int setLeftRGB(uint8_t red, uint8_t green, uint8_t blue);
    
    /**
     * Control right RGB LED (matches Python right_red, right_green, right_blue functions)
     * @param red Red component (0-255, 0=brightest, 255=off)
     * @param green Green component (0-255, 0=brightest, 255=off)
     * @param blue Blue component (0-255, 0=brightest, 255=off)
     * @return DEVICE_OK on success
     */
    int setRightRGB(uint8_t red, uint8_t green, uint8_t blue);
    
    /**
     * Turn off all LEDs (matches Python led_off function)
     * @return DEVICE_OK on success
     */
    int led_off();
};

#endif // MINICAR_H