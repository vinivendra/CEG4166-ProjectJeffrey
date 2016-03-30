/*
 * web_server.c
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
/* CODING STANDARDS:
 * Program file: Section I. Prologue: description about the file, description author(s), revision control
 * 				information, references, etc.
 */

/*(Doxygen help: use \brief to provide short summary and \details command can be used)*/

/*!	\file web_server.c
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


/******************************************************************************************************************/
/* CODING STANDARDS:
 * Program file: Section II. Include(s): header file includes. System include files and then user include files.
 * 				Ensure to add comments for an inclusion which is not very obvious. Suggested order of inclusion is
 * 								System -> Other Modules -> Same Module -> Specific to this file
 * Note: Avoid nested inclusions.
 */

/* --Includes-- */
#include "FreeRTOS.h" 						/* for various kernel functions */
//#include "serial.h"

#include <stdio.h>							/* for text string formatting functions */
#include <string.h>
#include <stdlib.h>

/*AVR includes*/
#include <avr/io.h>
#include <util/delay.h>

/* module includes */
#include "gainspan_gs1011m.h"				/* module include */
#include "web_server.h"

/******************************************************************************************************************/
/* CODING STANDARDS:
 * Program file: Section III. Defines and typedefs: order of appearance -> constant macros, function macros,
 * 				typedefs and then enums.
 * Naming convention: Use upper case and words joined with an underscore (_). Limit  the  use  of  abbreviations.
 * Constants: define and use constants, rather than using numerical values; it make code more readable, and easier
 * 			  to modify.
 */

#define HTML_ELEMENT_LABEL_SIZE 							40		/*!Label size (characters) for HTML elements on web-page*/
#define WEB_PAGE_ELEMENTS 									10		/*!Number of elements on web-page*/
#define WEB_TITLE_SIZE 										128		/*!Title size (characters) for web-page/menu-title*/

/*!\brief Data structure to hold web-server configuration parameters.
 *
 * \details Data structure to hold web-server configuration parameters.
 *
 */
typedef struct _WIFI_SERVER {
	uint16_t server_port; 													/*!<HTTP port*/
	uint8_t server_protocol;												/*!<Protocol; default is TCP, valid values are defined by PROTOCOLS*/
} WIFI_SERVER;

WIFI_SERVER wifi_server;													/*!<Varaible to hold web-server configuration parameter values*/


/*!\brief Data structure to client configuration parameters.
 *
 * \details Data structure to client configuration parameters.
 *
 */
typedef struct _WIFI_CLIENT {
	uint8_t client_socket; 													/*!<SOCKET -> 0 to MAX_SOCKET_NUMBER*/
} WIFI_CLIENT;

WIFI_CLIENT wifi_client;													/*!<Varaible to hold client configuration parameter values*/

/*!
 * \brief Web server status
 *
 *
 * \details Status of web server active or not.
 *
 */
typedef enum{
	WEB_SERVER_NOT_ACTIVE												= 0,	/*!<Web server not active*/
	WEB_SERVER_ACTIVE													= 1		/*!<Web server active*/
} WEB_SERVER_STATUS;


/*!\brief Data structure to hold detail of a HTML element.
 *
 * \details Data structure to hold detail of a HTML element.
 * \note element_identifier - single character, unique for each item; with label to display on webpage.
 *
 */
typedef struct _HTML_ELEMENT_CHOICE {
	char element_identifier; 												/*!<HTML element/entry identifier, single character. This will be returned via GET method as client choice*/
	char element_label[HTML_ELEMENT_LABEL_SIZE];													/*!<HTML element label on web-page*/

} HTML_ELEMENT_CHOICE;


/*!\brief Data structure to hold detail of HTML web-page
 *
 * \details Data structure to hold detail of HTML web-page
 * \note element_identifier - single character, unique for each item; with label to display on web-page.
 * \warning There can be only one type of element with 10 values. Like, a drop-down list with 10 entries or group of 10 radio buttons.
 *
 */
