#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/sync.h"
#include "hardware/uart.h"
#include <stdint.h>
#include "MotorDrive.h"
#include "MotorGPIO.h"

// UART defines
// `uart0` is the one connected to the USB on the Pico board
#define UART_ID uart0
#define BAUD_RATE 115200

// Use pins 0 and 1 for UART0
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 0
#define UART_RX_PIN 1

// UART interrupt handler prototype
void handleUartIqr(void);

// define gpio pins for the stepper motor
// colors are for the SP2575M0206-A stepper motor
static const uint8_t STEP_PIN_1_A = 10; // white
static const uint8_t STEP_PIN_2_A = 11; // blue
static const uint8_t STEP_PIN_1_B = 12; // red
static const uint8_t STEP_PIN_2_B = 13; // yellow

// Define number of steps for a full rotation in wave mode
// according to the datasheet, the step angle is 7.5 degrees
// i.e. 360 / 7.5 = 48 steps per rotation
static const uint8_t STEPS_PER_ROTATION_WAVE = 48;

#define TIMER_INTERVAL_US 1000 // 1 ms

// Timer callback prototype
int64_t handleTimerIrq(alarm_id_t id, void *user_data);

// function prototypes
void printUsage();
void initializeUart();

void handleByteReceived(uint8_t byteReceived);
void decreaseSpeed();
void increaseSpeed();
void driveForwards();
void driveBackwards();
void stop();
void doOneStateLoop();

const uint16_t INITIAL_SOFTWARE_LOOP_TIME_IN_MILLIS = 100;
const uint16_t SPEED_STEP_SIZE = 5;

volatile uint16_t softwareLoopTimeInMillis = INITIAL_SOFTWARE_LOOP_TIME_IN_MILLIS;
volatile uint16_t loopCounterInMillis = INITIAL_SOFTWARE_LOOP_TIME_IN_MILLIS;;
volatile uint8_t stepsRemainingForOneRotation = 0;

typedef enum
{
    DIRECTION_FORWARD,
    DIRECTION_REVERSE
} Direction;

typedef enum
{
    ROTATION_MODE_SINGLE,
    ROTATION_MODE_CONTINUOUS
} RotationMode;

typedef enum
{
    RUNNING,
    STOPPED
} RunMode;
    
volatile Direction activeDirection = DIRECTION_FORWARD; // used in both UART and timer IRQs, therefore volatile
volatile RotationMode activeRotationMode = ROTATION_MODE_CONTINUOUS; // used in both UART and timer IRQs, therefore volatile
volatile RunMode activeRunMode = STOPPED; // used in both UART and timer IRQs, therefore volatile

int main()
{
    stdio_init_all();

    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    // Example to turn on the Pico W LED
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    MotorGPIO_InitializePins(STEP_PIN_1_A, STEP_PIN_2_A, STEP_PIN_1_B, STEP_PIN_2_B);
    MotorGPIO_Interface* gpioInterface = MotorGPIO_GetInterface();
    MotorDrive_Initialize(gpioInterface);

    initializeUart();

    // configure and start a timer interrupt for every 1 ms
    uint32_t alarmId = add_alarm_in_us(TIMER_INTERVAL_US, &handleTimerIrq, NULL, true); // 1000 ms
    hard_assert(alarmId > 0); // if alarm id is negative, the configuration failed
    
    printUsage();

    for(;;)
    {
        // everything is handled in interrupts
        tight_loop_contents();
    }
}

void printUsage()
{
    uart_puts(UART_ID, "Step motor application started\r\n");
    uart_puts(UART_ID, "0: Stop\r\n");
    uart_puts(UART_ID, "1: Drive forwards\r\n");
    uart_puts(UART_ID, "2: Drive reverse\r\n");
    uart_puts(UART_ID, "3: One rotation forwards\r\n");
    uart_puts(UART_ID, "4: One rotation reverse\r\n");

    uart_puts(UART_ID, "a: Select wave drive\r\n");
    uart_puts(UART_ID, "s: Select full step drive\r\n");
    uart_puts(UART_ID, "d: Select half step drive\r\n");

    uart_puts(UART_ID, "q: Decrease speed\r\n");
    uart_puts(UART_ID, "w: Increase speed\r\n");
    uart_puts(UART_ID, "p: Print usage\r\n");
}

void initializeUart()
{
    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Send out a string, with CR/LF conversions
    uart_puts(UART_ID, " Hello, UART!\n");

    // configure UART interrupts
    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, handleUartIqr);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);
}

