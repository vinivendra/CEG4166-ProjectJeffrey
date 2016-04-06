/*
 * custom_timer.c
 *
 *
 *  Created on: Aug 8, 2015
 *      Author: shailendrasingh
 */

/****************************************************************************//*!
 * \defgroup custom_timer Module Custom Timer
 * @{
******************************************************************************/


/******************************************************************************************************************/
/* CODING STANDARDS:
 * Program file: Section I. Prologue: description about the file, description author(s), revision control
 * 				information, references, etc.
 */

/*(Doxygen help: use \brief to provide short summary and \details command can be used)*/

/*!	\file custom_timer.c
 * 	\brief This file defines and implements the custom timer functions including APIs.
 *
 * \details Defines functions such as providing tick time in microseconds, milliseconds, delay in milliseconds. This
 * module uses TIMER0 of ATMega2560 with a pre-scale of 64, and fast PWM mode.
 *
 * ATMega2560 TC (Timer/Counter) is like a clock, and can be used to measure time events. All the timers depends on
 * the system clock of system, the system clock is 16MHz for ATMega2560. Timer0: Timer0 is a 8bit timer, and is capable
 * of counting 2^8 = 256 steps from 0 to 255 (TOP). It can operate in normal mode, CTC mode or PWM mode. TIMER0 is
 * configured with pre-scaler 64 at 250 KHz with a resolution of 4μs.
 *
 * Fast PWM Mode: The fast Pulse Width Modulation or fast PWM mode provides a high frequency PWM wave-form generation
 * option. The fast PWM differs from the other PWM option by its single-slope operation. The counter counts from BOTTOM
 * to TOP then restarts from BOTTOM. In fast PWM mode, the counter is incremented until the counter value matches the TOP
 * value. The counter is then cleared at the following timer clock cycle.
 *
 * Module APIs:
 * 	- initialize_module_timer0(): initializes TIMER0 in fast PWM mode with pre-scale of 64, and activates it. Ensure this
 * 		is called at the beginning of program.
 * 	- time_in_microseconds(): returns time in microseconds.
 * 	- time_in_milliseconds(): returns time in milliseconds.
 * 	- delay_milliseconds(): accepts milliseconds and introduces required delay.
 *
 *
 * \note Ensure to initialize the module in the beginning of program.
 *
 * Usage guidelines:-
 *
 * 		=> Initialize the module by calling, initialize_module_timer0(). It should be done at
 * 			the beginning of program.
 *
 * 		=> Use time_in_microseconds() or time_in_milliseconds() to get current tick time in respective units
 * 			of microseconds and milliseconds.
 *
 * 			Example: To capture time elapsed between two events, capture time ticks using time_in_microseconds() or
 * 			time_in_milliseconds(); and the difference between two values will provide the required time elapsed value.
 *
 * 		=> Use delay_milliseconds to introduce a delay in milliseconds. Note that this function does not releases
 * 			CPU/microprocessor.
 *
 *
 *
 */


/******************************************************************************************************************/
/* CODING STANDARDS:
 * Program file: Section II. Include(s): header file includes. System include files and then user include files.
 * 				Ensure to add comments for an inclusion which is not very obvious. Suggested order of inclusion is
 * 								System -> Other Modules -> Same Module -> Specific to this file
 * Note: Avoid nested inclusions.
 */

/* --Includes-- */
/*AVR library*/
#include <avr/io.h>
#include <avr/interrupt.h>

/* module includes */
#include "custom_timer.h"		/* for module functions */


/******************************************************************************************************************/
/* CODING STANDARDS:
 * Program file: Section III. Defines and typedefs: order of appearance -> constant macros, function macros,
 * 				typedefs and then enums.
 * Naming convention: Use upper case and words joined with an underscore (_). Limit  the  use  of  abbreviations.
 * Constants: define and use constants, rather than using numerical values; it make code more readable, and easier
 * 			  to modify.
 */


