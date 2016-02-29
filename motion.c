/*
 * motion.c
 *
 *  Created on: Feb 25, 2015
 *      Author: Stefan Stanisic
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "motion.h"

/******************************************************************************
 *    DEFINES
 ******************************************************************************/

/* Encoders */

#define ENC_NUMBER_OF_ENCODERS      2

#define TIFR_ICF_MASK               ((uint8_t) 0b00100000)

/* Timer/Counter */

#define TC_MODULE_4                 0
#define TC_MODULE_5                 1

#define TC_CHANNEL_A                0
#define TC_CHANNEL_B                1
#define TC_CHANNEL_C                2

#define TC_OCM_NON_INVERTED         ((uint8_t) 2)

/* Number of 500-nanosecond cycles in 20 milliseconds. */

#define TC_20_MS_PERIOD_AT_PS_8     ((uint16_t) 39999)


/******************************************************************************
 *    Configuration Structure Definitions
 *
 *    These structures store configuration register pointers and configuration
 *    data for the the MEGA2560 Timer/Counter modules.
 ******************************************************************************/

struct tc_module_registers
{
	volatile uint8_t*  TIFR_ptr;
	volatile uint8_t*  TIMSK_ptr;
	volatile uint16_t* TCNT_ptr;
	volatile uint16_t* ICR_ptr;
	volatile uint8_t*  TCCR_A_ptr;
	volatile uint8_t*  TCCR_B_ptr;
	uint8_t            prescale;
	uint8_t            wgm_mode;
	volatile uint16_t* TOP_value_p;
};

struct encoder_config_t
{
	volatile uint8_t*                 PORT_ptr;
	volatile uint8_t*                 DDR_ptr;
	uint8_t                           pin;
	const struct tc_module_registers* tc_p;
};

struct motor_registers_s
{
	volatile uint16_t*                OCR_ptr;
	volatile uint8_t*                 DDR_ptr;
	volatile uint8_t*                 PORT_ptr;
	uint8_t                           pin;
	uint8_t                           channel;
	const struct tc_module_registers* tc_p;
};


/******************************************************************************
 *    Global Data Structures
 ******************************************************************************/

/* A 32-bit variable that keeps track of the TCNT overflows. */
static volatile uint32_t tov_cntr[ENC_NUMBER_OF_ENCODERS];

/* The latest value of the Input Capture Register */
static volatile uint16_t last_icr[ENC_NUMBER_OF_ENCODERS];

static volatile uint32_t new_data[ENC_NUMBER_OF_ENCODERS];
static volatile int      new_data_available[ENC_NUMBER_OF_ENCODERS];

static const struct tc_module_registers tc_module_s[] =
{
	/* Using the Waveform Generation Mode 15 (OCRA holds the TOP value). */

	{	/* TC_MODULE_4 */
		TIFR_ptr:    &TIFR4,
		TCNT_ptr:    &TCNT4,
		ICR_ptr:     &ICR4,
		TOP_value_p: &OCR4A,
		TCCR_A_ptr:  &TCCR4A,
		TCCR_B_ptr:  &TCCR4B,
		prescale:    8,
		wgm_mode:    ((uint8_t) 15),
		TIMSK_ptr:   &TIMSK4,
	},
	{	/* TC_MODULE_5 */
		TIFR_ptr:    &TIFR5,
		TCNT_ptr:    &TCNT5,
		ICR_ptr:     &ICR5,
		TOP_value_p: &OCR5A,
		TCCR_A_ptr:  &TCCR5A,
		TCCR_B_ptr:  &TCCR5B,
		prescale:    8,
		wgm_mode:    ((uint8_t) 15),
		TIMSK_ptr:   &TIMSK5,
	}
};

static const struct encoder_config_t encoders[] =
{
	{	/* Left encoder */
		/* Port L (PL0) Digital pin 49 ( ICP4 )  */
		PORT_ptr:   &PORTL,
		DDR_ptr:    &DDRL,
		pin:        0,
		tc_p:       &tc_module_s[TC_MODULE_4]
	},
	{	/* Right encoder */
		/* Port L (PL1) Digital pin 48 ( ICP5 ) */
		PORT_ptr:   &PORTL,
		DDR_ptr:    &DDRL,
		pin:        1,
		tc_p:       &tc_module_s[TC_MODULE_5]
	}
};

