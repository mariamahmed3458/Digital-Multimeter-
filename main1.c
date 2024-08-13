#include <avr/io.h>
#include <math.h>
#include "adc.h"
#include "gpio.h"
#include "lcd.h"
#include "keypad.h"
#include <util/delay.h>
#include "common_macros.h"

/*******************************************************************************
 *                        Definitions and Type Definitions                     *
 *******************************************************************************/

typedef enum {
    DC_VOLTMETER,
    AC_VOLTMETER,
    DC_AMMETER,
    AC_AMMETER,
    OHMMETER
} UsedOperation;

typedef enum {
    RESISTOR10k = 2,
    RESISTOR100k = 3,
} OhmmeterResistorNumber;

#define MAX_ANALOG_VALUE 1023.0
#define NUM_REF_RESISTORS 5
#define EXIT_OPERATION '='

/*******************************************************************************
 *                                Global Variables                             *
 *******************************************************************************/

uint8 exit_check = 0;
const uint32 resistor_table[NUM_REF_RESISTORS] = {100, 1000, 10000, 100000, 2000000};
uint32 resistor;

/*******************************************************************************
 *                                Function Prototypes                          *
 *******************************************************************************/

uint16 ACVOLTMETER_get_max(void);
uint16 DCVOLTMETER_get_max(void);
void ACAMMETER_run(void);
void DCAMMETER_run(void);
void DCVOLTMETER_run(void);
void ACVOLTMETER_run(void);
void OHMMETER_channelSelect(OhmmeterResistorNumber n);
void OHMMERTER_run(void);

/*******************************************************************************
 *                                   Functions                                 *
 *******************************************************************************/

/**
 * @brief Get the maximum ADC value for AC voltage measurement.
 *
 * This function samples the ADC multiple times and returns the maximum value
 * read, which represents the peak AC voltage.
 *
 * @return uint16 The maximum ADC value read.
 */
uint16 ACVOLTMETER_get_max(void) {
    uint16 max_v = 0;
    for (uint16 i = 0; i < 250; i++) {
        uint16 r;
        while (!ADC_readChannel(&r)) {}
        if (max_v < r) max_v = r;
        _delay_us(20);
    }
    return max_v;
}

/**
 * @brief Get the maximum ADC value for DC voltage measurement.
 *
 * This function samples the ADC multiple times and returns the maximum value
 * read, which represents the DC voltage.
 *
 * @return uint16 The maximum ADC value read.
 */
uint16 DCVOLTMETER_get_max(void) {
    uint16 max_v = 0;
    for (uint16 i = 0; i < 250; i++) {
        uint16 r;
        while (!ADC_readChannel(&r)) {}
        if (max_v < r) max_v = r;
        _delay_us(20);
    }
    return max_v;
}

/**
 * @brief Run the AC ammeter functionality.
 *
 * This function initializes the ADC, continuously reads the peak AC voltage,
 * converts it to current using a predefined formula, and displays the result
 * on an LCD. The function exits when the exit key is pressed.
 *
 * Measures:
 *
 *	from 2 mAMP -> 350 mAMP
 *
 */
void ACAMMETER_run(void) {
    uint16 dcc = 0;
    uint16 analog = 0;
    float32 vdiff = 0;

    ADC_init(3);
    LCD_displayString("AC Current:   ");

    while (1) {
        analog = ACVOLTMETER_get_max();
        vdiff = (analog * 5000.0) / 1023.0;
        vdiff *= 3;
        dcc = vdiff / 30;
        LCD_moveCursor(1, 0);
        if (dcc > 1000) {
            LCD_intgerToString(dcc / 1000);
            LCD_displayString(".");
            LCD_intgerToString(dcc % 1000);
            LCD_displayString(" Amp");
        } else {
            LCD_intgerToString(dcc);
            LCD_displayString(" mAmp");
        }
        KEYPAD_getPressedKeyInterrupts(&exit_check);
        if (exit_check == EXIT_OPERATION) {
            LCD_clearScreen();
            exit_check = 'a';
            break;
        }
    }
}

/**
 * @brief Run the DC ammeter functionality.
 *
 * This function initializes the ADC, continuously reads the peak DC voltage,
 * converts it to current using a predefined formula, and displays the result
 * on an LCD. The function exits when the exit key is pressed.
 *
 * Measures:
 *
 *	from 2 mAMP -> 350 mAMP
 */
