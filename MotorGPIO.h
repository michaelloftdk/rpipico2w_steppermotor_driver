#ifndef MOTOR_GPIO_H
#define MOTOR_GPIO_H

#include <stdint.h>

void MotorGPIO_InitializePins(uint8_t pin1A, uint8_t pin2A, uint8_t pin1B, uint8_t pin2B);

// declare struct with function pointers
typedef struct
{
    void (*SetPin_1_A)(uint8_t value);
    void (*SetPin_2_A)(uint8_t value);
    void (*SetPin_1_B)(uint8_t value);
    void (*SetPin_2_B)(uint8_t value);
} MotorGPIO_Interface;

// function to get the interface
MotorGPIO_Interface* MotorGPIO_GetInterface();

#endif // MOTOR_GPIO_H