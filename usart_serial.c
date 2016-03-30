/****************************************************************************//*!
 * \defgroup usartAsyncModule  USART Asynchronous Serial Module
 * @{
******************************************************************************/

// Doxygen documentation: Module Design

/****************************************************************************//*!
 * \file usart_serial.c
 * \author Gilbert Arbez
 * \date May 26 2015
 * \brief Module for asynchronous communication with the Atmega2560 USART's.
 * \details USART Asynchronous Serial Module
 *
 * Both interrupt driven and polling functions are available.
 *
 * Uses freeRTOS ring buffers to support interrupt functions.
 *
 * Design Principles
 *
 * The design of this module is loosely based on the Unix/Linux file system which is used to support I/O
 * to various devices (e.g. network sockets) in addition to supporting I/O with files.  The functions of
 * this system consists of:
 * 1. Low level open/close functions to establish and release connections.
 * 2. Low level read() and write() functions for reading and writing a sequence of bytes from and to the connections.
 * 3. Use of an integer handle (which is called the usart identifier, i.e. usartId, in this module).
 * 4. Higher level functions that use the read/write functions for formating data.
 * 5. Use of the variable parameter list and the vsprintf function for formating strings.
 *
 * Functions are augmented to provide versions that support the following features
 * 1. No usart identifier, i.e., define a standard input/output that can be set to any of the USARTS
 * 2. Polling functions that do use interrupts and can be used prior to enabling interrupts (i.e. the freeRTOS scheduler).
 * 3. Interrupt driven functions that uses the freeRTOS ring buffer for buffering data.  When the buffer is full the taak
 * that is calling the function shall block.
 * 4. Functions which uses formating string stored in program memory.
 * 5. Functions which uses formating strings stored in ram memory.
 *
 * Data Structures
 * Two arrays of data structures are used to implement this module:
 * 1. usartReg (array of USART_REGISTERS) - provides pointers to the USART hardware module registers.
 * 2. usartComBuf (array of USART_COM_BUF) - provides a set of buffers for each arrays to support communications.
 * Buffering is important for supporting transmission/reception using interrupts.  Two ring buffers (xRxedChars and
 * xCharsForTx) are used to buffer received characters and to buffer characters for transmission.  A work buffer
 * (serialWorkBuffer) is used to format strings for transmission (using the Unix/Linux printf style).
 *
 * Functions
 *
 * The functions are divided into two major sets:
 * 1. A set of functions that do not use interrupts. These functions can be used prior to invoking the freeRTOS
 * scheduler which enables interrupts.\
 * 2. A set of functions that use interrupts.  These functions are used after interrupts are enabled (i.e. the freeRTOS
 * scheduler has been invoked).  These provide more efficient processing since it does not occupy the CPU during the
 * transmission of a string.
 *
 * Each of these sets are further subdivided into two:
 * 1. A set of functions that uses a default USART (defined by the variable defaultUSART)
 * 2. A set of functions with a parameter, usartId, to define the USART used for communications.
 *
 * The format of the entry point function names are as follows
 * (the _P suffix indicates that the formating string is stored in program memory rather than RAM).
 * 1. Uses default USART
 *    - usart_printf(...)
 *    - usart_printf_P (...)
 * 2. Uses the specified USART
 *    - usart_fprintf(USART_ID usartID, ...)
 *    - usart_fprintf_P(USART_ID usartID, ...)
 * 3. Uses default USART with interrupts
 *    - usart_xprintf(...)
 *    - usart_xprintf_P (...)
 * 4. Uses the specified USART with interrupts
 *    - usart_xfprintf(USART_ID usartID, ...)
 *    - usart_xfprintf_P(USART_ID usartID, ...)
 *
 * Note that printf can be replace by other names (e.g. print to print an unformatted string).
 *************************************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <util/delay.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "ringBuffer.h"

#include "bit_definitions.h"
#include "usart_serial.h"


/*---------------------------------------------------------------------
 Local data structures
----------------------------------------------------------------------*/

//--------------------- Definitions -----------------------------------

/******************************************************************************//*!
 * \brief Defines the state of the USART.
 *********************************************************************************/
typedef enum
{
	USART_VACANT,//!< USART is available for access
	USART_ENGAGED//!< USART is engaged by another task.
} USART_STATE;

/******************************************************************************//*!
 *  \brief Structure of buffers to support communications with a USART.
 **********************************************************************************/
