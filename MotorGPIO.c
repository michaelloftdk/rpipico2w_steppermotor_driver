#include "MotorGPIO.h"
#include "pico/stdlib.h"
#include <stdint.h>

static uint8_t pin_1_a;
static uint8_t pin_2_a;
static uint8_t pin_1_b;
static uint8_t pin_2_b;

void MotorGPIO_SetPin_1_A(uint8_t value)
{
    gpio_put(pin_1_a, value);
}

void MotorGPIO_SetPin_2_A(uint8_t value)
{
    gpio_put(pin_2_a, value);
}

void MotorGPIO_SetPin_1_B(uint8_t value)
{
    gpio_put(pin_1_b, value);
}

void MotorGPIO_SetPin_2_B(uint8_t value)
{
    gpio_put(pin_2_b, value);
}

void MotorGPIO_InitializePins(uint8_t p1A, uint8_t p2A, uint8_t p1B, uint8_t p2B)
{
    pin_1_a = p1A;
    pin_2_a = p2A;
    pin_1_b = p1B;
    pin_2_b = p2B;
    gpio_init(pin_1_a);
    gpio_init(pin_2_a);
    gpio_init(pin_1_b);
    gpio_init(pin_2_b);
    gpio_set_dir(pin_1_a, GPIO_OUT);
    gpio_set_dir(pin_2_a, GPIO_OUT);
    gpio_set_dir(pin_1_b, GPIO_OUT);
    gpio_set_dir(pin_2_b, GPIO_OUT);
}

MotorGPIO_Interface* MotorGPIO_GetInterface()
{
    static MotorGPIO_Interface interface = {
        .SetPin_1_A = MotorGPIO_SetPin_1_A,
        .SetPin_2_A = MotorGPIO_SetPin_2_A,
        .SetPin_1_B = MotorGPIO_SetPin_1_B,
        .SetPin_2_B = MotorGPIO_SetPin_2_B
    };
    return &interface;
}