#define CLEAR_BIT(sfr, bit) 											( _SFR_BYTE(sfr) &= ~_BV(bit) )									/*!<Clear a bit i.e. set as 0 in the register.*/
#define SET_BIT(sfr, bit) 												( _SFR_BYTE(sfr) |= _BV(bit) )									/*!<Set a bit i.e. set as 1 in the register.*/
#define CHECK_BIT_STATUS(variable, position) 							( (variable) & (1 << (position)) )								/*!<Check bit status in the variable for a given position.*/
#define CPU_CYCLES_IN_ONE_MICROSECOND() 								( F_CPU / 1000000L )											/*!<Compute CPU Clock Cycles in a micro-seconds.*/
#define CPU_CYCYES_TO_MICROSECONDS(cpu_cycles) 							( (cpu_cycles) / CPU_CYCLES_IN_ONE_MICROSECOND() )				/*!<Compute micro-seconds for given CPU Clock Cycles.*/
/* TIMER0 is with pre-scaler set to tick every 64 CPU cycles, and the overflow handler is called every 256 ticks.*/
#define TIME_IN_MICROSECONDS_FOR_TIMER0_OVERFLOW						( CPU_CYCYES_TO_MICROSECONDS(64 * 256) )						/*!<Overflow time in micro-seconds for TIMER0. TIMER0 is with pre-scaler set at 64, and the overflow handler is called every 256 ticks i.e. 0 to 255 (TOP).*/
/* Milliseconds per TIMER0 overflow - Integral Portion*/
#define TIME_IN_MILLISSECONDS_FOR_TIMER0_OVERFLOW_INTEGREAL 			( TIME_IN_MICROSECONDS_FOR_TIMER0_OVERFLOW / 1000 )				/*!<Milliseconds per TIMER0 overflow - Integral Portion.*/
/* Milliseconds per TIMER0 overflow - Fractional Portion. Shift right by three positions to fit into byte*/
#define TIME_IN_MILLISSECONDS_FOR_TIMER0_OVERFLOW_FRACTION 				( (TIME_IN_MICROSECONDS_FOR_TIMER0_OVERFLOW % 1000) >> 3 )		/*!<Milliseconds per TIMER0 overflow - Fractional Portion.*/
/* Milliseconds per TIMER0 overflow - Maximum Fractional Portion i.e. 1000*/
#define TIME_IN_MILLISSECONDS_FOR_TIMER0_OVERFLOW_FRACTION_MAXIMUM 		( 1000 >> 3)													/*!<Milliseconds per TIMER0 overflow - Maximum Fractional Portion i.e. 1000*/


/******************************************************************************************************************/
/* CODING STANDARDS:
 * Program file: Section IV. Global   or   external   data   declarations -> externs, non­static globals, and then
 * 				static globals.
 *
 * Naming convention: variables names must be meaningful lower case and words joined with an underscore (_). Limit
 * 					  the  use  of  abbreviations.
 * Guidelines for variable declaration:
 *				1) Do not group unrelated variables declarations even if of same data type.
 * 				2) Do not declare multiple variables in one declaration that spans lines. Start a new declaration
 * 				   on each line, in­stead.
 * 				3) Move the declaration of each local variable into the smallest scope that includes all its uses.
 * 				   This makes the program cleaner.
 */


/*!
 * \brief Data structure to capture TIMER0 timer values.
 *
 *
 * \details Holds values for counter, milliseconds integral and fractional portion.
 *
 */
struct timer_counter_parameters{
	volatile unsigned long timer0_overflow_counter;										/*!<Overflow counter*/
	volatile unsigned long timer0_time_in_milliseconds;									/*!<Milliseconds - integral portion*/
	volatile unsigned char timer0_time_fraction;										/*!<Milliseconds - fraction portion*/
};

static struct timer_counter_parameters timer_counter_timer0;							/*!<Varaible holding TIMER0 values*/


/******************************************************************************************************************/
/* CODING STANDARDS
 * Program file: Section V. Functions: order on abstraction level or usage; and if independent alphabetical
 * 				or­dering is good choice.
 *
 * 1) Declare all the functions (entry points, external functions, local functions, and ISR-interrupt service
 *    routines) before first function definition in the program file or in header file and include it; and define
 *    functions in the same order as of declaration.
 * 2) Suggested order of declaration and definition of functions is
 * 	  Entry points -> External functions -> Local functions -> ISR-Interrupt Service Routines
 * 3) Declare function names, parameters (names and types) and re­turn type in one line; if not possible fold it at
 *    an appropriate place to make it easily readable.
 * 4) No function definition should be longer than a page or screen long. If it is long, try and split it into two
 *    or more functions.
 * 5) Indentation and Spacing: this can improve the readability of the source code greatly. Tabs should be used to
 *    indent code, rather than spaces; because spaces can often be out by one and lead to confusions.
 * 6) Keep the length of source lines to 79 characters or less, for max­imum readability.
 */

/*---------------------------------------  Function Declarations  -------------------------------------------------*/
/*
 * Declare all your functions, except for entry points, for the module here; ensure to follow the same order while
 * defining them later.
 */

/*Interrupt service routine for TIMER0 overflow*/
ISR(TIMER0_OVF_vect);

/*---------------------------------------  ENTRY POINTS  ---------------------------------------------------------*/
/*define your entry points here*/

/*(Doxygen help: use \brief to provide short summary, \details for detailed description and \param for parameters */