typedef struct _HTML_WEB_PAGE {
	char page_title[WEB_TITLE_SIZE];									/*!<HTML web-page title*/
	char menu_title[WEB_TITLE_SIZE];									/*!<HTML menu title*/
	HTML_ELEMENT_TYPE element_type;											/*!<HTML element type for the web-page*/
	HTML_ELEMENT_CHOICE web_page_elements[WEB_PAGE_ELEMENTS];				/*!<HTML elements for web-page*/
	uint8_t element_count;													/*!<HTML element count added*/
} HTML_WEB_PAGE;

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

HTML_WEB_PAGE client_web_page;												/*!<Varaible to hold HTML client web-page*/
char client_response_buffer[RING_BUFFER_SIZE] = "\0";										/*!<Circular buffer/Variable to hold client response captured. This can be used as needed to initiate specific action/process*/
uint8_t client_response_buffer_write_pointer = 0;							/*!<Write pointer*/
uint8_t client_response_buffer_read_pointer = 0;							/*!<Read pointer*/
WEB_SERVER_STATUS web_server_status = WEB_SERVER_NOT_ACTIVE;				/*!<Web server status*/

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

void initialize_web_server(uint16_t port, uint8_t protocol);

/*---------------------------------------  ENTRY POINTS  ---------------------------------------------------------*/
/*define your entry points here*/

/*(Doxygen help: use \brief to provide short summary, \details for detailed description and \param for parameters */


/*!\brief Configure web-page.
 *
 * \details Configure web-page with details of web-page title, HTML element type.
 * Default values are:
 *	element_type = HTML_DROPDOWN_LIST;;
 *	page_title = "Client Web Page";
 *
 * @param page_title - string defining page title, maximum 127 characters
 * @param menu_title - string defining menu title, maximum 127 characters
 * @param element_type - choice of HTML element, drop-down list or radio button. define by HTML_ELEMENT_TYPE
 *
 */
void configure_web_page(char *page_title, char *menu_title, HTML_ELEMENT_TYPE element_type){
	/*Set element type*/
	if (element_type != HTML_DROPDOWN_LIST && element_type != HTML_RADIO_BUTTON){
		client_web_page.element_type = HTML_DROPDOWN_LIST;
	}
	else{
		client_web_page.element_type = element_type;
	}
	/*Set page title*/
	if (strlen(page_title) < WEB_TITLE_SIZE  && strlen(page_title) > 0){
		strncpy(client_web_page.page_title, page_title, strlen(page_title));
	}else if(strlen(page_title) <= 0){
		strcpy(client_web_page.page_title, "Client Web Page");
	}else{
		strncpy(client_web_page.page_title, page_title, WEB_TITLE_SIZE);
	}
	/*Set menu title*/
	if (strlen(menu_title) < WEB_TITLE_SIZE  && strlen(menu_title) > 0){
		strncpy(client_web_page.menu_title, menu_title, strlen(menu_title));
	}else if(strlen(menu_title) <= 0){
		strcpy(client_web_page.menu_title, "Menu/options");
	}else{
		strncpy(client_web_page.menu_title, menu_title, WEB_TITLE_SIZE);
	}
	client_web_page.element_count = 0;
	strcpy(client_response_buffer, "");
	client_response_buffer_write_pointer = 0;
	client_response_buffer_read_pointer = 0;
	#if SET_WEB_SERVER_TERMINAL_OUTPUT_ON == 1
		/*Send message to serial terminal*/
		usart_xfprint(SERIAL_TERNMINAL, (uint8_t *) "\n\rWeb Page: configured....\n\r");
	#endif
}


/*!\brief Add an element entry/choice.
 *
 * \details Configure web-page with details of web-page title, HTML element type.
 * Default values are:
 *	element_type = HTML_DROPDOWN_LIST;;
 *	page_title = "Client Web Page";
 *
 * @param choice_identifier - a single character identifier for element/choice, this will be return value via GET method
 * @param element_label - label for element/choice, maximum 40 characters
 *
 */
