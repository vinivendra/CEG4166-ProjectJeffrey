/*
 * web_server.h
 *
 *
 *  Created on: Sep 14, 2015
 *      Author: shailendrasingh
 */

/****************************************************************************//*!
 * \defgroup web_server Module Web Server
 * @{
******************************************************************************/



/******************************************************************************************************************/
/* CODING STANDARD
 * Header file: Section I. Prologue: description about the file, description author(s), revision control
 * 				information, references, etc.
 * Note: 1. Header files should be functionally organized.
 *		 2. Declarations   for   separate   subsystems   should   be   in   separate
 */

/*(Doxygen help: use \brief to provide short summary and \details command can be used)*/

/*!\file web_server.h
 * 	\brief Web server to interact with clients connecting over WiFi.
 *
 * \details Limited function web server to interact with client connected over Gainspan module based
 * Wi-Fi network. A simple HTML page with elements such as drop down list (HTML_DROPDOWN_LIST)
 * or radio buttons (HTML_RADIO_BUTTON) can be created under a menu. Titles for page and menu can be
 * defined.
 *
 * Client response (single character for each event) are stored in a ring buffer, with size of
 * RING_BUFFER_SIZE.
 *
 * \note This module uses \ref usartAsyncModule and \ref gainspan_gs1011m .
 *
 * Usage guide :
 *
 * 		=> Set SET_WEB_SERVER_TERMINAL_OUTPUT_ON to 1 in "web_server.h" to get server logs on serial terminal.
 *
 * 		=> Configure web page by providing the page title, menu title and HTML element type
 *
 * 			call configure_web_page(char *page_title, char *menu_title, HTML_ELEMENT_TYPE element_type)
 *
 * 			Example: configure_web_page("Chico: The Robot", "! Control Interface !", HTML_DROPDOWN_LIST);
 *
 * 		=> Add elements (drop-down list entries or radio buttons) to the web page and complete the web page.
 *
 * 			call add_element_choice(char choice_identifier, char *element_label)
 *
 *			Example:
 *
 *				add_element_choice('F', "Forward");
 *
 *				add_element_choice('R', "Reverse");
 *
 * 		=> Start web server - with http port 80 and TCP protocol
 *
 * 			call start_web_server();
 *
 * 		=> Serve the incoming connection request from clients, ensure to call this function from task with
 * 			a sufficiently high frequency to serve all the request and avoid time-out for clients.
 *
 * 			call process_client_request();
 *
 * 		=> Read client response, the response (single character representing choice submission from web-page)
 * 			is stored in a ring buffer. Ensure to read the responses quick enough, as the ring buffer will be
 * 			overwritten by incoming responses.
 *
 * 			call get_next_client_response(void)
 *
 * 			Example: client_request = get_next_client_response();
 *
 *	\note To acknowledge and serve the HTTP request from client and read client response from web-page call
 *	functions process_client_request() and get_next_client_response() repeatedly in your task.
 *
 *
 */

#ifndef WEB_SERVER_H_
#define WEB_SERVER_H_


/******************************************************************************************************************/
/* CODING STANDARDS:
 * Header file: Section II. Include(s): header file includes. System include files and then user include files.
 * 				Ensure to add comments for an inclusion which is not very obvious. Suggested order of inclusion is
 * 								System -> Other Modules -> Same Module -> Specific to this file
 * Note: Avoid nested inclusions.
 */

#include "usart_serial.h"					/*USART Serial communication*/


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

#define SET_WEB_SERVER_TERMINAL_OUTPUT_ON				1				/*!Default - 0; set to 1 to send the commands and respective response from Gainspan device to serial terminal define in Gainspan data structure gainspan.serial_terminal_usart_id*/
#define SERIAL_TERNMINAL								USART_0			/*!Default - USART0 for serial terminal communication*/
#define SERVER_PORT										80				/*!Default - web server port*/
#define SERVER_PROTOCOL									PROTOCOL_TCP	/*!Default - protocol - PROTOCOL_TCP*/
#define RING_BUFFER_SIZE 								10				/*!Ring buffer size, no of characters*/


/*!
 * \brief HTML elements
 *
 *
 * \details Valid HTML elements for web-page
 *
 */
typedef enum{
	HTML_DROPDOWN_LIST												= 0,	/*!<HTML drop down list*/
	HTML_RADIO_BUTTON												= 1		/*!<HTML radio button*/
} HTML_ELEMENT_TYPE;

/******************************************************************************************************************/
/* CODING STANDARDS:
 * Header file: Section IV. Global   or   external   data   declarations -> externs, non­static globals, and then
 * 				static globals.
 *
 * Naming convention: variables names must be meaningful lower case and words joined with an underscore (_). Limit
 * 					  the  use  of  abbreviations.
 */

/*NO GLOBAL DECLARATION*/

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

void configure_web_page(char *page_title, char *menu_title, HTML_ELEMENT_TYPE element_type);

void add_element_choice(char choice_identifier, char *element_label);

void start_web_server(void);

void process_client_request(void);

char get_next_client_response(void);

#endif /* WEB_SERVER_H_ */

/*!@}*/   // end module