typedef struct
{
	ringBuffer_t xRxedChars;  //!< Buffer for receiving characters  (Space allocated on the heap)
	ringBuffer_t xCharsForTx;  //!< Buffer for transmitting characters (space allocated on the heap)
	uint8_t *workBuffer; //!< create a working buffer pointer, to later be malloc() on the heap.
	uint16_t workBufferSize; //!< size of working buffer as created on the heap.
	USART_STATE	workBufferInUse;	//!< flag to prevent overwriting by multiple tasks using the same USART.
} USART_COM_BUF;

/******************************************************************************//*!
 *  \brief Structure of pointers to USART registers.
 *********************************************************************************/
typedef struct
{
	volatile uint8_t *udrPtr;  //!< Pointer to data register UDRn
	volatile uint8_t *ucsrAPtr; //!< Pointer to control and status register USCRnA
	volatile uint8_t *ucsrBPtr; //!< Pointer to control and status register USCRnB
	volatile uint8_t *ucsrCPtr; //!< Pointer to control and status register USCRnC
	volatile uint16_t *ubbrPtr;  //!< Pointer to baud rate register UBBRnL/UBBRnH

} USART_REGISTERS;

//--------------------Bit definitions
// UCSR_A
#define RXC_BIT	    BIT7
#define TXC_BIT	    BIT6
#define UDRE_BIT	BIT5
#define FE_BIT		BIT4
#define DOR_BIT 	BIT3
#define UPE_BIT		BIT2
#define U2X_BIT		BIT1
// UCSR_B
#define RXCIE_BIT 	BIT7
#define UDRIE_BIT   BIT5
#define RXEN_BIT 	BIT4
#define TXEN_BIT    BIT3
#define UCSZ1_BIT   BIT2
#define UCSZ0_BIT   BIT1

//------------------------------------------Data structures------------------------------------
#define NUM_USARTS 4 // number of USARTS

/******************************************************************************//*!
 *  Array of USART_REGISTERS structures that defines the references to the registers of all four USARTS.
 * Use the enum USART_ID identifiers as indices into the array.
 *************************************************************************************************/
USART_REGISTERS usartReg[] =
{
		{ &UDR0, &UCSR0A, &UCSR0B, &UCSR0C, &UBRR0  },
		{ &UDR1, &UCSR1A, &UCSR1B, &UCSR1C, &UBRR1  },
		{ &UDR2, &UCSR2A, &UCSR2B, &UCSR2C, &UBRR2  },
		{ &UDR3, &UCSR3A, &UCSR3B, &UCSR3C, &UBRR3  }
};

/******************************************************************************//*!
 *  Array of USART_COM_BUF structures that provide buffers for each of the USART's. Note that the buffers
 * are allocated an initialised by the usartOpen function and released by the usartClose function.
 * Use the enum USART_ID identifiers as indices into the array.
  *************************************************************************************************/
USART_COM_BUF usartComBuf[NUM_USARTS];  // One set of buffer structures per USART

/******************************************************************************//*!
 *  Default USART
 * Can be changed using setDefaultUSART()
  *************************************************************************************************/
USART_ID defaultUSART = USART_0;   //< default USART - defaults to USART0


//--------------------------------Prototypes Local Functions-----------------------
// Polling functions

int8_t usartCheckRxComplete(USART_ID);
int8_t usartCheckTxReady(USART_ID);

void usart_fprintf_arg(USART_ID usartId, const char * format, va_list ap);
void usart_fprintf_P_arg(USART_ID usartId, PGM_P format, va_list arg);

// Interrupt functions
void usart_xfprintf_arg(USART_ID usartId, const char * format, va_list arg);
void usart_xfprintf_P_arg(USART_ID usartId, PGM_P format, va_list arg);

// Interrupt section
void xmitInterrupt_On(USART_ID );
void xmitInterrupt_Off(USART_ID );

void usart_rx_isr(USART_ID );
void usart_tx_isr(USART_ID );


/*===============================================================================================================
 * ENTRY POINTS
 *===============================================================================================================*/

/*-----------------------------------------------------------*/
// Initialization: open/close communications
/*-----------------------------------------------------------*/

/*************************************************************************************//*!
 * \brief Open a connection on a USART.
 *
 * Sets up the connection for both transmission and reception.  All buffers for supporting communications are allocated
 * and control registers of the USARTS initialised for both transmission and reception.
 *
 * @param usartId - USART identifier.
 * @param ulWantedBaud - USART bit rate (units of bits/second)
 * @param uxTxQueueLength - length of the transmission ring buffer (also sets the length of the work buffer)
 * @param uxRxQueueLength - length of the reception ring buffer
 * @return Usart identifier.
******************************************************************************************/