static const struct motor_registers_s motors[] =
{
	{	/* Left servo
		 * PL4 ( OC5B )	Digital pin 45 (PWM)
		 *
		 */
		OCR_ptr:    &OCR5B,
		DDR_ptr:    &DDRL,
		PORT_ptr:   &PORTL,
		pin:        4,
		channel:    TC_CHANNEL_B,
		tc_p:       &tc_module_s[TC_MODULE_5]
	},
	{	/* Right servo
		 * PH5 ( OC4C )	Digital pin 8 (PWM)
		 *
		 */
		OCR_ptr:    &OCR4C,
		DDR_ptr:    &DDRH,
		PORT_ptr:   &PORTH,
		pin:        5,
		channel:    TC_CHANNEL_C,
		tc_p:       &tc_module_s[TC_MODULE_4]
	},
	{	/* Center servo
		 * PH4 ( OC4B )	Digital pin 7 (PWM)
		 *
		 */
		OCR_ptr:    &OCR4B,
		DDR_ptr:    &DDRH,
		PORT_ptr:   &PORTH,
		pin:        4,
		channel:    TC_CHANNEL_B,
		tc_p:       &tc_module_s[TC_MODULE_4]
	}
};


/******************************************************************************
 *    Local Function Prototypes
 ******************************************************************************/

static inline void enc_init(int deviceId);

static inline void tc_init(int tcModule);

static inline void tc_init_ddr(volatile uint8_t* DDR_ptr,
                               volatile uint8_t* PORT_ptr,
                               uint8_t           pin,
                               volatile uint8_t  port_value,
                               volatile uint8_t  ddr_value);

static inline void tc_set_com(volatile uint8_t* TCCR_A_ptr,
                              uint8_t           channel,
                              uint8_t           co_mode);

static inline void tc_set_wgm(volatile uint8_t* TCCR_A_ptr,
                              volatile uint8_t* TCCR_B_ptr,
                              uint8_t           wgm);

static inline void tc_set_prescaler(volatile uint8_t* TCCR_B_ptr,
                                    int               prescaler);

static inline void increment_tov_cntr(int enc_id);

static inline void handle_transition(int enc_id);


/******************************************************************************
 *    Motion module entry points
 ******************************************************************************/

/*----------------------------------------------------------------------------
 * motion_init -- Module initialization
 *
 * Call at the beginning of the program.
 *
 * This function initializes this module.
 *
 *----------------------------------------------------------------------------*/

void motion_init(void)
{
	tc_init(TC_MODULE_4);
	tc_init(TC_MODULE_5);
	enc_init(MOTION_WHEEL_LEFT);
	enc_init(MOTION_WHEEL_RIGHT);
}


/*----------------------------------------------------------------------------
 * motion_servo_start -- start a servo motor
 *
 * This function connects the control signal to the appropriate output pin.
 *
 * The control signal is an electric pulse (step function) that is repeated
 * at a rate of 20 Hz. The pulse width can be set by the user.
 *
 * Parameters:
 *   - int deviceId
 *     The valid options are:
 *         MOTION_WHEEL_LEFT
 *         MOTION_WHEEL_RIGHT
 *         MOTION_SERVO_CENTER
 *
 *
 *----------------------------------------------------------------------------*/

void motion_servo_start(int deviceId)
{
	uint16_t pulse_width_cycles = *(motors[deviceId].OCR_ptr);


	/* This will ensure that the pulse width is within an acceptable range. */

	if (!(pulse_width_cycles <= MAX_PULSE_WIDTH_TICKS &&
		  pulse_width_cycles >= MIN_PULSE_WIDTH_TICKS))
	{
		*(motors[deviceId].OCR_ptr) = INITIAL_PULSE_WIDTH_TICKS;
	}


	/* The OCnX pin needs to be an output.
	 * Initialize the Data Direction Register of the OCnX pin.
	 */

	tc_init_ddr(motors[deviceId].DDR_ptr,
	            motors[deviceId].PORT_ptr,
	            motors[deviceId].pin,
	            0,          /* make the output 0 (0V) */
	            1);         /* pin mode: OUTPUT */


	/* Set the Compare Output Mode to non-inverted. */

	tc_set_com(motors[deviceId].tc_p->TCCR_A_ptr,
	           motors[deviceId].channel,
	           TC_OCM_NON_INVERTED);
}