void handleUartIqr()
{
    // read the received byte until there are no more bytes
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = uart_getc(UART_ID);
        // Echo if possible
        if (uart_is_writable(UART_ID)) {
            uart_putc(UART_ID, ch);
        }
        // handle the received byte
        handleByteReceived(ch);
    }
}

int64_t handleTimerIrq(alarm_id_t id, void *user_data)
{
    if (activeRunMode == RUNNING)
    {
        // do one loop of the current state
        loopCounterInMillis--;
        if (loopCounterInMillis == 0)
        {
            loopCounterInMillis = softwareLoopTimeInMillis;
            if (activeRotationMode == ROTATION_MODE_SINGLE
                && stepsRemainingForOneRotation > 0)
            {
                stepsRemainingForOneRotation--;
                doOneStateLoop();
            }
            else if (activeRotationMode == ROTATION_MODE_CONTINUOUS)
            {
                doOneStateLoop();
            }
        }
    }
    // reschedule after TIMER_INTERVAL_US from the time the alarm was previously scheduled to fire
    return -TIMER_INTERVAL_US; 
}

void handleByteReceived(uint8_t byteReceived)
{
    switch(byteReceived)
    {
        case 'q' :
        {
            decreaseSpeed();
        }
        break;
        case 'w' :
        {
            increaseSpeed();
        }
        break;
        case '1' :
        {
            activeRotationMode = ROTATION_MODE_CONTINUOUS;
            driveForwards();
        }
        break;
        case '2' :
        {
            activeRotationMode = ROTATION_MODE_CONTINUOUS;
            driveBackwards();
        }
        break;
        case '3' :
        {
            uart_puts(UART_ID, "Single rotation\r\n");
            uint32_t saved = save_and_disable_interrupts();
            activeRotationMode = ROTATION_MODE_SINGLE;
            stepsRemainingForOneRotation = MotorDrive_GetStepsForOneRotation(STEPS_PER_ROTATION_WAVE);
            restore_interrupts(saved);
            driveForwards();
        }
        break;
        case '4' :
        {
            uart_puts(UART_ID, "Single rotation\r\n");
            uint32_t saved = save_and_disable_interrupts();
            activeRotationMode = ROTATION_MODE_SINGLE;
            stepsRemainingForOneRotation = MotorDrive_GetStepsForOneRotation(STEPS_PER_ROTATION_WAVE);
            restore_interrupts(saved);
            driveBackwards();
        }
        break;
        case '0' :
        {
            stop();
        }
        break;
        case 'a' :
        {
            uart_puts(UART_ID, "Drive mode: wave\r\n");
            MotorDrive_SetDriveMode(DRIVE_MODE_WAVE);
        }
        break;
        case 's' :
        {
            uart_puts(UART_ID, "Drive mode: full step\r\n");
            MotorDrive_SetDriveMode(DRIVE_MODE_FULL_STEP);
        }
        break;
        case 'd' :
        {
            uart_puts(UART_ID, "Drive mode: half step\r\n");
            MotorDrive_SetDriveMode(DRIVE_MODE_HALF_STEP);
        }
        break;
        case 'p' :
        {
            printUsage();
        }
        break;
        default :
        {
            // nothing
        }
        break;
    }
}

void doOneStateLoop()
{
    if (activeDirection == DIRECTION_FORWARD)
    {
        MotorDrive_DriveOneStepForward();
    }
    else if (activeDirection == DIRECTION_REVERSE)
    {
        MotorDrive_DriveOneStepReverse();
    }
}

void decreaseSpeed()
{
    softwareLoopTimeInMillis += SPEED_STEP_SIZE;
    printf("Decreasing speed - waiting %d millis between steps\r\n", softwareLoopTimeInMillis);
}

void increaseSpeed()
{
    if (softwareLoopTimeInMillis > SPEED_STEP_SIZE)
    {
        softwareLoopTimeInMillis = softwareLoopTimeInMillis - SPEED_STEP_SIZE;
    }
    printf("Increasing speed - waiting %d millis between steps\r\n", softwareLoopTimeInMillis);
}

void driveForwards()
{
    uart_puts(UART_ID, "Set direction: forwards\r\n");
    activeDirection = DIRECTION_FORWARD;
    activeRunMode = RUNNING;
}

void driveBackwards()
{
    uart_puts(UART_ID, "Set direction: reverse\r\n");
    activeDirection = DIRECTION_REVERSE;
    activeRunMode = RUNNING;
}

void stop()
{
    uart_puts(UART_ID, "Stop\r\n");
    activeRunMode = STOPPED;
}