USART_ID usartOpen(USART_ID usartId, uint32_t ulWantedBaud, uint16_t uxTxQueueLength, uint16_t uxRxQueueLength )
{
	uint8_t * dataPtr;

	/* Create the ring-buffers for the USART. */
	if( (dataPtr = (uint8_t *)pvPortMalloc( sizeof(uint8_t) * uxRxQueueLength )))
		ringBuffer_InitBuffer( &(usartComBuf[usartId].xRxedChars), dataPtr, uxRxQueueLength);

	if( (dataPtr = (uint8_t *)pvPortMalloc( sizeof(uint8_t) * uxTxQueueLength )))
		ringBuffer_InitBuffer( &(usartComBuf[usartId].xCharsForTx), dataPtr, uxTxQueueLength);

	// create a working buffer for vsnprintf on the heap (so we can use extended RAM, if available).
	// create the structures on the heap (so they can be moved later).
	if( !(usartComBuf[usartId].workBuffer = (uint8_t *)pvPortMalloc( sizeof(uint8_t) * uxTxQueueLength )))
		usartComBuf[usartId].workBuffer = NULL;

	usartComBuf[usartId].workBufferSize = uxTxQueueLength; // size of the working buffer for vsnprintf
	usartComBuf[usartId].workBufferInUse = USART_VACANT;  // clear the occupation flag.

	portENTER_CRITICAL();  // Disable interrupts during configuration
	/*
	 * Calculate the baud rate register value from the equation in the data sheet. */

	/* As the 16MHz Arduino boards have bad karma for serial port, we're using the 2x clock U2X0 */
	// for Arduino at 16MHz; above data sheet calculation is wrong. Need below from <util/setbaud.h>
	// This provides correct rounding truncation to get closest to correct speed.
	// Normal mode gives 3.7% error, which is too much. Use 2x mode gives 2.1% error.
	// Or, use 22.1184 MHz over clock which gives 0.00% error, for all rates.

	// ulBaudRateCounter = ((configCPU_CLOCK_HZ + ulWantedBaud * 8UL) / (ulWantedBaud * 16UL) - 1); // for normal mode
	// ulBaudRateCounter = (uint16_t)((configCPU_CLOCK_HZ + ulWantedBaud * 4UL) / (ulWantedBaud * 8UL) - 1);  // for 2x mode
	*usartReg[usartId].ubbrPtr = (uint16_t)((configCPU_CLOCK_HZ + ulWantedBaud * 4UL) / (ulWantedBaud * 8UL) - 1);  // for 2x mode, using 16 bit avr-gcc capability.
	/* Set the 2x speed mode bit */
	*usartReg[usartId].ucsrAPtr = U2X_BIT;  /* bits 7:2 are status bits, bit 0 set to 0 disables multi-processor comms mode */

	/* Enable the Rx and Tx. Also enable the Rx interrupt. The Tx interrupt will get enabled later. */
	*usartReg[usartId].ucsrBPtr = RXCIE_BIT | RXEN_BIT | TXEN_BIT;

	/* Set the data bit register to 8n1. */
	/* Set to asycrhnoous USART (bits 7:6 set to 0), parity disables (bits 5:4 is 0) 1 stop bit (bit 3 set to 0)
	 * 8 data bits (UCSZ1, UCSZ0 set to 1)
	 */
	*usartReg[usartId].ucsrCPtr = UCSZ1_BIT | UCSZ0_BIT;  // 011 stored in UCSZ2:1 - bit UCSZ2 is zero in register ucsrB (above instruction)

	portEXIT_CRITICAL(); // Enables interrupts after configuration

	return usartId;
}

/*************************************************************************************//*!
 *  \brief Update default USART.
 *
 *  The usart must be initilased with usartOpen to be used as the default USART.
 * @param usartId - Usart Identifier.
 *****************************************************************************************/
void setDefaultUSART(USART_ID usartId) { defaultUSART = usartId; }