/*----------------------------------------------------------------------------
 * motion_servo_stop -- stop a servo motor
 *
 * This function disconnects the control signal from the output pin.
 *
 * Instead of the control signal, a constant value of 0V is applied to the
 * output. This turns the servo off.
 *
 * Parameters:
 *   - int deviceId
 *     The valid options are:
 *         MOTION_WHEEL_LEFT
 *         MOTION_WHEEL_RIGHT
 *         MOTION_SERVO_CENTER
 *
 *----------------------------------------------------------------------------*/

void motion_servo_stop(int deviceId)
{
	/* Set the pin as an output of 0V. */

	tc_init_ddr(motors[deviceId].DDR_ptr,
	            motors[deviceId].PORT_ptr,
	            motors[deviceId].pin, 0, 1);


	/* Disconnect the output pin from the Timer/Counter module. */

	tc_set_com(motors[deviceId].tc_p->TCCR_A_ptr,
	           motors[deviceId].channel,
	           0b00);
}


/* ---------------------------------------------------------------------------
 * motion_servo_set_pulse_width -- set the pulse width length
 *
 *
 * Parameters:
 *   - int deviceId
 *     The valid options are:
 *         MOTION_WHEEL_LEFT
 *         MOTION_WHEEL_RIGHT
 *         MOTION_SERVO_CENTER
 *
 *   - uint16_t ticks
 *     The pulse width length in ticks.
 *
 * Sets the pulse width by changing the value of the Output Compare Register.
 *----------------------------------------------------------------------------*/

void motion_servo_set_pulse_width(int deviceId, uint16_t pulse_width_cycles)
{
	if (pulse_width_cycles <= MAX_PULSE_WIDTH_TICKS &&
		pulse_width_cycles >= MIN_PULSE_WIDTH_TICKS)
	{
		*(motors[deviceId].OCR_ptr) = pulse_width_cycles;
	}
}


/*----------------------------------------------------------------------------
 * motion_servo_get_pulse_width -- get the pulse width length
 *
 * Parameters:
 *   - int deviceId
 *     The valid options are:
 *         MOTION_WHEEL_LEFT
 *         MOTION_WHEEL_RIGHT
 *         MOTION_SERVO_CENTER
 *
 * Return value:
 *   The current pulse width length (in ticks).
 *
 * Returns the pulse width by reading the value of the Output Compare Register.
 *----------------------------------------------------------------------------*/

uint16_t motion_servo_get_pulse_width(int deviceId)
{
    return *(motors[deviceId].OCR_ptr);
}


/*----------------------------------------------------------------------------
 * motion_enc_read -- poll an optical encoder
 *
 * Parameter:
 *   - int deviceId
 *     The valid options are:
 *         MOTION_WHEEL_LEFT
 *         MOTION_WHEEL_RIGHT
 *
 *   - uint32_t* val
 *     Provide the pointer to the variable to which the time elapsed
 *     (in ticks) between the most recent and the second most
 *     recent input capture events will be written. Nothing is written
 *     if no new data is available.
 *
 * Return value:
 *   - 0 if no new data is available. Otherwise, 1.
 *
 *----------------------------------------------------------------------------*/

int motion_enc_read(int deviceId, uint32_t *val)
{
	int retVal = 0;

	cli(); /* mask interrupts */

	/* If new data is available */

	if (new_data_available[deviceId] == 1)
	{
		*val = new_data[deviceId];
		new_data_available[deviceId] = 0;
		retVal = 1;
	}

	sei(); /* unmask interrupts */

	return retVal;
}


/******************************************************************************
 *    Motion Module Local Functions
 ******************************************************************************/

/*----------------------------------------------------------------------------
 *    TIMER/COUNTER CONFIGURATION FUNCTIONS
 *----------------------------------------------------------------------------*/

/* tc_init -- Initialize a Timer/Counter module
 *
 * This function will configure the following Timer/Counter module parameters:
 *   - Output Compare Mode (OCM)
 *   - Waveform Generation Mode (WGM)
 *   - prescaler value
 *   - TOP value
 *
 * Parameter:
 *   - timerId
 *     The Timer/Counter module identifier (index into tc_module_s array).
 */