void add_element_choice(char choice_identifier, char *element_label){
	uint8_t loop_counter = 0, choice_identifier_exists = 0;
	for(loop_counter = 0; loop_counter <= client_web_page.element_count ; loop_counter++){
		if (choice_identifier == client_web_page.web_page_elements[loop_counter].element_identifier){
			choice_identifier_exists = 1;
			break;
		}
	}
	if(choice_identifier_exists == 1){
		#if SET_WEB_SERVER_TERMINAL_OUTPUT_ON == 1
			/*Send message to serial terminal*/
			usart_xfprint(SERIAL_TERNMINAL, (uint8_t *) "\n\rWeb Page: element choice identifier already exists....\n\r");
		#endif
	}else{
		if (client_web_page.element_count < 10){
			client_web_page.web_page_elements[client_web_page.element_count].element_identifier = choice_identifier;
			if (strlen(element_label) < HTML_ELEMENT_LABEL_SIZE && strlen(element_label) > 0){
				strncpy(client_web_page.web_page_elements[client_web_page.element_count].element_label , element_label, strlen(element_label));
			}else if(strlen(element_label) <= 0){
				strcpy(client_web_page.web_page_elements[client_web_page.element_count].element_label, "Client choice");
			}else{
				strncpy(client_web_page.web_page_elements[client_web_page.element_count].element_label , element_label, HTML_ELEMENT_LABEL_SIZE);
			}
			client_web_page.element_count++;
			#if SET_WEB_SERVER_TERMINAL_OUTPUT_ON == 1
				/*Send message to serial terminal*/
				usart_xfprint(SERIAL_TERNMINAL, (uint8_t *) "\n\rWeb Page: element added....\n\r");
			#endif
		}else{
			#if SET_WEB_SERVER_TERMINAL_OUTPUT_ON == 1
				/*Send message to serial terminal*/
				usart_xfprint(SERIAL_TERNMINAL, (uint8_t *) "\n\rWeb Page: can't add element, max 10 allowed....\n\r");
			#endif
		}
	}
}


/*!\brief Start the web-server.
 *
 * \details Initializes and start the web-server, web-sever starts to listen to clients
 * at sockets within range 0 to MAX_SOCKET_NUMBER.
 * \note Only one socket will be made active at a time.
 *
 * Default values are:
 *	server port = 80;
 *	server protocol = PROTOCOL_TCP;
 *
 * \warning Ensure web-page is configured and required elements are added, before starting/activating sever.
 *
 *
 */
void start_web_server(void){
	uint16_t port = SERVER_PORT;
	uint8_t protocol = SERVER_PROTOCOL;

	//COMMAND_OUTCOME command_outcome;

	if (client_web_page.element_count > 0){
		/*Initialize the server*/
		initialize_web_server(port, protocol);

		/*Search for available socket, activate and start to listen incoming connection*/
		for (TCP_SOCKET socket  = 0; socket < MAX_SOCKET_NUMBER; socket++){
			if (gs_get_socket_status(socket) == SOCKET_STATUS_CLOSED){
				if (wifi_server.server_protocol == PROTOCOL_TCP){
					gs_configure_socket(socket, wifi_server.server_protocol, wifi_server.server_port);
					//command_outcome = gs_enable_activate_socket(socket);
					gs_enable_activate_socket(socket);
					wifi_client.client_socket = socket;
					#if SET_WEB_SERVER_TERMINAL_OUTPUT_ON == 1
						/*Send message to serial terminal*/
						usart_xfprint(SERIAL_TERNMINAL, (uint8_t *) "\n\rWeb Server: Started....\n\r");
					#endif
					web_server_status = WEB_SERVER_ACTIVE;
					break;
				}
			}
		}
	}else{
		#if SET_WEB_SERVER_TERMINAL_OUTPUT_ON == 1
			/*Send message to serial terminal*/
			usart_xfprint(SERIAL_TERNMINAL, (uint8_t *) "\n\rWeb Server: can't start, web-page empty....\n\r");
		#endif
	}
}


/*!\brief Process client request.
 *
 * \details accepts incoming connection on socket, sends the web-page and
 * reads the client response
 * \warning Ensure web-page is configured and web server is started before calling this routine/function.
 *
 *
 */