/*******************************************************************************//*!
 * \brief Close a connection on a USART
 *
 * The buffers allocated to the connection are released.  USART registers are configure to turn off reception and
 * transmission.  Interrups are also turned off.
 * @param usartId
******************************************************************************************/
void usartClose(USART_ID usartId )
{
	uint8_t ucByte;

	/* Turn off the interrupts.  We may also want to delete the queues and/or
	re-install the original ISR. */

	vPortFree( usartComBuf[usartId].workBuffer );
	vPortFree( usartComBuf[usartId].xRxedChars.start );
	vPortFree( usartComBuf[usartId].xCharsForTx.start );

	portENTER_CRITICAL();   // Turn off interrupts
	xmitInterrupt_Off(usartId);
	//
	ucByte = *usartReg[usartId].ucsrBPtr;
	ucByte &= ~RXCIE_BIT;
	*usartReg[usartId].ucsrBPtr = ucByte;
	portEXIT_CRITICAL();   // Enable interrupts
}

/*-------------------------------------------------------------*/
// Polling functions
/*-----------------------------------------------------------------*/
// Polling read and write routines, for use before freeRTOS vTaskStartScheduler (interrupts enabled).
// The following functions use the default USART defined by defaultUSART.

/*************************************************************************************//*!
 * \brief Standard printf function
 *
 * This function has a variable list and uses the variable list feature of C (see documentation on stdarg). This function
 * calls usart_fprintf using the USART identifier provided by defaultUSART.
 * @param format -  formatting string
******************************************************************************************/
void usart_printf(const char * format, ...)
{
	va_list arg;
	va_start(arg, format);
	usart_fprintf_arg(defaultUSART, (const char *)format, arg);
	va_end(arg);
}

/*************************************************************************************//*!
 * \brief Standard printf function that uses a formating string stored in memory.
 *
 * The function calls usart_fprintf_P to print to the USART defined with defaultUSART.
 * @param format
 ****************************************************************************************/
void usart_printf_P(PGM_P format, ...)
{
	va_list arg;
	va_start(arg, format);
	usart_fprintf_P_arg(defaultUSART, format, arg);
	va_end(arg);
}

/*************************************************************************************//*!
 * \brief Print string on default USART
 *
 * The function transmits the given string to the default USART defined by defaultUSART
 * with a call to usart_fprint().
 * @param str - The string to transmit.
******************************************************************************************/
void usart_print(uint8_t * str)
{
	usart_fprint(defaultUSART, str);
}

/*************************************************************************************//*!
 * \brief Print string stored in program memory on default USART.
 *
 * The function transmits the given string stored in program memory to the default USART defined by defaultUSART
 * with a call to usart_fprint_P().
 * @param str
 */
void usart_print_P(PGM_P str)
{
	usart_fprint_P(defaultUSART, str);
}

/*-----------------------------------------------------------*/

// Polling read and write routines, for use before freeRTOS vTaskStartScheduler (interrupts enabled).
// USART specified with first argument usartId.
// These can be set to use any USART.

/*************************************************************************************//*!
 * \brief Standard printf for specified USART.
 *
 * This function has a variable list and uses the variable list feature of C (see documentation on stdarg). The function
 * uses vsnprintf function for formating the string to output in the workBuffer before transmission. This function
 * calls usart_print to send the contents of the work buffer to the USART.
 * @param usartid - USART identifier - note that var_list, va_start, etc does not work when using USERID enumerated types.  must be an int.
 * @param format - Formating string.
******************************************************************************************/
void usart_fprintf(int usartid, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	usart_fprintf_arg(usartid, (const char *) format, arg);
	va_end(arg);
}

/*************************************************************************************//*!
 * \brief Standard printf function for specific USART with formating string in program memory.
 *
 * See usart_fprintf for details.  Requires PTM_P types for formatting string.
 * @param usartId - USART identifier - - note that var_list, va_start, etc does not work when using USERID enumerated types.  must be an int.
 * @param format - Formating string.
 ******************************************************************************************/
void usart_fprintf_P(int usartId, PGM_P format, ...)
{
	va_list arg;
	va_start(arg, format);
	usart_fprintf_P_arg(usartId, format, arg);
	va_end(arg);
}

/*************************************************************************************//*!
 * \brief Print string to specific USART
 *
 * Transmits a given string to the USART.
 * @param usartId - USART identifier
 * @param str - String to transmit via the USART.
******************************************************************************************/
void usart_fprint(USART_ID usartId, uint8_t * str)
{
	uint16_t i = 0;
	size_t stringlength;

	stringlength = strlen((char *)str);

	while(i < stringlength)
		usartWrite(usartId, str[i++]);
}
/***********************************************************************************//*!
 * \brief Print a string stored in program memory to specific USART.
 *
 * Prints a string stored in the program memory to the USART identified by usartId.  Special functions
 * are required to access the string stored in program memory.
 * @param usartId - USART identifier.
 * @param str - String for transmitting over USART.
 */