void DCAMMETER_run(void) {
    uint16 dcc = 0;
    uint16 analog = 0;
    float32 vdiff = 0;

    ADC_init(3);
    LCD_displayString("DC Current:   ");

    while (1) {
        analog = DCVOLTMETER_get_max();
        vdiff = (analog * 5000.0) / 1023.0;
        vdiff *= 3;
        dcc = vdiff / 30;
        LCD_moveCursor(1, 0);
        if (dcc > 1000) {
            LCD_intgerToString(dcc / 1000);
            LCD_displayString(".");
            LCD_intgerToString(dcc % 1000);
            LCD_displayString(" Amp");
        } else {
            LCD_intgerToString(dcc);
            LCD_displayString(" mAmp");
        }
        KEYPAD_getPressedKeyInterrupts(&exit_check);
        if (exit_check == EXIT_OPERATION) {
            LCD_clearScreen();
            exit_check = 'a';
            break;
        }
    }
}

/**
 * @brief Run the DC voltmeter functionality.
 *
 * This function initializes the ADC, continuously reads the DC voltage, converts
 * it to a readable format, and displays the result on an LCD. The function
 * exits when the exit key is pressed.
 *
 * Measures:
 *
 * from 27 mV -> 55 V
 */
void DCVOLTMETER_run(void) {
    float32 input_volt = 0.0;
    float32 temp = 0.0;
    float32 r1 = 10000.0;    // resistance value
    float32 r2 = 100000.0;   // resistance value
    uint16 milli_volt = 0;
    float32 v = 0;
    int flag = 0;
    ADC_init(1);
    LCD_displayString("DC Voltage:           ");

    while (1) {
        v = DCVOLTMETER_get_max();
        if (v < 100) {
            flag = 1;
            SET_BIT(ADMUX, 6);
            SET_BIT(ADMUX, 7);  // Use internal voltage reference
            v = DCVOLTMETER_get_max();
            temp = (v * 2.56) / 1023.0;
            input_volt = temp / (r2 / (r1 + r2));
            milli_volt = input_volt * 1000;
        } else {
            flag = 0;
            if (v > 725) v -= 1;
            temp = (v * 5.0) / 1023.0;
            input_volt = temp / (r2 / (r1 + r2));
            milli_volt = input_volt * 1000;
        }

        LCD_moveCursor(1, 0);
        if (milli_volt > 1000) {
            LCD_intgerToString(milli_volt / 1000);
            LCD_displayString(".");
            LCD_intgerToString(milli_volt % 1000);
            LCD_displayString(" V             ");
        } else {
            LCD_intgerToString(milli_volt);
            LCD_displayString(" mV            ");
        }

        KEYPAD_getPressedKeyInterrupts(&exit_check);
        if (exit_check == EXIT_OPERATION) {
            LCD_clearScreen();
            exit_check = 'a';
            break;
        }
    }
}

/**

 *
 * @brief Run the AC voltmeter functionality.
 *
 * This function initializes the ADC, continuously reads the AC voltage, converts
 * it to a readable format, and displays the result on an LCD. The function
 * exits when the exit key is pressed.
 *
 * Measures:
 *
 * 20 mV -> 25 V (P2P)
 *
 */
void ACVOLTMETER_run(void) {
    float32 input_volt = 0.0;
    float32 temp = 0.0;
    float32 r1 = 10000.0;    // resistance value
    float32 r2 = 100000.0;   // resistance value
    uint16 milli_volt = 0;
    float32 v = 0;
    int flag = 0;
    ADC_init(2);
    LCD_displayString("AC Voltage:           ");

    while (1) {
        v = DCVOLTMETER_get_max();
        if (v < 100) {
            flag = 1;
            SET_BIT(ADMUX, 6);
            SET_BIT(ADMUX, 7);  // Use internal voltage reference
            v = DCVOLTMETER_get_max();
            temp = (v * 2.56) / 1023.0;
            input_volt = temp / (r2 / (r1 + r2));
            milli_volt = input_volt * 1000;
        } else {
            flag = 0;
            if (v > 725) v -= 1;
            temp = (v * 5.0) / 1023.0;
            input_volt = temp / (r2 / (r1 + r2));
            milli_volt = input_volt * 1000;
        }

        LCD_moveCursor(1, 0);
        if (milli_volt > 1000) {
            LCD_intgerToString(milli_volt / 1000);
            LCD_displayString(".");
            LCD_intgerToString(milli_volt % 1000);
            LCD_displayString(" V             ");
        } else {
            LCD_intgerToString(milli_volt);
            LCD_displayString(" mV            ");
        }

        KEYPAD_getPressedKeyInterrupts(&exit_check);
        if (exit_check == EXIT_OPERATION) {
            LCD_clearScreen();
            exit_check = 'a';
            break;
        }
    }
}

/**
 * @brief Select the appropriate reference resistor for the ohmmeter.
 *
 * This function sets the GPIO pins to select one of the predefined reference
 * resistors based on the given resistor number.
 *
 * @param n The resistor number to select.
 */