void process_client_request(void){

	char data_string[128] ="\0";
	char html_string[128] ="\0";
	//COMMAND_OUTCOME command_outcome;
	//SUCCESS_ERROR result = ERROR;
	char *find_GET_in_response = "\0";
	char client_response = ' ';
	uint8_t loop_counter = 0;

	if (web_server_status == WEB_SERVER_ACTIVE){
		if (gs_get_socket_status(wifi_client.client_socket) == SOCKET_STATUS_LISTEN){
			//result = gs_read_data_from_socket(data_string); //accept connection, get CID
			gs_read_data_from_socket(data_string); //accept connection, get CID
			if(strlen(data_string) > 0){
				find_GET_in_response = strstr(data_string, "GET");
				if (*(find_GET_in_response + 5) == '?'){
					client_response = *(find_GET_in_response + 8);

					//usart_printf_P(PSTR("\r\nClient request-> %d"), client_response );

					/*Add to circular bugger for processing*/
					client_response_buffer[client_response_buffer_write_pointer] = client_response;
					client_response_buffer_write_pointer++;
					if (client_response_buffer_write_pointer >= RING_BUFFER_SIZE){
						client_response_buffer_write_pointer = 0;
					}
				}
			}
			if(gs_get_socket_status(wifi_client.client_socket) == SOCKET_STATUS_ESTABLISHED){
				//HTML header
				gs_write_data_to_socket(wifi_client.client_socket, "HTTP/1.1 200 OK\n");
				gs_write_data_to_socket(wifi_client.client_socket, "Content-Type: text/html\n\n");
				gs_write_data_to_socket(wifi_client.client_socket, "<!DOCTYPE HTML>\n\n");
				//Send web page HTML script/code
				gs_write_data_to_socket(wifi_client.client_socket, "<html> \n");
				gs_write_data_to_socket(wifi_client.client_socket, "<head> \n");
				/*Page title*/
				//gs_write_data_to_socket(wifi_client.client_socket, "<title>" + client_web_page.page_title + "</title> \n");
				strcpy(html_string, "<title>");
				strcat(html_string, client_web_page.page_title);
				strcat(html_string, "</title> \n");
				gs_write_data_to_socket(wifi_client.client_socket, html_string);
				gs_write_data_to_socket(wifi_client.client_socket, "</head> \n");
				gs_write_data_to_socket(wifi_client.client_socket, "<body> \n");
				/*Page title*/
				//gs_write_data_to_socket(wifi_client.client_socket, "<center><h1>" + client_web_page.page_title + "</h1> \n\n");
				strcpy(html_string, "<center><h1>");
				strcat(html_string, client_web_page.page_title);
				strcat(html_string, "</h1> \n");
				gs_write_data_to_socket(wifi_client.client_socket, html_string);
				strcpy(html_string, "<center><h3>");
				strcat(html_string, client_web_page.menu_title);
				strcat(html_string, "</h3> \n\n");
				gs_write_data_to_socket(wifi_client.client_socket, html_string);
				gs_write_data_to_socket(wifi_client.client_socket, "<p> \n");
				gs_write_data_to_socket(wifi_client.client_socket, "<form method=\"get\" action=\"\"> \n");
				/*Check for element type*/
				if (client_web_page.element_type == HTML_DROPDOWN_LIST ){
					gs_write_data_to_socket(wifi_client.client_socket, "<select name=\"l\"> \n");
					/*Add the elements*/
					for (loop_counter = 0; loop_counter < client_web_page.element_count; loop_counter++){
						//gs_write_data_to_socket(wifi_client.client_socket, "<option value=\"" + client_web_page.web_page_elements[loop_counter].element_identifier + "\">" + client_web_page.web_page_elements[loop_counter].element_label + "</option> \n");
						strcpy(html_string, "<option value=\"");
						strncat(html_string, &client_web_page.web_page_elements[loop_counter].element_identifier, 1);
						strcat(html_string, "\">");
						strcat(html_string, client_web_page.web_page_elements[loop_counter].element_label);
						strcat(html_string, "</option> \n");
						gs_write_data_to_socket(wifi_client.client_socket, html_string);
					}
					gs_write_data_to_socket(wifi_client.client_socket, "</select> \n");
				}else if (client_web_page.element_type == HTML_RADIO_BUTTON){
					for (loop_counter = 0; loop_counter < client_web_page.element_count; loop_counter++){
						//gs_write_data_to_socket(wifi_client.client_socket, "<input type=\"radio\" name=\"choice\" value=\"" + client_web_page.web_page_elements[loop_counter].element_identifier + "\">" + client_web_page.web_page_elements[loop_counter].element_label + " \n");
						strcpy(html_string, "<input type=\"radio\" name=\"choice\" value=\"");
						strncat(html_string, &client_web_page.web_page_elements[loop_counter].element_identifier, 1);
						strcat(html_string, "\">");
						strcat(html_string, client_web_page.web_page_elements[loop_counter].element_label);
						strcat(html_string, " \n");
						gs_write_data_to_socket(wifi_client.client_socket, html_string);
					}
				}else{
					gs_write_data_to_socket(wifi_client.client_socket, "<center><h3> No valid elements added, please check! </h3> \n\n");
				}
				gs_write_data_to_socket(wifi_client.client_socket, "<input type=\"submit\" value=\"Set\"> \n");
				gs_write_data_to_socket(wifi_client.client_socket, "</form> \n");
				gs_write_data_to_socket(wifi_client.client_socket, "</p> \n");
				gs_write_data_to_socket(wifi_client.client_socket, "</center> \n");
				gs_write_data_to_socket(wifi_client.client_socket, "</body> \n");
				gs_write_data_to_socket(wifi_client.client_socket, "</html>");
				gs_write_data_to_socket(wifi_client.client_socket, "");

				//command_outcome = gs_reset_socket(wifi_client.client_socket);
				gs_reset_socket(wifi_client.client_socket);

				gs_flush();

				/*Wait for web browser to get refresh*/
				_delay_ms(100);
			}
		}
	}

}