/*!\brief Initializes the module.
 *
 *\details This procedure initializes the module, which uses TIMER0.
 *
 * \note This should be called at the beginning of the program.
 *
 *
 *
 * @return void
 *
 */
void initialize_module_timer0(void){

	sei();

	/*TIMER0 - Fast PWM Mode*/
	SET_BIT(TCCR0A, WGM01);
	SET_BIT(TCCR0A, WGM00);

	/*TIMER0 - pre-scale factor to 64*/
	SET_BIT(TCCR0B, CS01);
	SET_BIT(TCCR0B, CS00);
	//CS02- default is 0;

	/*TIMER0 -  Enable overflow interrupt*/
	SET_BIT(TIMSK0, TOIE0);

	/*Initialize data structure*/
	timer_counter_timer0.timer0_time_fraction = 0;
	timer_counter_timer0.timer0_time_in_milliseconds = 0;
	timer_counter_timer0.timer0_overflow_counter = 0;
}


/*!\brief Time ticks in microseconds.
 *
 * \details Returns time in microseconds, using TIMER0.
 *
 *
 *
 * @return void
 *
 */
unsigned long time_in_microseconds(void){
	unsigned long microseconds = 0;
	uint8_t backup_SREG = SREG, timer_counter = 0;

	cli();
	microseconds  = timer_counter_timer0.timer0_overflow_counter;
	timer_counter = TCNT0;

	if ((TIFR0 & _BV(TOV0)) && (timer_counter < 255))
		microseconds ++;

	SREG = backup_SREG;

	return ((microseconds  << 8) + timer_counter) * (64 / CPU_CYCLES_IN_ONE_MICROSECOND());

}


/*!\brief Time ticks in milliseconds.
 *
 * \details Returns time in milliseconds, using TIMER0.
 *
 *
 *
 * @return void
 *
 */
unsigned long time_in_milliseconds(void){
	unsigned long milliseconds = 0;
	uint8_t backup_SREG = SREG;

	cli();
	milliseconds  = timer_counter_timer0.timer0_time_in_milliseconds;
	SREG = backup_SREG ;

	return milliseconds;
}


/*!\brief Time delay in milliseconds.
 *
 * \details delay time in milliseconds, using TIMER0.
 *
 * \note It does not releases CPU/microprocessor.
 *
 *
 * @return void
 *
 */
void delay_milliseconds(unsigned long milliseconds){
    uint16_t start_time = (uint16_t)time_in_microseconds();

    while (milliseconds > 0) {
        if (((uint16_t)time_in_microseconds() - start_time) >= 1000) {
        	milliseconds--;
        	start_time += 1000;
        }
    }

}


/*---------------------------------------  LOCAL FUNCTIONS  ------------------------------------------------------*/
/*define your local functions here*/

/*(Doxygen help: use \brief to provide short summary, \details for detailed description and \param for parameters */


/*NO LOCAL FUNCTIONS*/


/*---------------------------------------  ISR-Interrupt Service Routines  ---------------------------------------*/
/*define your Interrupt Service Routines here*/

/*(Doxygen help: use \brief to provide short summary, \details for detailed description and \param for parameters */


/*!\brief TIMER0 overflow ISR (Interrupt Service Routine).
 *
 * \details Interrupt Service Routine, to service overflow in TIMER0. Updates \c timer_counter_timer0.
 *
 *
 *
 * @return void
 *
 */
ISR(TIMER0_OVF_vect){
	/*Copy these to local variables so they can be stored in registers*/
	/*Volatile variables must be read from memory on every access*/
	unsigned long time_milliseconds = timer_counter_timer0.timer0_time_in_milliseconds;
	unsigned char time_milliseconds_fraction = timer_counter_timer0.timer0_time_fraction;

	time_milliseconds += TIME_IN_MILLISSECONDS_FOR_TIMER0_OVERFLOW_INTEGREAL;
	time_milliseconds_fraction += TIME_IN_MILLISSECONDS_FOR_TIMER0_OVERFLOW_FRACTION;
	if (time_milliseconds_fraction >= TIME_IN_MILLISSECONDS_FOR_TIMER0_OVERFLOW_FRACTION_MAXIMUM) {
		time_milliseconds_fraction -= TIME_IN_MILLISSECONDS_FOR_TIMER0_OVERFLOW_FRACTION_MAXIMUM;
		time_milliseconds += 1;
	}

	timer_counter_timer0.timer0_time_fraction = time_milliseconds_fraction;
	timer_counter_timer0.timer0_time_in_milliseconds = time_milliseconds;
	timer_counter_timer0.timer0_overflow_counter++;
}

/*!@}*/   // end module