void usart_fprint_P(USART_ID usartId, PGM_P str)
{
	uint16_t i = 0;
	size_t stringlength;

	stringlength = strlen(str);//strlen_P(str);

	while(i < stringlength)
		usartWrite(usartId, pgm_read_byte(&str[i++]));
}

/*************************************************************************************//*!
 *  \brief Low level write function (single character).
 * Will delay until usart is ready to accept the 8 bit data.
 *
 * @param usartId - identifier of the usart (see USART_ID type; used with usartReg array to get pointer to data register UDR.
 * @param dataOut - the 8-bit data to write to the data register.
******************************************************************************************/
void usartWrite(USART_ID usartId, int8_t dataOut)
{
	while (!usartCheckTxReady(usartId))		// while NOT ready to transmit
        _delay_us(25);     						// delay

	*usartReg[usartId].udrPtr = dataOut;
}

/*************************************************************************************//*!
 *  \brief Low level read function (single character).
 * Delays until data available from the USART.  Will check for reception errors.
 *
 * @param usartId - identifier of the usart (see USART_ID type; used with usartReg array to get pointer to data register UDR.
 * @return 8-bit data received and read from the UDR register. Returns - 0xFF if an error occured.
*******************************************************************************************/
int8_t usartRead(USART_ID usartId)
{
	int8_t retVal = 0xFF; // Error value

	while (!usartCheckRxComplete(usartId))	// While data is NOT available to read
		_delay_us(25);     						    // delay
	// Get status and data
	// from buffer

	if(! (*usartReg[usartId].ucsrAPtr & (FE_BIT| DOR_BIT | UPE_BIT)) )
		retVal = *usartReg[usartId].udrPtr;

	// If error, return 0xFF
	return retVal;
}

/*-------------------------------------------------------------*/
// Interrupt driven functions
/*-----------------------------------------------------------------*/

// The following four functions uses the default USART defined by defaultUSART.

/********************************************************************************//*!
 * \brief Standard printf (interrupt driven) to default USART.
 *
 * Formats and prints string on default USART (defaultUSART) using the usart_xfprintf function.
 * @param format - Formatting string. The string is stored in the
 * transmission ring buffer for transmission with the ISRs.
 */
void usart_xprintf(const char * format, ...)
{
	va_list arg;
	va_start(arg, format);
	usart_xfprintf_arg(defaultUSART, format, arg);
	va_end(arg);
}
/********************************************************************************//*!
 * \brief Standard printf (interrupt driven) to default USART using formating string stored in program memory
 *
 * Formats and prints string on default USART (defaultUSART) using the usart_xfprintf_P function.
 * @param format - Formatting string stored in program memory. The string is stored in the
 * transmission ring buffer for transmission with the ISRs.
 */
void usart_xprintf_P(PGM_P format, ...)
{
	va_list arg;
	va_start(arg, format);
	usart_xfprintf_P_arg(defaultUSART, format, arg);
	va_end(arg);
}
/********************************************************************************//*!
 * \brief Prints a string on the default USART (defined by defaultUSART).
 *
 * Uses a call to usart_xfprint to print a string referenced by str.  The string is stored in the
 * transmission ring buffer for transmission with the ISRs.
 * @param str - Pointer to string.
 */
void usart_xprint(uint8_t * str)
{
	usart_xfprint(defaultUSART, str);
}
/********************************************************************************//*!
 * \brief Prints a string stored in program memory on the default USART (defined by defaultUSART).
 *
 * Uses a call to usart_xfprint_P to print a string referenced by str.  The string is stored in the
 * transmission ring buffer for transmission with the ISRs.
 * @param str - Pointer to string in program memory.
 */
void usart_xprint_P(PGM_P str)
{
	usart_xfprint_P(defaultUSART, str);
}

// The following functions can be used with any USART
/********************************************************************************//*!
 * \brief  Standard printf function for specific USART.
 *
 * This function has a variable list and uses the variable list feature of C (see documentation on stdarg). The function
 * uses vsnprintf function for formating the string to output in the workBuffer before transmission. This function
 * calls usart_fprint_P to send the contents of the work buffer to the USART.
 *
 * @param usartId - USART Identifier - note that var_list, va_start, etc does not work when using USERID enumerated types.  must be an int.
 * @param format - Formating string.
 */
