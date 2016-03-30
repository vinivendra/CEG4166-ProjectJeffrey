/****************************************************************************//*!
 * \defgroup usartAsyncModule  USART Asynchronous Serial Module
 * @{
******************************************************************************/


/****************************************************************************//*!
    \file usart_serial.h
    Module: USART Async Serial

    Description:  Provides asynchronous serial communication via any of the four USARTS available in the
    AtMega 2560.

    Both interrupt driven and polling functions are available.

    Uses freeRTOS ring buffers to support interrupt functions.
**********************************************************************************/


#ifndef LIB_SERIAL_H
#define LIB_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <avr/pgmspace.h>

#include "queue.h"
#include "portable.h"

#include "ringBuffer.h"

/****************************************************************//*!
 * \brief USART identifiers
 * Shall be used as indexes into corresponding structure arrays. They play the same
 * role as file identifiers in a Unix/Linux operating system
 ******************************************************************/
typedef enum {
  USART_0 = 0,   //!< identifier for USART0
  USART_1 = 1,   //!< identifier for USART1
  USART_2 = 2,   //!< identifier for USART2
  USART_3 = 3   //!< identifier for USART3
} USART_ID;


/*-----------------------------------------------------------*/
//Entry Points - documented in c file
/*-----------------------------------------------------------*/
// Initialisation and configuration
void setDefaultUSART(USART_ID usartId);
USART_ID usartOpen(USART_ID usartId, uint32_t ulWantedBaud, uint16_t uxTxQueueLength, uint16_t uxRxQueueLength );
void usartClose(USART_ID usartId );
// Polling functions using default USART
void usart_printf(const char *, ...);
void usart_printf_P(PGM_P, ...);
void usart_print(uint8_t *);
void usart_print_P(PGM_P);
// Polling functions that specifies USART
void usart_fprintf(int, const char *, ...);
void usart_fprintf_P(int, PGM_P, ...);
void usart_fprint(USART_ID, uint8_t *);
void usart_fprint_P(USART_ID, PGM_P);
void usartWrite(USART_ID, int8_t);
int8_t usartRead(USART_ID usartId);
// Interrupt functions using default USART
void usart_xprintf(const char *, ...);
void usart_xprintf_P(PGM_P, ...);
void usart_xprint(uint8_t *);
void usart_xprint_P(PGM_P);
// Interrupt functions that specifies USART
void usart_xfprintf(int, const char *, ...);
void usart_xfprintf_P(int, PGM_P, ...);
void usart_xfprint(USART_ID, uint8_t *);
void usart_xfprint_P(USART_ID, PGM_P);
void usart_xflushRx(USART_ID);
uint16_t usart_AvailableCharRx(USART_ID);
UBaseType_t usart_xgetChar(USART_ID, UBaseType_t * );
UBaseType_t usart_xputChar(USART_ID, const UBaseType_t);

#ifdef __cplusplus
}
#endif

#endif

/*!@}*/   // End of usartAsyncModule Group