/*!\brief Get the client response (next)
 *
 * \details reads the buffer and returns the next response of client,
 * i.e. single character received from web-page of choice.
 * \note After reading the position, initializes with blank.
 *
 * @return - a single character response according to choice of client on web -page.
 *
 */
char get_next_client_response(void){
	char client_response  = ' ';
	/*read the current read location from buffer*/
	client_response = client_response_buffer[client_response_buffer_read_pointer];
	/*initialize with null*/
	client_response_buffer[client_response_buffer_read_pointer] = ' ';
	/*move the pointer*/
	client_response_buffer_read_pointer++;
	/*Ensure remains in bound for circular buffer*/
	if (client_response_buffer_read_pointer >= RING_BUFFER_SIZE){
		client_response_buffer_read_pointer = 0;
	}
	return client_response;
}
/*---------------------------------------  LOCAL FUNCTIONS  ------------------------------------------------------*/
/*define your local functions here*/

/*(Doxygen help: use \brief to provide short summary, \details for detailed description and \param for parameters */


/*!\brief Initialize web-server.
 *
 * \details Initialize the web-server with default configuration.
 * Default values are:
 *	server port = 80;
 *	server protocol = PROTOCOL_TCP;
 * \note This function does not start the web-server, it initializes
 * the configuration parameters
 *
 * @param port - HTTP port, default is 80
 * @param protocol - protocol, default is PROTOCOL_TCP
 *
 */
void initialize_web_server(uint16_t port, uint8_t protocol){
	wifi_server.server_port = 80;
	wifi_server.server_protocol = PROTOCOL_TCP;
	#if SET_WEB_SERVER_TERMINAL_OUTPUT_ON == 1
		/*Send message to serial terminal*/
		usart_xfprint(SERIAL_TERNMINAL, (uint8_t *) "\n\rWeb Server: Initialized....\n\r");
	#endif
}


/*---------------------------------------  ISR-Interrupt Service Routines  ---------------------------------------*/
/*define your Interrupt Service Routines here*/

/*(Doxygen help: use \brief to provide short summary, \details for detailed description and \param for parameters */

/*NO ISR's */

/*!@}*/   // end module
