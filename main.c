////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////    main.c
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

/* serial interface include file. */
//#include "usartserial.h"

#include "temperatureTask.h"

/*-----------------------------------------------------------*/

int main(void) {
  // turn on the serial port for debugging or for other USART reasons.
  // int usartfd;
  // usartfd = usartOpen( USART0_ID, 115200, portSERIAL_BUFFER_TX,
  // portSERIAL_BUFFER_RX); //  serial port: WantedBaud, TxQueueLength,
  // RxQueueLength (8n1)

  // usart_print_P(PSTR("\r\n\n\nHello World!\r\n")); // Ok, so we're alive...

  I2C_Master_Initialise(0xBA);

  xTaskCreate(temperatureTask,
              (const portCHAR *)"RedLED", // Main Arduino Mega 2560, Freetronics
                                          // EtherMega (Red) LED Blink
              256,                        // Tested 9 free @ 208
              NULL, 3, NULL);

  //  usart_printf_P(PSTR("\r\n\nFree Heap Size: %u\r\n"),
  //                 xPortGetFreeHeapSize()); // needs heap_1 or heap_2 for this
  //                                          // function to succeed.

  vTaskStartScheduler();

  // usart_print_P(PSTR("\r\n\n\nGoodbye... no space for idle task!\r\n")); //
  // Doh, so we're dead...
}

/*-----------------------------------------------------------*/

/**
 *  Do we really need this?
 *
 *  @param xTask      <#xTask description#>
 *  @param pcTaskName <#pcTaskName description#>
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, portCHAR *pcTaskName) {

  DDRB |= _BV(DDB7);
  PORTB |= _BV(PORTB7); // main (red PB7) LED on. Mega main LED on and die.

  while (1)
    ;
}

/*-----------------------------------------------------------*/