static inline void tc_init(int timerId)
{
	/* Set the OCM to 0 for the three channels of the TC module.
	 * As result, the pin associated to each channel will be
	 * disconnected from the TC module.
	 */

	tc_set_com(tc_module_s[timerId].TCCR_A_ptr,
	           TC_CHANNEL_A,
	           0b00);

	tc_set_com(tc_module_s[timerId].TCCR_A_ptr,
	           TC_CHANNEL_B,
	           0b00);

	tc_set_com(tc_module_s[timerId].TCCR_A_ptr,
	           TC_CHANNEL_C,
	           0b00);

	tc_set_wgm(tc_module_s[timerId].TCCR_A_ptr,
	           tc_module_s[timerId].TCCR_B_ptr,
	           tc_module_s[timerId].wgm_mode);

	tc_set_prescaler(tc_module_s[timerId].TCCR_B_ptr,
	                 tc_module_s[timerId].prescale);


	/* This value determines the rate at which the pulses are sent.
	 * Store the value in the Input Capture Register
	 */

	*tc_module_s[timerId].TOP_value_p = TC_20_MS_PERIOD_AT_PS_8 - 1;
}


/* tc_init_ddr -- Set a pin as an input or as an output
 *
 * Parameters:
 *  - DDR_ptr
 *      A pointer to the Data Direction Register associated with the pin.
 *
 *  - PORT_ptr
 *      A pointer to the PORT Register associated with the pin.
 *
 *  - pin
 *      The bit that corresponds to the pin in the PORT Register.
 *      ---> The expected value is an integer between 0 and 7.
 *
 *  - port_value
 *      The value that will be used to initialize the PORT register.
 *
 *  - ddr_value
 *      The value that will be used to initialize the DDR register.
 */

static inline void tc_init_ddr(volatile uint8_t* DDR_ptr,
                               volatile uint8_t* PORT_ptr,
                               uint8_t pin,
                               volatile uint8_t port_value,
                               volatile uint8_t ddr_value)
{
	if (ddr_value == 1)
	{
		*DDR_ptr |= (1<<pin);	/* write '1' (OUTPUT) */
	}
	else
	{
		*DDR_ptr &= ~(1<<pin);	/* write '0' (INPUT) */
	}

	if (port_value == 0)
	{
		*PORT_ptr &= ~(1<<pin);	/* write '0' */
	}
	else
	{
		*PORT_ptr |= 1<<pin;	/* write '1' */
	}
}


/* tc_set_com -- Set the Compare Output Mode for channel A, B, or C.
 *
 * Parameters:
 *  - TCCR_A_ptr
 *      A pointer to the TC module's configuration register A.
 *
 *  - channel
 *      The TC channel for which the compare output mode is set.
 *
 *  - co_mode
 *      The desired compare output mode of the channel.
 *
 */

static inline void tc_set_com(volatile uint8_t* TCCR_A_ptr,
                              uint8_t channel,
                              uint8_t co_mode)
{
	uint8_t	tccra;
	uint8_t	co_mode_mask = 0b00000011;

	/* read the value of TCCRnA */

	tccra = *TCCR_A_ptr;

	switch(channel)
	{
	case TC_CHANNEL_A:
		co_mode = co_mode << 6;
		co_mode_mask = co_mode_mask << 6;
		break;

	case TC_CHANNEL_B:
		co_mode = co_mode << 4;
		co_mode_mask = co_mode_mask << 4;
		break;

	case TC_CHANNEL_C:
		co_mode = co_mode << 2;
		co_mode_mask = co_mode_mask << 2;
		break;

	default:
		return;
		break;
	}

	co_mode &= co_mode_mask;

	tccra |= co_mode;

	co_mode |= ~co_mode_mask;

	tccra &= co_mode;

	/* write the value to TCCRnB */

	*TCCR_A_ptr = tccra;
}


/* tc_set_wgm -- Set the Waveform Generation Mode
 *
 * Parameters:
 *  - TCCR_A_ptr
 *      A pointer to the TC module's configuration register A.
 *
 *  - TCCR_B_ptr
 *      A pointer to the configuration register B.
 *
 *  - wgm
 *      The desired waveform generation mode.
 *
 */

static inline void tc_set_wgm(volatile uint8_t* TCCR_A_ptr,
                              volatile uint8_t* TCCR_B_ptr,
                              uint8_t wgm)
{
	uint8_t wgm_bit3_bit2 = wgm & 0b00001100;
	uint8_t wgm_bit1_bit0 = wgm & 0b00000011;

	wgm_bit3_bit2 = wgm_bit3_bit2 << 1;

	uint8_t tccra = *TCCR_A_ptr;
	uint8_t tccrb = *TCCR_B_ptr;

	/* set WGMn1 and WGMn0 in TCCRnA*/
	tccra |= wgm_bit1_bit0;
	tccra &= (0b11111100 | wgm_bit1_bit0);

	/* set WGMn3 and WGMn2 in TCCRnB */
	tccrb |= wgm_bit3_bit2;
	tccrb &= (0b11100111 | wgm_bit3_bit2);

	*TCCR_A_ptr = tccra;
	*TCCR_B_ptr = tccrb;
}