void usart_xfprintf(int usartId, const char * format, ...)
{
	va_list arg;
	va_start(arg, format);
	usart_xfprintf_arg(usartId, format, arg);
	va_end(arg);
}

/********************************************************************************//*!
 * \brief Standard printf function for specific USART that uses a formating string stored in program memory.
 *
 * This function has a variable list and uses the variable list feature of C (see documentation on stdarg). The function
 * uses vsnprintf_P function for formating the string to output in the workBuffer before transmission. This function
 * calls usart_fprint_P to send the contents of the work buffer to the USART.  The formating string is stored
 * in program memory.
 * @param usartId - USART identifier - note that var_list, va_start, etc does not work when using USERID enumerated types.  must be an int.
 * @param format - Formating String.
 */

void usart_xfprintf_P(int usartId, PGM_P format, ...)
{
	va_list arg;
	va_start(arg, format);
	usart_xfprintf_P_arg(usartId, format, arg);
	va_end(arg);
}

/********************************************************************************//*!
 * \brief Print string to specific USART.
 *
 * Adds the passed string to the ring buffer using usart_xputChar.
 * @param usartId
 * @param str
 ************************************************************************************/
void usart_xfprint(USART_ID usartId, uint8_t * str)
{
	int16_t i = 0;
	size_t stringlength;

	stringlength = strlen((char *)str);

	while(i < stringlength)
		usart_xputChar( usartId, str[i++]);
}
/********************************************************************************//*!
 * \brief Print string stored in program memory to specific USART.
 *
 * Adds the passed string to the ring buffer using usart_xputChar.
 * @param usartId - USART identifier.
 * @param str - Reference to string stored in program memory.
 ***********************************************************************************/
void usart_xfprint_P(USART_ID usartId, PGM_P str)
{
	uint16_t i = 0;
	size_t stringlength;

	stringlength = strlen(str); //strlen_P(str);

	while(i < stringlength)
		usart_xputChar( usartId, pgm_read_byte(&str[i++]) );
}

/********************************************************************************//*!
 * \brief Flushes all characters in the receive ring buffer (i.e. empties the buffer).
 *
 * All characters in the receive ring buffer are discarded.  If characters are waiting in the
 * USART for reading (a 2 byte FIFO is used for buffering received characters, they are also discarded.
 *
 * @param usartId - USART identifier.
 */
void usart_xflushRx(USART_ID usartId)
{
	// Flush received characters from the USART transmit buffer.
	register uint8_t byte __attribute__ ((unused));
	// Flush the bytes in the USART
	while (*usartReg[usartId].ucsrAPtr & RXC_BIT) byte = *usartReg[usartId].udrPtr;
	// Flush the characters in the ring buffer.
	ringBuffer_Flush( &(usartComBuf[usartId].xRxedChars) );
}

/**********************************************************************//*!
 * \brief Gives number of bytes in reception ring buffer.
 *
 * This function can be used to check the number of available characters in the reception
 * ring buffer.  Returns 0 if no characters are available in the ring buffer.
 * @param usartId - USART identifier
 * @return Number of available characters in the reception ring buffer.  Returns 0 of no characters are available.
 ****************************************************************************/
uint16_t usart_AvailableCharRx(USART_ID usartId)
{
	// Are characters available in the serial port buffer.

	return ringBuffer_GetCount( &(usartComBuf[usartId].xRxedChars) );
}

/*********************************************************************************//*!
 * \brief Get a character from the reception ring buffer.
 *
 * The retrieved character if available is stored using pointer pcRxedChar.  The function
 * returns pdTRUE if a character was available and pdFALSE otherwise (in this latter case, the contents
 * referenced by pcRxedChar are not changed).
 * @param usartId - USART identifier
 * @param pcRxedChar - pointer for storing character read.
 * @return pdFALSE if a character was available in the reception ring buffer (stored at pcRXedChar) and pdFALSE otherwise.
 */
UBaseType_t usart_xgetChar( USART_ID usartId, UBaseType_t *pcRxedChar )
{
	UBaseType_t retVal = pdFALSE;
	// Get the next character from the ring buffer.  Return false if no characters are available
	if( ! ringBuffer_IsEmpty( &(usartComBuf[usartId].xRxedChars) ) )
	{
		* pcRxedChar = ringBuffer_Pop( &(usartComBuf[usartId].xRxedChars) );
		retVal = pdTRUE;
	}
	return retVal;
}

