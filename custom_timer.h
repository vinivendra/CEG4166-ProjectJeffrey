/*
 * custom_timer.h
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
/* CODING STANDARDS
 * Header file: Section I. Prologue: description about the file, description author(s), revision control
 * 				information, references, etc.
 * Note: 1. Header files should be functionally organized.
 *		 2. Declarations   for   separate   subsystems   should   be   in   separate
 */

/*(Doxygen help: use \brief to provide short summary and \details command can be used)*/

/*!\file custom_timer.h
 * 	\brief This file declares the custom timer API functions.
 *
 * \details Functions such as providing tick time in microseconds, milliseconds, delay in milliseconds. This
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
 */


#ifndef INCLUDE_CUSTOM_TIMER_H_
#define INCLUDE_CUSTOM_TIMER_H_


/******************************************************************************************************************/
/* CODING STANDARDS:
 * Header file: Section II. Include(s): header file includes. System include files and then user include files.
 * 				Ensure to add comments for an inclusion which is not very obvious. Suggested order of inclusion is
 * 								System -> Other Modules -> Same Module -> Specific to this file
 * Note: Avoid nested inclusions.
 */

/* NO INCLUDES */


/******************************************************************************************************************/
/* CODING STANDARDS
 * Header file: Section III. Defines and typedefs: order of appearance -> constant macros, function macros,
 * 				typedefs and then enums.
 *
 * Custom data types and typedef: these definitions are best placed in a header file so that all source code
 * files which rely on that header file have access to the same set of definitions. This also makes it easier
 * to modify.
 * Naming convention: Use upper case and words joined with an underscore (_). Limit  the  use  of  abbreviations.
 * Constants: define and use constants, rather than using numerical values; it make code more readable, and easier
 * 			  to modify.
 * Note: Avoid initialized data definitions.
 */


/* NO DEFINITION OF TYPEDEF */

/******************************************************************************************************************/
/* CODING STANDARDS:
 * Header file: Section IV. Global   or   external   data   declarations -> externs, non­static globals, and then
 * 				static globals.
 *
 * Naming convention: variables names must be meaningful lower case and words joined with an underscore (_). Limit
 * 					  the  use  of  abbreviations.
 */


/* NO DEFINITION OF TYPEDEF */


/******************************************************************************************************************/
/* CODING STANDARDS
 * Header file: Section V. Functions: order on abstraction level or usage; and if independent alphabetical
 * 				or­dering is good choice.
 *
 * 1) Declare all the entry point functions.
 * 2) Declare function names, parameters (names and types) and re­turn type in one line; if not possible fold it at
 *    an appropriate place to make it easily readable.
 */


/*---------------------------------------  ENTRY POINTS  ---------------------------------------------------------*/
/*Declare your entry points here*/

void initialize_module_timer0(void);

unsigned long time_in_microseconds(void);

unsigned long time_in_milliseconds(void);

void delay_milliseconds(unsigned long milliseconds);

#endif /* INCLUDE_CUSTOM_TIMER_H_ */

/*!@}*/   // end module