/* tc_set_prescaler -- Set the prescaler
 *
 * Parameters:
 *  - TCCR_B_ptr
 *      A pointer to the TC module's configuration register B.
 *
 *  - prescaler
 *      The desired clock frequency divider.
 *
 */

static inline void tc_set_prescaler(volatile uint8_t* TCCR_B_ptr,
                                    int prescaler)
{
	/* copy the value contained in the register */

	uint8_t tccrb = *TCCR_B_ptr;

	switch(prescaler)
	{
	case 1:     // 0b001
		tccrb |= 0b00000001;
		tccrb &= 0b11111001;
		break;

	case 8:     // 0b010

default_case:   /* DEFAULT */

		tccrb |= 0b00000010;
		tccrb &= 0b11111010;
		break;

	case 64:    // 0b011
		tccrb |= 0b00000011;
		tccrb &= 0b11111011;
		break;

	case 256:    // 0b100
		tccrb |= 0b00000100;
		tccrb &= 0b11111100;
		break;

	case 1024:   // 0b101
		tccrb |= 0b00000101;
		tccrb &= 0b11111101;
		break;

	default:
		goto default_case; /* GOTO DEFAULT */
		break;
	}

	/* write the new value to the configuration register */

	*TCCR_B_ptr = tccrb;
}


/*----------------------------------------------------------------------------
*  ENCODER CONFIGURATION FUNCTIONS
 *----------------------------------------------------------------------------*/

static inline void enc_init(int enc_id)
{

	/* initialize global variables */

	last_icr[enc_id] = 0;
	tov_cntr[enc_id] = 0;


	/* The Input Capture Pins are INPUTS. This call will
	 * set the Data Direction Registers accordingly.
	 */

	tc_init_ddr(encoders[enc_id].DDR_ptr,
	            encoders[enc_id].PORT_ptr,
	            encoders[enc_id].pin,
	            0,      /* no pull-up resistor */
	            0);     /* INPUT */


	/* Input Capture Interrupt Enable */

	*encoders[enc_id].tc_p->TIMSK_ptr  |= (1 << 5);


	/* Timer Overflow Interrupt Enable */

	*encoders[enc_id].tc_p->TIMSK_ptr  |= (1 << 0);


	/* rising edge */ /* falling edge: &= ~(1 << 6) */

	*encoders[enc_id].tc_p->TCCR_B_ptr |= 1 << 6;
}


/*----------------------------------------------------------------------------
 *    INTERRUPT SERVICE ROUTINES
 *----------------------------------------------------------------------------*/

ISR(TIMER4_OVF_vect)
{
    increment_tov_cntr(MOTION_WHEEL_LEFT);
}

ISR(TIMER5_OVF_vect)
{
    increment_tov_cntr(MOTION_WHEEL_RIGHT);
}

ISR(TIMER4_CAPT_vect)
{
    handle_transition(MOTION_WHEEL_LEFT);
}

ISR(TIMER5_CAPT_vect)
{
    handle_transition(MOTION_WHEEL_RIGHT);
}

static inline void increment_tov_cntr(int enc_id)
{
    tov_cntr[enc_id]++;
}

static inline void handle_transition(int enc_id)
{
	uint32_t delta;
	uint16_t icr;
	uint32_t tov;

	/* clear the Input Capture Flag */

	*encoders[enc_id].tc_p->TIFR_ptr |= (1 << 5);

	icr = *encoders[enc_id].tc_p->ICR_ptr;
	tov =  tov_cntr[enc_id];

	tov_cntr[enc_id] = 0;

	if (tov == 0)
	{
		delta = icr - last_icr[enc_id];
	}
	else
	{
		delta = icr + tov*TC_20_MS_PERIOD_AT_PS_8 - last_icr[enc_id];
	}

	new_data[enc_id] = delta;

	new_data_available[enc_id] = 1;

	last_icr[enc_id] = icr;
}