/*************************************************************************//*!
 * \brief Drops a character into the transmit ring buffer.
 *
 * If the ring buffer is full, the function delays to allow transmission of characters to free
 * up space in buffer.  The function turns on the transmit interrupt for the USART.
 * @param usartId
 * @param cOutChar
 * @return
 */
UBaseType_t usart_xputChar(USART_ID usartId, const UBaseType_t cOutChar )
{
	// Return false if there remains no room on the Tx ring buffer
	UBaseType_t retVal = pdPASS;
	if( ! ringBuffer_IsFull( &(usartComBuf[usartId].xCharsForTx) ) )
		ringBuffer_Poke( &(usartComBuf[usartId].xCharsForTx), cOutChar ); // poke in a fast byte
	else
	{
		 // go slower, per character rate for 115200 is 86us
		_delay_us(25); // delay for about one character (maximum _delay_loop_1() delay is 32 us at 22MHz)
		_delay_us(25);
		_delay_us(25);
		_delay_us(25);

		if( ! ringBuffer_IsFull( &(usartComBuf[usartId].xCharsForTx) ) )
			ringBuffer_Poke( &(usartComBuf[usartId].xCharsForTx), cOutChar ); // poke in a byte slowly
		else
			retVal = pdFAIL; // if the Tx ring buffer remains full
	}
	xmitInterrupt_On(usartId);
	return retVal;
}
/*===============================================================================================================
 * LOCAL FUNCTONS
 *===============================================================================================================*/


void usart_fprintf_arg(USART_ID usartId, const char * format, va_list arg)
{
	while(usartComBuf[usartId].workBufferInUse == USART_ENGAGED ) _delay_us(25);
	usartComBuf[usartId].workBufferInUse = USART_ENGAGED;

	vsnprintf((char *)(usartComBuf[usartId].workBuffer), usartComBuf[usartId].workBufferSize, (const char *)format, arg);
	usart_fprint(defaultUSART, (usartComBuf[usartId].workBuffer));

	usartComBuf[usartId].workBufferInUse = USART_VACANT;
}

void usart_fprintf_P_arg(USART_ID usartId, PGM_P format, va_list arg)
{
	while(usartComBuf[usartId].workBufferInUse == USART_ENGAGED ) _delay_us(25);
	usartComBuf[usartId].workBufferInUse = USART_ENGAGED;

	vsnprintf_P((char *)(usartComBuf[usartId].workBuffer), usartComBuf[usartId].workBufferSize, format, arg);
	usart_fprint(usartId,usartComBuf[usartId].workBuffer);

	usartComBuf[usartId].workBufferInUse = USART_VACANT;

}

void usart_xfprintf_arg(USART_ID usartId, const char * format, va_list arg)
{
	while(usartComBuf[usartId].workBufferInUse == USART_ENGAGED ) taskYIELD();
	usartComBuf[usartId].workBufferInUse = USART_ENGAGED;

	vsnprintf((char *)(usartComBuf[usartId].workBuffer), usartComBuf[usartId].workBufferSize, (const char *)format, arg);
	usart_xfprint(usartId, (uint8_t *)(usartComBuf[usartId].workBuffer));

	usartComBuf[usartId].workBufferInUse = USART_VACANT;
}

void usart_xfprintf_P_arg(USART_ID usartId, PGM_P format, va_list arg)
{
	while(usartComBuf[usartId].workBufferInUse == USART_ENGAGED ) taskYIELD();
	usartComBuf[usartId].workBufferInUse = USART_ENGAGED;

	vsnprintf_P((char *)(usartComBuf[usartId].workBuffer), usartComBuf[usartId].workBufferSize, format, arg);
	usart_xfprint(usartId, (uint8_t *)(usartComBuf[usartId].workBuffer));

	usartComBuf[usartId].workBufferInUse = USART_VACANT;
}

/*************************************************************************************//*!
 *  \brief Check if reception is complete.
 * The function returns the status of the RXC bit of the USART.  When set, the bit indicates that data can be read from the USART.
 *
 * @param usartId - identifier of the usart (see USART_ID type; used with usartReg array to get pointer to control register UCSRA.
 * @return - status of the RSC bit, if non-zero, the USART has received data and the data register can be read.
******************************************************************************************/
int8_t usartCheckRxComplete(USART_ID usartId)
{
	return(*usartReg[usartId].ucsrAPtr & RXC_BIT);  // nonzero if serial data is available to read.
}

