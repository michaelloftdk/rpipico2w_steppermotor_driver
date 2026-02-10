#ifndef MOTOR_DRIVE_H
#define MOTOR_DRIVE_H

#include <stdint.h>
#include "MotorGPIO.h"

typedef enum
{
    DRIVE_MODE_WAVE,
    DRIVE_MODE_FULL_STEP,
    DRIVE_MODE_HALF_STEP
} DriveMode;

void MotorDrive_Initialize(MotorGPIO_Interface* gpioInterface);

void MotorDrive_SetDriveMode(DriveMode driveMode);

void MotorDrive_DriveOneStepForward();

void MotorDrive_DriveOneStepReverse();

uint8_t MotorDrive_GetStepsForOneRotation(uint8_t stepsForWaveMode);


#endif // MOTOR_DRIVE_H
