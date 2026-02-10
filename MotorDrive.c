#include <stdint.h>
#include "MotorDrive.h"
#include "pico/stdlib.h"

typedef struct 
{
    uint8_t _1a;
    uint8_t _2a;
    uint8_t _1b;
    uint8_t _2b;
} MotorDrive_Exitation;

#define STEP_ARRAY_LENGTH 8
// Step array length is 8, even though wave and full step only use 4 steps.
// The steps for wave and full step are repeated, to allow the iteration
// logic to be the same for all drive modes.

static const MotorDrive_Exitation stepArrayWave[STEP_ARRAY_LENGTH] =
{
//  1a 2a 1b 2b  
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1},
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}
};

static const MotorDrive_Exitation stepArrayFullStep[STEP_ARRAY_LENGTH] =
{
//  1a 2a 1b 2b  
    {1, 0, 0, 1},
    {1, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 1},
    {1, 0, 0, 1},
    {1, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 1}
};

static const MotorDrive_Exitation stepArrayHalfStep[STEP_ARRAY_LENGTH] =
{
//  1a 2a 1b 2b  
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

static DriveMode activeDriveMode = DRIVE_MODE_WAVE;
static uint8_t stepArrayPosition = 0;
static void MotorDrive_DriveCurrentStep();
MotorGPIO_Interface* gpioInterfacePtr;

void MotorDrive_Initialize(MotorGPIO_Interface* gpioInterface)
{
    if (gpioInterface == NULL) assert(false); // fatal error - no GPIO interface provided
    if (gpioInterface->SetPin_1_A == NULL) assert(false); // fatal error - missing function pointer to set pin
    if (gpioInterface->SetPin_2_A == NULL) assert(false); // fatal error - missing function pointer to set pin
    if (gpioInterface->SetPin_1_B == NULL) assert(false); // fatal error - missing function pointer to set pin
    if (gpioInterface->SetPin_2_B == NULL) assert(false); // fatal error - missing function pointer to set pin
    
    gpioInterfacePtr = gpioInterface;
}


void MotorDrive_SetDriveMode(DriveMode driveMode)
{
    activeDriveMode = driveMode;
}

void MotorDrive_DriveOneStepForward()
{
    MotorDrive_DriveCurrentStep();
    stepArrayPosition = (stepArrayPosition + 1) % STEP_ARRAY_LENGTH;
}

void MotorDrive_DriveOneStepReverse()
{
    MotorDrive_DriveCurrentStep();
    if (stepArrayPosition == 0)
    {
        stepArrayPosition = STEP_ARRAY_LENGTH;
    }
    stepArrayPosition--;
}

void MotorDrive_DriveCurrentStep()
{
    MotorDrive_Exitation step;

    switch (activeDriveMode)
    {
        case DRIVE_MODE_WAVE : 
        {
            step = stepArrayWave[stepArrayPosition];
        }
        break;
        case DRIVE_MODE_FULL_STEP : 
        {
            step = stepArrayFullStep[stepArrayPosition];
        }
        break;
        case DRIVE_MODE_HALF_STEP : 
        {
            step = stepArrayHalfStep[stepArrayPosition];
        }
        break;
    }

    gpioInterfacePtr->SetPin_1_A( step._1a );
    gpioInterfacePtr->SetPin_2_A( step._2a );
    gpioInterfacePtr->SetPin_1_B( step._1b );
    gpioInterfacePtr->SetPin_2_B( step._2b );
}

uint8_t MotorDrive_GetStepsForOneRotation(uint8_t stepsForWaveMode)
{
    if (activeDriveMode == DRIVE_MODE_WAVE) return stepsForWaveMode;
    if (activeDriveMode == DRIVE_MODE_FULL_STEP) return stepsForWaveMode;
    if (activeDriveMode == DRIVE_MODE_HALF_STEP) return stepsForWaveMode * 2;

    return 0; // unsupported drive mode
}