/*************************************************************************************//*!
 *  \brief Check if transmission is complete.
 * The function returns the status of the UDRE bit of the USART.  When set, the bit indicates that data can be written to the USART.
 *
 * @param usartId - identifier of the usart (see USART_ID type; used with usartReg array to get pointer to control register UCSRA.
 * @return - status of the UDRE bit, if non-zero, the USART, data for transmission can be written to the transmit register UDRE.
 ******************************************************************************************/
inline int8_t usartCheckTxReady(USART_ID usartId)
{
	return( *usartReg[usartId].ucsrAPtr & UDRE_BIT); // nonzero if transmit register is ready to receive new data.
}



/*===============================================================================================================
 * Interrupt Section
 *===============================================================================================================*/

/*************************************************************************************//*!
 *  Turns on the transmit interrupt for a USART
 * Sets the UDRIE bit in the control and status register UCSRB which configures the USART to
 * generate an interrupt when the transmit data register is empty, i.e. can write a character
 * to the transmit register (UDR) for transmission.  The receive interrupt is enabled when the
 * USART is configured.
 * @param usartId - USART identifier
 ******************************************************************************************/
inline void xmitInterrupt_On(USART_ID usartId)
{
	uint8_t ucByte;
	ucByte  = *usartReg[usartId].ucsrBPtr;
	ucByte |= UDRIE_BIT;
	*usartReg[usartId].ucsrBPtr  = ucByte;
}

/*************************************************************************************//*!
 *  Turns off the transmit interrupt for a USART.
 * Clears the UDRIE bit in the control and status register UCSRB which disables the interrupt
 * when the transmit data register is empty.
 * @param usartId - USART identifier
 ******************************************************************************************/

inline void xmitInterrupt_Off(USART_ID usartId)
{
	uint8_t ucByte;

	ucByte  = *usartReg[usartId].ucsrBPtr;
	ucByte &= ~UDRIE_BIT;
	*usartReg[usartId].ucsrBPtr  = ucByte;
}

// Receive interrupts
ISR(USART0_RX_vect) { usart_rx_isr(USART_0); }
ISR(USART1_RX_vect) { usart_rx_isr(USART_1); }
ISR(USART2_RX_vect) { usart_rx_isr(USART_2); }
ISR(USART3_RX_vect) { usart_rx_isr(USART_3); }

/*************************************************************************************//*!
 *  \private Interrupt service routine for character reception.
 * Saves a received character in the receive ring buffer.  If an error occurred, nothing is done.
 * If the ring buffer is full, the character is lost.
 * @param usartId - USART id
******************************************************************************************/
void usart_rx_isr(USART_ID usartId)
{
	uint8_t cChar;

	/* Get status and data from buffer */

	/* If error bit set (Frame Error, Data Over Run, Parity), do nothing */
	if ( ! (*usartReg[usartId].ucsrAPtr & ((1<<FE0)|(1<<DOR0)|(1<<UPE0)) ) )
	{
		/* If no error, get the character and post it on the buffer of Rxed characters.*/
		cChar = *usartReg[usartId].udrPtr;

		if( ! ringBuffer_IsFull( &(usartComBuf[usartId].xRxedChars) ) )
			ringBuffer_Poke( &(usartComBuf[usartId].xRxedChars), cChar);
	}
}
/*-----------------------------------------------------------*/
// Transmit interrupts
ISR(USART0_UDRE_vect) { usart_tx_isr(USART_0); }
ISR(USART1_UDRE_vect) { usart_tx_isr(USART_1); }
ISR(USART2_UDRE_vect) { usart_tx_isr(USART_2); }
ISR(USART3_UDRE_vect) { usart_tx_isr(USART_3); }

/***************************************************************************************//*!
 * \private Interrupt service routine for character transmission.
 * Transmits a character from the transmit ring buffer.  If the buffer is empty, the transmit interrupt is disabled.
 * @param usartId - USART id
 ******************************************************************************************/
void usart_tx_isr(USART_ID usartId)
{
	if( ringBuffer_IsEmpty( &(usartComBuf[usartId].xCharsForTx) ) )
	{
		// Queue empty, nothing to send.
		xmitInterrupt_Off(usartId);
	}
	else
	{
		*usartReg[usartId].udrPtr = ringBuffer_Pop( &(usartComBuf[usartId].xCharsForTx) );
	}
}

/*!@}*/   // End of usartAsyncModule Group