void OHMMETER_channelSelect(OhmmeterResistorNumber n) {
    switch (n) {
    case RESISTOR10k:
        GPIO_writePin(PORTD_ID, PIN6_ID, LOGIC_LOW);
        GPIO_writePin(PORTD_ID, PIN7_ID, LOGIC_HIGH);
        GPIO_writePin(PORTA_ID, PIN6_ID, LOGIC_LOW);
        break;
    case RESISTOR100k:
        GPIO_writePin(PORTD_ID, PIN6_ID, LOGIC_HIGH);
        GPIO_writePin(PORTD_ID, PIN7_ID, LOGIC_HIGH);
        GPIO_writePin(PORTA_ID, PIN6_ID, LOGIC_LOW);
        break;
    default:
        GPIO_writePin(PORTD_ID, PIN6_ID, LOGIC_LOW);
        GPIO_writePin(PORTD_ID, PIN7_ID, LOGIC_LOW);
        GPIO_writePin(PORTA_ID, PIN6_ID, LOGIC_HIGH);
        break;
    }
    resistor = resistor_table[n];
}

/**
 * @brief Run the ohmmeter functionality.
 *
 * This function initializes the GPIO and ADC, continuously reads the voltage
 * across the unknown resistor, calculates its resistance, and displays the
 * result on an LCD. The function exits when the exit key is pressed.
 *
 * Measures:
 *
 * from 10 ohm -> 1M, and  from 1M -> 2M with error
 */
void OHMMERTER_run(void) {
    uint16 volt_image;
    uint8 ch_number = RESISTOR100k;

    GPIO_setupPinDirection(PORTD_ID, PIN6_ID, PIN_OUTPUT);
    GPIO_setupPinDirection(PORTD_ID, PIN7_ID, PIN_OUTPUT);
    GPIO_setupPinDirection(PORTA_ID, PIN6_ID, PIN_OUTPUT);
    LCD_init();
    ADC_init(5);
    LCD_displayString("Resistance exp:           ");
    OHMMETER_channelSelect(ch_number);

    while (1) {
        while (!ADC_readChannel(&volt_image)) {}
        _delay_ms(100);

        if (volt_image <= 95) {
            ch_number = RESISTOR10k;
            OHMMETER_channelSelect(ch_number);
            _delay_ms(50);
            while (!ADC_readChannel(&volt_image)) {}
            _delay_ms(50);
        }

        LCD_moveCursor(1, 15);
        LCD_intgerToString(ch_number);
        LCD_moveCursor(1, 0);

        uint32 value = volt_image * (resistor + 330) / (1023 - volt_image);
        if (value > 1000) {
            LCD_intgerToString(value / 1000);
            LCD_displayString(".");
            LCD_intgerToString(value % 1000);
            LCD_displayString(" kohm          ");
        } else {
            LCD_intgerToString(value % 1000);
            LCD_displayString(" ohm          ");
        }

        KEYPAD_getPressedKeyInterrupts(&exit_check);
        if (exit_check == EXIT_OPERATION) {
            LCD_clearScreen();
            exit_check = 'a';
            break;
        }
    }
}

/*******************************************************************************
 *                                    main                                     *
 *******************************************************************************/

/**
 * @brief Main function to run the multimeter operations.
 *
 * This function initializes the system and continuously displays a menu for
 * the user to choose a measurement operation (DC voltmeter, AC voltmeter, DC
 * ammeter, AC ammeter, or ohmmeter). It executes the chosen operation and
 * allows the user to exit back to the menu.
 *
 * @return int Exit status (not used in this embedded application).
 */
int main(void) {
    SREG |= (1 << 7);  // Enable global interrupts
    LCD_init();
    UsedOperation used_case;

    while (1) {
        if (exit_check == 'a') {
            LCD_clearScreen();
            LCD_moveCursor(0, 0);
            exit_check = 0;
        }
        LCD_displayStringRowColumn(0, 0, "choose operation");
        used_case = KEYPAD_getPressedKeyPolling();

        switch (used_case) {
        case DC_VOLTMETER:
            LCD_clearScreen();
            LCD_moveCursor(0, 0);
            DCVOLTMETER_run();
            break;
        case AC_VOLTMETER:
            LCD_clearScreen();
            LCD_moveCursor(0, 0);
            ACVOLTMETER_run();
            break;
        case DC_AMMETER:
            LCD_clearScreen();
            LCD_moveCursor(0, 0);
            DCAMMETER_run();
            break;
        case AC_AMMETER:
            LCD_clearScreen();
            LCD_moveCursor(0, 0);
            ACAMMETER_run();
            break;
        case OHMMETER:
            LCD_clearScreen();
            LCD_moveCursor(0, 0);
            OHMMERTER_run();
            break;
        default:
            break;
        }
    }
}
