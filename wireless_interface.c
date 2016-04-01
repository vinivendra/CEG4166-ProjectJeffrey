/*
 * wireless_interface.c
 *
 *
 *  Created on: Mar 27, 2016
 *      Author: shailendrasingh
 */

/****************************************************************************//*!
 * \defgroup wireless_interface  Module Wireless Interface
 * @{
******************************************************************************/



/******************************************************************************************************************/
/* CODING STANDARDS:
 * Program file: Section I. Prologue: description about the file, description author(s), revision control
 * 				information, references, etc.
 */

/*(Doxygen help: use \brief to provide short summary and \details command can be used)*/

/*!	\file wireless_interface.c
 * 	\brief This file defines and implements the functions responsible for interface with Gainspan GS1011M WiFi
 * 	module and Web server to interact with clients connecting over WiFi.
 *
 *
 * \details
 * Gainspan GS1011M WiFi: Defines the functions responsible for interface with WiFi shields based on Gainspan
 * GS1011M module. Provides functions to configure, activate Limited AP hot-spot, and interact or communicate using sockets.
 *
 * \note Module uses \ref usartAsyncModule to communicate with Gainspan GS1011M module over serial communication.
 * It also communicates the progress to serial terminal. Hence, it requires two USARTs, for Gainspan and serial
 * terminal communication respectively.
 *
 * \note Default: USART0 is for serial terminal and USART2 for Gainspan communication.
 *
 * Web Server : Limited function web server to interact with client connected over Gainspan module based
 * Wi-Fi network. A simple HTML page with elements such as drop down list (HTML_DROPDOWN_LIST)
 * or radio buttons (HTML_RADIO_BUTTON) can be created under a menu. Titles for page and menu can be
 * defined.
 *
 * Client response (single character for each event) are stored in a ring buffer, with size of
 * RING_BUFFER_SIZE.
 *
 * \note Web-server can be accessed via default host ip 192.168.3.1 over HTTP i.e. use a web browser
 * to access the web-page/home page via host ip 192.168.3.1
 *
 * \note This module uses \ref usartAsyncModule.
 *
 * \note Default network configuration :
 * 			IP 		- 192.168.3.1
 * 			Subnet  - 255.255.255.0
 *			Gateway	- 192.168.3.1
 *
 * \note Module takes approximately 4200ms (4.2s) to complete the initialization.
 *
 * \warning Ensure interrupts are enable before initializing the module.
 *
 * \warning Ensure switch S3 (WiFi USART:SW/HW)	on Hydrogen WiFi Shield is at USART:Sw/HW position.
 *
 * \warning Ensure to define an unique SSID, to avoid conflict with neighbouring networks.
 *
 * Usage guide (For "Limited AP" or hot-spot mode):
 *
 * 		=> Set SET_GAINSPAN_TERMINAL_OUTPUT_ON to 1 in "gainspan_gs1011m.h" to get the command response/progress on
 * 			serial terminal.
 *
 * 		=> Set SET_WEB_SERVER_TERMINAL_OUTPUT_ON to 1 in "web_server.h" to get server logs on serial terminal.
 *
 * 		=> Initialize USART0 and USART2
 *
 * 		=> Call API function gs_initialize_module(USART_ID target_usart_id, BAUD_RATE target_baud_rate, USART_ID target_serial_terminal_usart_id, BAUD_RATE target_serial_terminal_baud_rate)
 * 			to initialize the module with default parameters.
 *
 * 			Example: gs_initialize_module(usart_two, BAUD_RATE_9600, usart_zero, BAUD_RATE_115200);
 *
 * 		=> Call gs_set_wireless_ssid(char *wireless_ssid) to assign SSID for your WiFi network, alternately
 * 			you can call gs_set_wireless_configuration(WIRELESS_PROFILE target_wireless_profile) and
 * 			gs_set_network_configuration(NETWORK_PROFILE target_network_profile) to provide detailed configuration
 * 			parameters for network and wireless configuration.
 *
 * 			Example: gs_set_wireless_ssid("WifiTeamX")
 *
 * 		=> Call gs_activate_wireless_connection(), to activate wireless network with configuration parameters
 *			defined in earlier step. Status will be returned defined by GAINSPAN_ACTIVE, which you can verify.
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
 * \warning In case system does not initialize successfully, either reset the hardware or reload the source code
 * and try again.
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
/* FreeRTOS includes */
#include "FreeRTOS.h" 						/* for various kernel functions */

#include <stdio.h>							/* for text string formatting functions */
#include <string.h>
#include <stdlib.h>

#include <avr/io.h>
#include <util/delay.h>

/* module includes */
#include "wireless_interface.h"				/* module include */


/******************************************************************************************************************/
/* CODING STANDARDS:
 * Program file: Section III. Defines and typedefs: order of appearance -> constant macros, function macros,
 * 				typedefs and then enums.
 * Naming convention: Use upper case and words joined with an underscore (_). Limit  the  use  of  abbreviations.
 * Constants: define and use constants, rather than using numerical values; it make code more readable, and easier
 * 			  to modify.
 */

/*Number of characters to be read from response from Gainspan*/
#define CHARACTERS_TO_READ_FROM_GAINSPAN_RESPONSE 						128							/*!<Number of characters to read from response from Gainspan module*/
#define GENERAL_SIZE 													128							/*!<Number of characters for SSID, ID, and passwords*/
#define IP_SIZE 														15							/*!<Number of characters for IP, Subnet, gateway*/
/*Polling interval, after issuing command, to check availability of response from Gainspan*/
#define COMMAND_RESPONSE_POLLING_INTERVAL_IN_MILLISECONDS				5							/*!<Polling interval, after issuing command, to check availability of response from Gainspan*/

#define HTML_ELEMENT_LABEL_SIZE 										40							/*!<Label size (characters) for HTML elements on web-page*/
#define WEB_PAGE_ELEMENTS 												10							/*!<Number of elements on web-page*/
#define WEB_TITLE_SIZE 													128							/*!<Title size (characters) for web-page/menu-title*/
#define MIN(X, Y) 														((X) < (Y) ? (X) : (Y)) 	/*!<Min of two numbers*/
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
	char *element_label;													/*!<HTML element label on web-page*/
//	char element_label[HTML_ELEMENT_LABEL_SIZE];													/*!<HTML element label on web-page*/

} HTML_ELEMENT_CHOICE;


/*!\brief Data structure to hold detail of HTML web-page
 *
 * \details Data structure to hold detail of HTML web-page
 * \note element_identifier - single character, unique for each item; with label to display on web-page.
 * \warning There can be only one type of element with 10 values. Like, a drop-down list with 10 entries or group of 10 radio buttons.
 *
 */
typedef struct _HTML_WEB_PAGE {

	char *page_title;														/*!<HTML web-page title*/
	char *menu_title;														/*!<HTML menu title*/
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

/*Command table for Gainspan GS1011M*/
/*!
 * \brief Gainspan module command table;
 *
 *
 * \details Valid commands implemented in this software module.
 *
 */
const char *gs_at_commands[] = {
		/*Serial-to-WiFi profile configuration*/
		"AT",											/*OK*/
		"ATE0",											/*Echo off for all inputs*/
		"ATV1",											/*Verbose responses are enabled. The status response is in the form of ASCII strings*/
		"ATB=",											/*Set the UART parameters:<baudrate>[[,<bitsperchar>][,<parity>][,<stopbits>]]; example-115200,8,n,1*/
		"ATI0",											/*Get OEM identification*/
		"ATI1",											/*Get hardware version*/
		"ATI2",											/*Get software version*/
		/*WiFi interface configuration*/
		"AT+NMAC=?",									/*Get MAC address of device*/
		"AT+WS=",										/*Scan for network: <SSID>*/
		"AT+WM=",										/*Set wireless mode: 0-infrastructure, 1-ad hoc, 2-limited ap*/
		"AT+WA=",										/*Associate with a Network, or Start an Ad Hoc or Infrastructure (AP) Network, parameters-<SSID>[,[<BSSID>][,<Ch>],{Rssi Flag]]*/
		"AT+WD",										/*Disassociate from current network*/
		"AT+NSTAT=?",									/*Get information about the current network status-MAC, WLAN, Mode, BSSID, SSID, Channel, Security, RSSI, Network configuration, Rx count, Tx count*/
		"AT+WSTAT=?",									/*Get information about the current wireless network status-Mode, BSSID, SSID, Channel, Security*/
		"AT+WRSSI=?",									/*Get RSSI in dBm*/
		"AT+WRATE=",									/*Set transmit rate:0-Auto, 2-1 Mbps, 4-2 Mbps, 1-5.5 Mbps, 22-11 Mbps*/
		"AT+WRATE=?",									/*Get transmit rate*/
		/*WiFi Security Configuration	*/
		"AT+WAUTH=",									/*Set authentication mode - 0-None, 1-WEP Open, 2-WEP Shared*/
		"AT+WSEC=",										/*Set wireless security configuration: 0-Auto security (All), 1-Open security, 2-WEP security, 4-Wpa-psk security, 8-WPA2-PSK security, 16-WPA Enterprise, 32-WPA2 Enterprise*/
		"AT+WWPA=",										/*Set WPA passphrase value: strin 8-63 characters*/
		"AT+WPAPSK=",									/*Compute and store WPA2 PSK value from SSID and Passkey*/
		"AT+WRXACTIVE=0",								/*Disable (0) 802.11 radio receiver*/
		"AT+WRXACTIVE=1",								/*Enable (1) 802.11 radio receiver*/
		"AT+WRXPS=0",									/*Disable (0) 802.11 Power Saver Mode, by informing AP, AP shall buffer all the incoming unicast traffic during this time.*/
		"AT+WRXPS=1",									/*Enable (1) 802.11 Power Saver Mode, by informing AP, AP shall buffer all the incoming unicast traffic during this time.*/
		/*Network interface*/
		"AT+NDHCP=0",									/*Disable (0) DHCP for IPv4*/
		"AT+NDHCP=1",									/*Enable (1) DHCP for IPv4*/
		"AT+NSET=",										/*Set static network parameters for IPv4:<Src Address>,<Net-mask>,<Gateway>*/
		"AT+DHCPSRVR=0",								/*Stop (0) DHCP Server IPv4*/
		"AT+DHCPSRVR=1",								/*Start (1) DHCP Server IPv4*/
		"AT+DNS=0",										/*Stop (0) DNS Server*/
		"AT+DNS=1",										/*Start (1) DNS Server:<Start/stop>,<url>*/
		"AT+DNSLOOKUP=",								/*DNS lookup:<URL>,[<RETRY>,<TIMEOUT-S>,<CLEAR CACHE ENTRY>]*/
		/*GSLink*/
		"AT+WEBSERVER=0",								/*Stop (n=0) web serve*/
		"AT+WEBSERVER=1",								/*Start (n=1) web serve n,<user name>,<password>,[1=SSL enable/0=SSL disable],[idle timeout],[Response timeout]*/
		"AT+XMLPARSE=0", 								/*Disable (0) XML Parser on HTTP Data*/
		"AT+XMLPARSE=1", 								/*Enable (1) XML Parser on HTTP Data*/
		/*Connection management configuration*/
		"AT+NSTCP=",									/*Start the TCP server connection with IPv4 address:<Port>,[max client connection (1-15)]*/
		"AT+NCTCP=",									/*Create a TCP client connection to the remote server with IPv4:<Dest-Address>,<Port> */
		"AT+NSUDP=",									/*Start the UDP server connection with IPv4 address:<Port>*/
		"AT+NCUDP=",									/*Create a UDP client connection to the remote server with IPv4:<Dest-Address>,<Port>[<,Src.Port>]*/
		"AT+NCLOSE=",									/*Close the connection associated with current active socket by identifying CID:<CID>*/
		"TCP_RESPONSE" 									/*This is not a command, it is used to identify and send message to serial/terminal*/
		"AT_COMMAND_INVALID" 							/*Not a command, it is an identifier for invalid command*/
		/*Provisioning*/
		//"AT+WEBPROV=",								/*Start support provisioning through web pages:<user name>,<password>[,SSL Enabled,Param StoreOption,idletimeout,ncmautoconnect]*/
		//"AT+WEBPROVSTOP",								/*Stop support provisioning through web pages*/
};


/*Structure holds Gainspan device socket table*/
/*!
 * \brief Gainspan device socket table.
 *
 *
 * \details Holds socket connection parameters for Gainspan clients.
 *
 */
typedef struct _SOCKET_TABLE {
	SOCKET_STATUS status;																/*!<Socket status*/
	PROTOCOLS protocol;																	/*!<Socket protocol*/
	char * ip_address;																	/*!<Socket protocol*/
	TCP_PORT port;																		/*!<Socket port*/
	uint8_t cid;																		/*!<Socket cid*/
} SOCKET_TABLE;


/*!
 * \brief Gainspan module;
 *
 *
 * \details Holds configuration parameters for Gainspan WiFi module.
 *
 */
typedef struct _GAINSPAN {
	/*USART interface: MCU USART connected to Gainspan*/

	USART_ID serial_terminal_usart_id;  												/*!<MCU USART port connected to serial terminal: to display progress messages*/
	BAUD_RATE serial_terminal_baud_rate;												/*!<Baud rate for MCU USART port connected to serial terminal*/

	USART_ID usart_id; 																	/*!<MCU USART port connected to Gainspan WiFi module*/
	BAUD_RATE baud_rate;																/*!<Baud rate for MCU USART port connected to Gainspan WiFi module*/

	/*Device connection status*/
	GAINSPAN_ACTIVE device_connection_status;											/*!<Gainspan device activation or connection status*/

	/*Wireless configuration*/
	char *ssid;																			/*!<Gainspan wireless configuration: SSID*/
	char *security_key;																	/*!<Gainspan wireless configuration: Secirty Key*/
	WIRELESS_MODE wireless_mode;														/*!<Gainspan wireless configuration: Wireless Mode*/
	AUTHENTICATION_MODE authentication_mode;											/*!<Gainspan wireless configuration: Authentication Mode*/
	WIRELESS_SECURITY_CONFIGURATION wireless_security_configuration;					/*!<Gainspan wireless configuration: Security Configuration*/
	TRANSMISSION_RATE transmission_rate;												/*!<Gainspan wireless configuration: Transmission Rate*/
	WIRELESS_CHANNEL wireless_channel;													/*!<Gainspan wireless configuration: Channel*/

	/*Network configuration*/
	char *local_ip_address;																/*!<Gainspan network configuration: Local device IP address*/
	char *subnet;																		/*!<Gainspan network configuration: Subnet*/
	char *gateway;																		/*!<Gainspan network configuration: Gateway*/
	PROTOCOLS server_protocol;															/*!<Gainspan network configuration: Protocol - TCP/UDP*/
	TCP_PORT server_port;																/*!<Gainspan network configuration: Port - TCP/UDP*/
	uint8_t server_number_of_connection;												/*!<Gainspan network configuration: Connection i.e. clients for TCP/UDP*/

	/*Gainspan web-server authentication parameters*/
	char *web_server_administrator_id;													/*!<Gainspan web-server authentication: Administrator ID*/
	char *web_server_administrator_password;											/*!<Gainspan web-server authentication: Administrator Password*/

	/*Client connection parameters*/
	uint8_t server_cid;																	/*!<Socket cid for TCP Server*/
	SOCKET_TABLE socket_table[MAX_SOCKET_NUMBER];										/*!<Socket Table*/
	TCP_SOCKET socket_with_data;														/*!<Socket with valid data available.*/
	TCP_SOCKET active_socket;															/*!<Socket active for current communication. Needs to be modified by external module to ensure proper communication*/
	uint8_t active_client_cid;															/*!<Socket cid for Active Client*/

	/*Device operation mode*/
	GAINSPAN_DEVICE_OPERATION_MODE device_operation_mode;								/*!<Device operation mode: GAINSPAN_DEVICE_MODE_COMMAND, GAINSPAN_DEVICE_MODE_DATA, or GAINSPAN_DEVICE_MODE_DATA_RX*/

	/*Data transmission flag/indicator*/
	BOOLEAN_DATA data_transmission_completed; 											/*!<Data transmission status - BOOLEAN_TRUE or BOOLEAN_FALSE, default values BOOLEAN_TRUE indicates there is no data  */
} GAINSPAN;


/*Structure holds gainspan interface parameter*/

/*!
 * \brief Gainspan data structure;
 *
 *
 * \details Structure holds Gainspan interface parameters.
 *
 */
GAINSPAN gainspan;																		/*!<Gainspan data structure*/


HTML_WEB_PAGE client_web_page; 															/*!<Varaible to hold HTML client web-page*/
char *client_response_buffer;															/*!<Circular buffer/Variable to hold client response captured. This can be used as needed to initiate specific action/process*/
uint8_t client_response_buffer_write_pointer = 0;										/*!<Write pointer*/
uint8_t client_response_buffer_read_pointer = 0;										/*!<Read pointer*/
WEB_SERVER_STATUS web_server_status = WEB_SERVER_NOT_ACTIVE;							/*!<Web server status*/


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

void gs_initialize_gainspan(void);

void gs_send_command(AT_COMMAND at_command);

uint16_t gs_get_command_response(char *gs_command_response, uint16_t polling_period_in_milliseconds);

COMMAND_OUTCOME gs_parse_command_response(char *gs_command_response);

COMMAND_OUTCOME gs_parse_command_response_tcp(char *gs_command_response, SOCKET_MODE socket_mode, AT_COMMAND at_command);

void gs_send_command_response_to_serial_terminal(AT_COMMAND at_command, COMMAND_OUTCOME command_result);

void gs_send_activation_status_to_serial_terminal(GAINSPAN_ACTIVE gs_active);

void initialize_web_server(uint16_t port, uint8_t protocol);

uint8_t hex_to_int(char character);

char int_to_hex(uint8_t character);


/*---------------------------------------  ENTRY POINTS  ---------------------------------------------------------*/
/*define your entry points here*/

/*(Doxygen help: use \brief to provide short summary, \details for detailed description and \param for parameters */


/*!
 * \brief Initialize Gainspan module.
 *
 *
 * \details Sets the module parameters to default and sets the USARTS for communication.
 *
 *
 *
 * @param target_usart_id	- USART connected to Gainspan WiFi module.
 * @param target_baud_rate	- Baud rate for communication with Gainspan WiFi module.
 * @param target_serial_terminal_usart_id - USART connected to serial terminal.
 * @param target_serial_terminal_baud_rate - Baurd rate for communication with serial terminal.
 *
 */
void gs_initialize_module(USART_ID target_usart_id, BAUD_RATE target_baud_rate, USART_ID target_serial_terminal_usart_id, BAUD_RATE target_serial_terminal_baud_rate){
	/*Initialize the data structure*/
	gs_initialize_gainspan();
	/*set the USART*/
	gs_set_usart(target_usart_id, target_baud_rate, target_serial_terminal_usart_id, target_serial_terminal_baud_rate);
}


/*!
 * \brief Set USART parameters for Gainspan module.
 *
 *
 * \details Sets the USARTS for communication.
 *
 *
 *
 * @param target_usart_id	- USART connected to Gainspan WiFi module.
 * @param target_baud_rate	- Baud rate for communication with Gainspan WiFi module.
 * @param target_serial_terminal_usart_id - USART connected to serial terminal.
 * @param target_serial_terminal_baud_rate - Baud rate for communication with serial terminal.
 *
 */
void gs_set_usart(USART_ID target_usart_id, BAUD_RATE target_baud_rate, USART_ID target_serial_terminal_usart_id, BAUD_RATE target_serial_terminal_baud_rate){
	gainspan.usart_id = target_usart_id;
	gainspan.baud_rate = target_baud_rate;

	gainspan.serial_terminal_usart_id = target_serial_terminal_usart_id;
	gainspan.serial_terminal_baud_rate = target_serial_terminal_baud_rate;
}


/*!
 * \brief Set network configuration parameters for Gainspan module.
 *
 *
 * \details Set network configuration parameters such as device local ip address, subnet, and gateway for
 * Gainspan module, through structure NETWORK_PROFILE.
 *
 *
 *
 * @param target_network_profile - Structure having valid values for local ip address, subnet, and gateway.
 *
 */
void gs_set_network_configuration(NETWORK_PROFILE target_network_profile){
	strcpy(gainspan.local_ip_address, target_network_profile.local_ip_address);
	strcpy(gainspan.subnet, target_network_profile.subnet);
	strcpy(gainspan.gateway, target_network_profile.gateway);
}


/*!
 * \brief Set wireless configuration parameters for Gainspan module.
 *
 *
 * \details Set wireless configuration parameters such as SSID, security key, wireless mode, authentication
 * mode, security configuration, transmission rate, and wireless channel through structure WIRELESS_PROFILE.
 *
 *
 *
 * @param target_wireless_profile - Structure having valid values for SSID, security key, wireless mode, authentication mode, security configuration, transmission rate, and channel
 *
 */
void gs_set_wireless_configuration(WIRELESS_PROFILE target_wireless_profile){
	strcpy(gainspan.ssid, target_wireless_profile.ssid);
	strcpy(gainspan.security_key, target_wireless_profile.security_key);
	gainspan.wireless_mode = target_wireless_profile.wireless_mode;
	gainspan.authentication_mode = target_wireless_profile.authentication_mode;
	gainspan.wireless_security_configuration = 	target_wireless_profile.wireless_security_configuration;
	gainspan.transmission_rate = target_wireless_profile.transmission_rate;
	gainspan.wireless_channel = target_wireless_profile.wireless_channel;
}


/*!
 * \brief Set SSID for wireless configurations for Gainspan module.
 *
 *
 * \details Set wireless configuration parameters of SSID.
 *
 *
 *
 * @param wireless_ssid - valid SSID string
 *
 */
void gs_set_wireless_ssid(char *wireless_ssid){
	strcpy(gainspan.ssid, wireless_ssid);
}


/*!
 * \brief Set web server authentication parameters for Gainspan module.
 *
 *
 * \details Set web server authentication parameters such as administration ID and password through
 * structure WEBSERVER_AUTHENTICATION_PROFILE.
 *
 *
 *
 * @param target_webserver_profile - Structure having valid values for administrator ID and password.
 *
 */
void gs_set_webserver_authentication(WEBSERVER_AUTHENTICATION_PROFILE target_webserver_profile){
	strcpy(gainspan.web_server_administrator_id, target_webserver_profile.web_server_administrator_id);
	strcpy(gainspan.web_server_administrator_password, target_webserver_profile.web_server_administrator_password);
}


/*!
 * \brief Activate Gainspan WiFi device using the configuration parameters.
 *
 *
 * \details Activates Gaispan WiFi device using the configuration parameters from structure GAINSPAN.
 *
 * \note: Current implementation allows only Limited AP mode.
 *
 *
 * @return - activation status, valid values are defined by GAINSPAN_ACTIVE.
 *
 */
GAINSPAN_ACTIVE gs_activate_wireless_connection(void){
	char gs_command_response[MAX_TX_BUFFER] = "\0";
	GAINSPAN_ACTIVE gs_active = GAINSPAN_ACTIVE_FALSE;
	uint16_t number_of_characters_read = 0;
	COMMAND_OUTCOME command_result = COMMAND_OUTCOME_SUCCESS;
	uint8_t command_outcomes_success = 0, command_outcomes_errors = 0;

	#if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
		/*Send message to serial terminal*/
		usart_xfprint(gainspan.serial_terminal_usart_id, (uint8_t *) "\n\rGainspan Device: activation in progress....\n\r");
	#endif

	/*Test connection with device. Observed while testing that the first command always gets error; hence sending AT-OK two times*/
	strcpy(gs_command_response, "\0");
	gs_send_command(AT_OK);
	number_of_characters_read = gs_get_command_response(gs_command_response, 300);
	command_result = gs_parse_command_response(gs_command_response);
	#if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
		gs_send_command_response_to_serial_terminal(AT_OK, command_result);
	#endif
	/*do not include this towards outcome success or error*/

	strcpy(gs_command_response, "\0");
	gs_send_command(AT_OK);
	number_of_characters_read = gs_get_command_response(gs_command_response, 300);
	command_result = gs_parse_command_response(gs_command_response);
	#if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
		gs_send_command_response_to_serial_terminal(AT_OK, command_result);
	#endif
	if(command_result == COMMAND_OUTCOME_SUCCESS){
		command_outcomes_success++;
	}else{
		command_outcomes_errors++;
	}

	/*Echo off*/
	strcpy(gs_command_response, "\0");
	gs_send_command(AT_DISABLE_ECHO);
	number_of_characters_read = gs_get_command_response(gs_command_response, 300);
	command_result = gs_parse_command_response(gs_command_response);
	#if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
		gs_send_command_response_to_serial_terminal(AT_DISABLE_ECHO, command_result);
	#endif
	if(command_result == COMMAND_OUTCOME_SUCCESS){
		command_outcomes_success++;
	}else{
		command_outcomes_errors++;
	}

	/*Get Device MAC Address*/
	//Handle the response first
	/*
	strcpy(gs_command_response, "\0");
	gs_send_command(AT_GET_DEVICE_MAC_ADDRESS);
	number_of_characters_read = gs_get_command_response(gs_command_response, 300);
	command_result = gs_parse_command_response(gs_command_response);
	#if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
		gs_send_command_response_to_serial_terminal(AT_GET_DEVICE_MAC_ADDRESS, command_result);
	#endif
	(if(command_result == COMMAND_OUTCOME_SUCCESS){
		command_outcomes_success++;
	}else{
		command_outcomes_errors++;
	}*/

	/*Stop DHCP server*/
	strcpy(gs_command_response, "\0");
	gs_send_command(AT_STOP_DHCP_SERVER_IPV4);
	number_of_characters_read = gs_get_command_response(gs_command_response, 300);
	command_result = gs_parse_command_response(gs_command_response);
	#if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
		gs_send_command_response_to_serial_terminal(AT_STOP_DHCP_SERVER_IPV4, command_result);
	#endif
	if(command_result == COMMAND_OUTCOME_SUCCESS){
		command_outcomes_success++;
	}else{
		command_outcomes_errors++;
	}

	/*Dis-associate current network*/
	strcpy(gs_command_response, "\0");
	gs_send_command(AT_DISASSOCIATE_CURRENT_NETWORK);
	number_of_characters_read = gs_get_command_response(gs_command_response, 300);
	command_result = gs_parse_command_response(gs_command_response);
	#if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
		gs_send_command_response_to_serial_terminal(AT_DISASSOCIATE_CURRENT_NETWORK, command_result);
	#endif
	if(command_result == COMMAND_OUTCOME_SUCCESS){
		command_outcomes_success++;
	}else{
		command_outcomes_errors++;
	}

	/*Disable DHCP*/
	strcpy(gs_command_response, "\0");
	gs_send_command(AT_DISABLE_DHCP_IPV4);
	number_of_characters_read = gs_get_command_response(gs_command_response, 300);
	command_result = gs_parse_command_response(gs_command_response);
	#if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
		gs_send_command_response_to_serial_terminal(AT_DISABLE_DHCP_IPV4, command_result);
	#endif
	if(command_result == COMMAND_OUTCOME_SUCCESS){
		command_outcomes_success++;
	}else{
		command_outcomes_errors++;
	}

	if (gainspan.wireless_mode == WIRELESS_MODE_LIMITEDAP){
		/*Set network stack parameters*/
		strcpy(gs_command_response, "\0");
		gs_send_command(AT_SET_STATIC_NETWORK_PARAMTERS_IPV4);
		number_of_characters_read = gs_get_command_response(gs_command_response, 300);
		command_result = gs_parse_command_response(gs_command_response);
		#if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
			gs_send_command_response_to_serial_terminal(AT_SET_STATIC_NETWORK_PARAMTERS_IPV4, command_result);
		#endif
		if(command_result == COMMAND_OUTCOME_SUCCESS){
			command_outcomes_success++;
		}else{
			command_outcomes_errors++;
		}

		/*Set wireless mode*/
		strcpy(gs_command_response, "\0");
		gs_send_command(AT_SET_WIRELESS_MODE);
		number_of_characters_read = gs_get_command_response(gs_command_response, 300);
		command_result = gs_parse_command_response(gs_command_response);
		#if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
			gs_send_command_response_to_serial_terminal(AT_SET_WIRELESS_MODE, command_result);
		#endif
		if(command_result == COMMAND_OUTCOME_SUCCESS){
			command_outcomes_success++;
		}else{
			command_outcomes_errors++;
		}

		/*Create infrastructure network*/
		strcpy(gs_command_response, "\0");
		gs_send_command(AT_ASSOCIATE_START_NETWORK);
		number_of_characters_read = gs_get_command_response(gs_command_response, 1500);
		command_result = gs_parse_command_response(gs_command_response);
		#if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
			gs_send_command_response_to_serial_terminal(AT_ASSOCIATE_START_NETWORK, command_result);
		#endif
		if(command_result == COMMAND_OUTCOME_SUCCESS){
			command_outcomes_success++;
		}else{
			command_outcomes_errors++;
		}

		/*Start DHCP server*/
//		strcpy(gs_command_response, "\0");
		strcpy(gs_command_response, "");
		gs_send_command(AT_START_DHCP_SERVER_IPV4);
		number_of_characters_read = gs_get_command_response(gs_command_response, 300);
		command_result = gs_parse_command_response(gs_command_response);
		#if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
			gs_send_command_response_to_serial_terminal(AT_START_DHCP_SERVER_IPV4, command_result);
		#endif
		if(command_result == COMMAND_OUTCOME_SUCCESS){
			command_outcomes_success++;
		}else{
			command_outcomes_errors++;
		}

	}

	/*Determine the Gainspan activation status*/
	if(command_outcomes_success > 0 && command_outcomes_errors > 0){
		gs_active = GAINSPAN_ACTIVE_TRUE_WITH_ERRORS;
	}else if(command_outcomes_success > 0 && command_outcomes_errors == 0){
		gs_active = GAINSPAN_ACTIVE_TRUE;
	}else if(command_outcomes_success == 0 && command_outcomes_errors > 0){
		gs_active = GAINSPAN_ACTIVE_FALSE;
	}

	/*Send activation status to serial terminal*/
	#if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
		gs_send_activation_status_to_serial_terminal(gs_active);
	#endif

	gainspan.device_connection_status = gs_active;

	return gs_active;
}


/*!
 * \brief Get socket status.
 *
 *
 * \details retrieves socket status.
 *
 *
 * @param socket - valid socket number, limited by MAX_SOCKET_NUMBER.
 * @return - status of socket defined by SOCKET_STATUS values.
 *
 */
SOCKET_STATUS gs_get_socket_status(TCP_SOCKET socket){
	if ((socket >= MAX_SOCKET_NUMBER) || (socket < 0)){
		return SOCKET_STATUS_INVALID;
	}else {
		return gainspan.socket_table[socket].status;
	}
}


/*!
 * \brief Activate socket.
 *
 *
 * \details Activates socket for current communications. Checks if socket has been configured and
 * in state SOCKET_STATUS_INIT before activating it.
 *
 *
 * @param socket - valid socket number, limited by MAX_SOCKET_NUMBER.
 * @return - outcome, SUCCESS or ERROR; defined by SUCCESS_ERROR.
 *
 */
SUCCESS_ERROR gs_activate_socket(TCP_SOCKET socket){
	SOCKET_STATUS socket_status;

	socket_status = gs_get_socket_status(socket);
	if ((socket_status == SOCKET_STATUS_INIT) || (socket_status == SOCKET_STATUS_LISTEN) || (socket_status == SOCKET_STATUS_ESTABLISHED)){
		gainspan.active_socket = socket;
		gainspan.active_client_cid = gainspan.socket_table[socket].cid;
		return SUCCESS;
	}else{
		return ERROR;
	}
}


/*!
 * \brief Get current active socket.
 *
 *
 * \details Get current active socket for current communications.
 *
 *
 * @return - socket, which is currently active, values defined by SOCKET
 *
 */
TCP_SOCKET gs_get_active_socket(void){
	return gainspan.active_socket;
}


/*!
 * \brief Get socket protocol.
 *
 *
 * \details retrieves socket protocol.
 *
 *
 * @param socket - valid socket number, limited by MAX_SOCKET_NUMBER.
 * @return - socket protocol defined by PROTOCOLS.
 *
 */
PROTOCOLS gs_get_socket_protocol(TCP_SOCKET socket){
	return gainspan.socket_table[socket].protocol;
}


/*!
 * \brief Get socket port.
 *
 *
 * \details retrieves socket port.
 *
 *
 * @param socket - valid socket number, limited by MAX_SOCKET_NUMBER.
 * @return - socket port.
 *
 */
TCP_PORT gs_get_socket_port(TCP_SOCKET socket){
	return gainspan.socket_table[socket].port;
}


/*!
 * \brief Check socket for data availability i.e. Check if socket has data through TCP connection to a client..
 *
 *
 * \details retrieves data availability status for socket i.e. Check if socket has data through TCP connection to a client.
 *
 *
 * @param socket - valid socket number, limited by MAX_SOCKET_NUMBER.
 * @return - status - SUCCESS OR ERROR, based on status if socket has data or not respectively.
 *
 */
SUCCESS_ERROR gs_get_data_on_socket_status(TCP_SOCKET socket){
	SUCCESS_ERROR check_result = ERROR;
		if((gainspan.socket_with_data == socket)){
			check_result  = SUCCESS;
		}
	return check_result ;
}


/*!
 * \brief Configure and activate a socket for TCP Server to listen.
 *
 *
 * \details Configure a socket for TCP Server to listen. Also activates socket for communication.
 *
 *
 * @param socket - valid socket number, limited by MAX_SOCKET_NUMBER.
 * @param socket_protocol - socket protocol define by PROTOCOLS.
 * @param socket_port- socket port.
 *
 */
void gs_configure_socket(TCP_SOCKET socket, PROTOCOLS socket_protocol, TCP_PORT socket_port){
	gainspan.active_socket = socket;
	gainspan.socket_table[socket].protocol = socket_protocol;
	gainspan.socket_table[socket].port = socket_port;
	gainspan.socket_table[socket].status = SOCKET_STATUS_INIT;
}


/*!
 * \brief Enable TCP Server on a socket to listen.
 *
 *
 * \details Enable TCP Server on a socket to listen. checks if the socket is configured; activates it
 * and enables TCP server to listen.
 *
 *
 * @param socket - valid socket number, limited by MAX_SOCKET_NUMBER.
 * @return - outcome, SUCCESS or ERROR; defined by SUCCESS_ERROR.
 *
 */
SUCCESS_ERROR  gs_enable_activate_socket(TCP_SOCKET socket){
	char gs_command_response[MAX_TX_BUFFER] = "\0";
	uint16_t number_of_characters_read = 0;
	COMMAND_OUTCOME command_result = COMMAND_OUTCOME_ERROR;
	SUCCESS_ERROR process_result = ERROR;

	/*Make the socket active for disconnection/deactivation*/
	if(gs_activate_socket(socket) == SUCCESS){
		/*Start TCP Server - Enable TCP Listen mode on socket*/
		strcpy(gs_command_response, "\0");
		gs_send_command(AT_START_TCP_SERVER);
		number_of_characters_read = gs_get_command_response(gs_command_response, 300);
		command_result = gs_parse_command_response_tcp(gs_command_response, SOCKET_MODE_ENABLE, AT_START_TCP_SERVER);
		#if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
			gs_send_command_response_to_serial_terminal(AT_START_TCP_SERVER, command_result);
		#endif
		if(command_result == COMMAND_OUTCOME_SUCCESS){
			gainspan.device_operation_mode = GAINSPAN_DEVICE_MODE_COMMAND;
			process_result = SUCCESS;
		}else{
			process_result = ERROR;
		}
	}
	return process_result;
}


/*!
 * \brief Reset socket to defaults and put in listen mode.
 *
 *
 * \details Reset socket to defaults and put in listen mode.
 * Any associated connection will be closed.
 *
 *
 *
 *
 * @param socket - valid socket number, limited by MAX_SOCKET_NUMBER.
 * @return - outcome, SUCCESS or ERROR; defined by SUCCESS_ERROR.
 *
 */
SUCCESS_ERROR gs_reset_socket(TCP_SOCKET socket){
	SUCCESS_ERROR process_result = ERROR;
	char command_buffer[MAX_TX_BUFFER];

	if(gs_activate_socket(socket) == SUCCESS){
		/*Close the connection with client on the socket by sending the Escape-C*/

		/*Escape sequence indicating data mode - Escape*/
		sprintf(command_buffer,"\x1b");
		usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);

		/*TCP Data start - S 0x53*/
		sprintf(command_buffer,"\x53");
		usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);

		/*Put client CID based on socket*/
		sprintf(command_buffer,"%x", (uint8_t) gainspan.socket_table[socket].cid);
		usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);

		/*TCP Data end - E - 0x45*/
		sprintf(command_buffer,"\x1b");
		usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);

		//sprintf(command_buffer,"\x45");
		sprintf(command_buffer,"\x43");
		usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);

		/*Reset socket.*/
		strcpy(gainspan.socket_table[socket].ip_address, "0.0.0.0");
		gainspan.socket_table[socket].status = SOCKET_STATUS_LISTEN;
		gainspan.socket_table[socket].protocol = PROTOCOL_TCP;
		gainspan.socket_table[socket].port = INVALID_PORT;
		gainspan.socket_table[socket].cid = gainspan.server_cid;
		gainspan.active_client_cid = gainspan.server_cid;
		//gainspan.active_socket = NO_ACTIVE_SOCKET; 						// No need to modify the active socket
		gainspan.socket_with_data = NO_SOCKET_WTIH_DATA;
		gainspan.device_operation_mode = GAINSPAN_DEVICE_MODE_COMMAND;
		process_result = SUCCESS;

	}
	return process_result;
}


/*!
 * \brief Disconnect and deactivate socket.
 *
 *
 * \details Disconnect and deactivate socket.
 * Takes 1000 ms to complete the process.
 *
 *
 * @param socket - valid socket number, limited by MAX_SOCKET_NUMBER.
 * @return - outcome, SUCCESS or ERROR; defined by SUCCESS_ERROR.
 *
 */
SUCCESS_ERROR gs_disconnect_deactivate_socket(TCP_SOCKET socket){
	TCP_SOCKET socket_counter = 0;
	char gs_command_response[MAX_TX_BUFFER] = "\0";
	uint16_t number_of_characters_read = 0;
	COMMAND_OUTCOME command_result = COMMAND_OUTCOME_ERROR;
	SUCCESS_ERROR process_result = ERROR;

	if(gs_activate_socket(socket) == SUCCESS){
		/*Disconnect/deactivate socket.*/
		strcpy(gs_command_response, "\0");
		gs_send_command(AT_CLOSE_CONNECTION_CID);
		number_of_characters_read = gs_get_command_response(gs_command_response, 1000);
		command_result = gs_parse_command_response_tcp(gs_command_response, SOCKET_MODE_ENABLE, AT_CLOSE_CONNECTION_CID);
		#if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
			gs_send_command_response_to_serial_terminal(AT_CLOSE_CONNECTION_CID, command_result);
		#endif
		if(command_result == COMMAND_OUTCOME_SUCCESS){
			strcpy(gainspan.socket_table[socket].ip_address, "0.0.0.0");
			gainspan.socket_table[socket].status = SOCKET_STATUS_CLOSED;
			gainspan.socket_table[socket].protocol = PROTOCOL_TCP;
			gainspan.socket_table[socket].port = INVALID_PORT;
			gainspan.socket_table[socket].cid = INVALID_CID;
			/*Check if the this socket is active socket, and if yes activate different available socket*/
			if(socket == gainspan.active_socket){
				for(socket_counter = 0; socket_counter < MAX_SOCKET_NUMBER; socket_counter++){
					if(((gainspan.socket_table[socket_counter].status == SOCKET_STATUS_LISTEN) || (gainspan.socket_table[socket_counter].status == SOCKET_STATUS_ESTABLISHED)) && socket_counter != socket){
						gainspan.active_socket = socket_counter;
						gainspan.active_client_cid = gainspan.socket_table[socket_counter].cid;
						break;
					}
				}
			}else{
				gainspan.active_client_cid = INVALID_CID;
			}
			gainspan.socket_with_data = NO_SOCKET_WTIH_DATA;
			gainspan.device_operation_mode = GAINSPAN_DEVICE_MODE_COMMAND;
			process_result = SUCCESS;
		}else{
			process_result = ERROR;
		}
	}
	return process_result;
}


/*!
 * \brief Process TCP response/request.
 *
 *
 * \details Process TCP response/request. Identifies the socket from CID and make it active socket, having data.
 * Polls the serial interface for 300 ms.
 *
 *
 * @param data_string - pointer, data read will be returned.
 * @return - success or failure, return values from SUCCESS_ERROR
 *
 */
SUCCESS_ERROR gs_read_data_from_socket(char *data_string){
	/*Assumption:all the data will be available from GS hardware and no inconsistency is expected*/
	SUCCESS_ERROR process_result = ERROR;
	BOOLEAN_DATA cid_extracted = BOOLEAN_FALSE;
	COMMAND_OUTCOME command_result = COMMAND_OUTCOME_NO_RESPONSE;
	TCP_SOCKET socket = gainspan.active_socket;
	char gs_command_response[MAX_TX_BUFFER] = "\0";
	uint16_t number_of_characters_read = 0;
	uint16_t string_index = 0;
	uint16_t data_string_length = 0;

	strcpy(data_string, "\0");

	strcpy(gs_command_response, "\0");
	//number_of_characters_read = gs_get_command_response(gs_command_response, 50); //50 ms works better
	number_of_characters_read = gs_get_command_response(gs_command_response, 30);

	if ((strlen(gs_command_response)) <= 0 || (number_of_characters_read <=0)){
		strcpy(data_string, "\0");
		data_string = gs_command_response; //send the complete read data back for debugging
		process_result = ERROR;
	}else{
		for(string_index = 0; string_index <= strlen(gs_command_response); string_index++){
			/*TCP Command Socket Process Mode - Client response/request*/
			if ((gs_command_response[string_index] != 0x1b) && (gs_command_response[string_index] != 0x53) && (gs_command_response[string_index] != 0x45) && (gainspan.device_operation_mode == GAINSPAN_DEVICE_MODE_COMMAND)){
				command_result  = gs_parse_command_response_tcp(gs_command_response, SOCKET_MODE_PROCESS, TCP_RESPONSE);
				if (command_result == COMMAND_OUTCOME_SUCCESS){
					process_result = SUCCESS;

				}
			}
			/*Escape sequence indicating data mode - Escape*/
			if ((gs_command_response[string_index] == 0x1b)){
				gainspan.device_operation_mode = GAINSPAN_DEVICE_MODE_DATA;
				gainspan.data_transmission_completed = BOOLEAN_FALSE;
			}
			/*TCP Data start - S 0x53*/
			if ((gs_command_response[string_index] == 0x53) && (gainspan.device_operation_mode == GAINSPAN_DEVICE_MODE_DATA)){
				gainspan.device_operation_mode = GAINSPAN_DEVICE_MODE_DATA_RX;
				gainspan.data_transmission_completed = BOOLEAN_FALSE;
				cid_extracted = BOOLEAN_FALSE;
			}
			/*Get the active socket based on client CID - third byte of response*/
			if ((cid_extracted == BOOLEAN_FALSE) && (gainspan.device_operation_mode == GAINSPAN_DEVICE_MODE_DATA_RX)){
				for(socket = 0; socket < MAX_SOCKET_NUMBER ; socket++){
					if(gainspan.socket_table[socket].cid == hex_to_int(gs_command_response[string_index])){
						gainspan.active_socket = socket;					/*Identify the active socket*/
						gainspan.socket_with_data = socket; 				/*indicates if data is available, and on which socket*/
						gainspan.active_client_cid = gainspan.socket_table[socket].cid;
					}
				}
				gainspan.data_transmission_completed = BOOLEAN_FALSE;
				cid_extracted = BOOLEAN_TRUE;
			}
			/*Receive data*/
			if ((cid_extracted == BOOLEAN_TRUE) && (gainspan.device_operation_mode == GAINSPAN_DEVICE_MODE_DATA_RX)){
				gainspan.data_transmission_completed = BOOLEAN_FALSE;
				data_string[data_string_length] = gs_command_response[string_index];
				data_string_length++;
			}
			/*TCP Data end - E - 0x45*/
			if ((gs_command_response[string_index] == 0x45) && (gainspan.device_operation_mode == GAINSPAN_DEVICE_MODE_DATA)){
				gainspan.data_transmission_completed = BOOLEAN_TRUE;
				gainspan.device_operation_mode = GAINSPAN_DEVICE_MODE_COMMAND;
				gainspan.socket_with_data = NO_SOCKET_WTIH_DATA;
				cid_extracted = BOOLEAN_FALSE;
				process_result = SUCCESS;
			}
		}
	}

	return process_result;
}


/*!
 * \brief Check if TCP response/request registered after process of any socket i.e. client.
 *
 *
 * \details Check if TCP response/request registered after process of any socket i.e. client.
 * Effectively, this function must be called after gs_process_tcp_udp_response();
 *
 *
 * @return - socket number having active connection established with client and available data, else NO_SOCKET_WTIH_DATA.
 *
 */
TCP_SOCKET gs_get_socket_having_active_connection_and_data(void){
	TCP_SOCKET socket = NO_ACTIVE_SOCKET, socket_with_data = NO_SOCKET_WTIH_DATA;
	for(socket = 0 ; socket < MAX_SOCKET_NUMBER; socket++){
		if((gainspan.socket_table[socket].status == SOCKET_STATUS_ESTABLISHED) && (gainspan.socket_table[socket].status == gainspan.socket_with_data)){
			socket_with_data = socket;
			break;
		}
	}
	return socket_with_data ;
}


/*!
 * \brief Check if socket has active TCP connection to a client.
 *
 *
 * \details Check if socket has active TCP connection to a client.
 *
 *
 * @param socket - valid socket number, limited by MAX_SOCKET_NUMBER.
 * @return - status - SUCCESS OR ERROR, based on status if socket has active connection i.e. SOCKET_STATUS_ESTABLISHED.
 *
 */
SUCCESS_ERROR gs_get_socket_connection_status(TCP_SOCKET socket){
	SUCCESS_ERROR check_result = ERROR;
		if((gainspan.socket_table[socket].status == SOCKET_STATUS_ESTABLISHED)){
			check_result  = SUCCESS;
		}
	return check_result ;
}


/*!
 * \brief Write data to socket.
 *
 *
 * \details Write data to socket.
 * Introduces 150 ms delay for complete transfer of data.
 *
 *
 * @param socket - valid socket number, limited by MAX_SOCKET_NUMBER.
 * @param data_string - data to be written. Limited by MAX_TX_BUFFER
 *
 */
void gs_write_data_to_socket(TCP_SOCKET socket, char *data_string){
	char command_buffer[MAX_TX_BUFFER];

	memset(command_buffer, ' ', MAX_TX_BUFFER);

	strcpy(command_buffer, "\0");

	if(strlen(data_string) > 0 && data_string[0] != '\r'){
		if(gainspan.socket_table[socket].protocol == PROTOCOL_TCP){

			/*Escape sequence indicating data mode - Escape*/
			sprintf(command_buffer,"\x1b");
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);

			/*TCP Data start - S 0x53*/
			sprintf(command_buffer,"\x53");
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);

			/*Put client CID based on socket*/
			sprintf(command_buffer,"%x", (uint8_t) gainspan.socket_table[socket].cid);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);

			/*Transmit data*/
			if(strlen(data_string) == 1){
	            if(data_string[0] != '\r' && data_string[0] != '\n'){
					sprintf(command_buffer,"%s\n\r", data_string);
					usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
	            } else if (data_string[0] == '\n') {
					sprintf(command_buffer,"\n\r");
					usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
	            }
			}else{
				sprintf(command_buffer,"%s", data_string);
				usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			}

			/*TCP Data end - E - 0x45*/
			sprintf(command_buffer,"\x1b");
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);

			sprintf(command_buffer,"\x45");
			//sprintf(command_buffer,"\x43");
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
		}
	}
	/*Delay for transmission to complete*/
	 _delay_ms(150);
}


/*!
 * \brief Flush the receiving buffer for Gainspan interface.
 *
 *
 * \details Flush or clears the buffer for incoming data from Gainspan interface.
 *
 *
 */
void gs_flush(void){
	 unsigned char character_from_response = ' ';
	 while (usart_AvailableCharRx(gainspan.usart_id)){
		usart_xgetChar(gainspan.usart_id, &character_from_response);
	 }
}


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
	/*Initialize data structures*/
	/*Client Web Page*/
	client_web_page.page_title = (char *)pvPortMalloc( sizeof(char) * WEB_TITLE_SIZE);
	strcpy(client_web_page.page_title, "");
	client_web_page.menu_title = (char *)pvPortMalloc( sizeof(char) * WEB_TITLE_SIZE);
	strcpy(client_web_page.menu_title, "");
	client_web_page.element_type = HTML_DROPDOWN_LIST ;
	for (int index = 0; index < WEB_PAGE_ELEMENTS; index++){
		client_web_page.web_page_elements[index].element_identifier = ' ';
		client_web_page.web_page_elements[index].element_label = (char *)pvPortMalloc( sizeof(char) * HTML_ELEMENT_LABEL_SIZE);
		strcpy(client_web_page.web_page_elements[index].element_label, " ");
	}
	client_web_page.element_count = 0;
	/*Client response buffer*/
	client_response_buffer = (char *)pvPortMalloc( sizeof(char) * RING_BUFFER_SIZE);
	strcpy(client_response_buffer, "\0");

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
	/*Check if the element already exists*/
	for(loop_counter = 0; loop_counter < client_web_page.element_count ; loop_counter++){
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
		if (client_web_page.element_count <= WEB_PAGE_ELEMENTS){
			client_web_page.web_page_elements[client_web_page.element_count].element_identifier = choice_identifier;
			if (strlen(element_label) < HTML_ELEMENT_LABEL_SIZE && strlen(element_label) > 0){
				strncpy(client_web_page.web_page_elements[client_web_page.element_count].element_label , element_label, strlen(element_label));
			}else if(strlen(element_label) <= 0){
				strcpy(client_web_page.web_page_elements[client_web_page.element_count].element_label, "Client choice");
			}else{
				strncpy(client_web_page.web_page_elements[client_web_page.element_count].element_label , element_label, HTML_ELEMENT_LABEL_SIZE);
			}
			client_web_page.element_count++;
			if (client_web_page.element_count > WEB_PAGE_ELEMENTS){
				client_web_page.element_count = WEB_PAGE_ELEMENTS;
			}
			#if SET_WEB_SERVER_TERMINAL_OUTPUT_ON == 1
				/*Send message to serial terminal*/
				usart_xfprint(SERIAL_TERNMINAL, (uint8_t *) "\n\rWeb Page: element added....\n\r");
			#endif
		}else{
			#if SET_WEB_SERVER_TERMINAL_OUTPUT_ON == 1
				/*Send message to serial terminal*/
				usart_xfprint(SERIAL_TERNMINAL, (uint8_t *) "\n\rWeb Page: can't add element, max 10 allowed....\n\r");
				_delay_ms(5000);
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

	if (client_web_page.element_count > 0){
		/*Initialize the server*/
		initialize_web_server(port, protocol);
		/*Search for available socket, activate and start to listen incoming connection*/
		for (TCP_SOCKET socket  = 0; socket < MAX_SOCKET_NUMBER; socket++){
			if (gs_get_socket_status(socket) == SOCKET_STATUS_CLOSED){
				if (wifi_server.server_protocol == PROTOCOL_TCP){
					gs_configure_socket(socket, wifi_server.server_protocol, wifi_server.server_port);
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

//usart_printf_P(PSTR("\r\n1 Choices-> %u"), client_web_page.element_count);



/*!\brief Process client request.
 *
 * \details accepts incoming connection on socket, sends the web-page and
 * reads the client response
 * \warning Ensure web-page is configured and web server is started before calling this routine/function.
 *
 *
 */
void process_client_request(void){

	char data_string[128] = "\0";
	char html_string[128] = "\0";
	char find_GET_in_response[128] = "\0";
	char client_response = ' ';
	uint8_t loop_counter = 0;

	if (web_server_status == WEB_SERVER_ACTIVE){
		if (gs_get_socket_status(wifi_client.client_socket) == SOCKET_STATUS_LISTEN){
			gs_read_data_from_socket(data_string); //accept connection, get CID
			/*Extract client request and store in ring buffer*/
			if(strlen(data_string) > 0){
				strcpy(find_GET_in_response, strstr(data_string, "GET"));
				if (*(find_GET_in_response + 5) == '?'){
					client_response = *(find_GET_in_response + 8);

					/*Add to circular buffer for processing*/
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
				strcpy(html_string, "<title>");
				strcat(html_string, client_web_page.page_title);
				strcat(html_string, "</title> \n");
				gs_write_data_to_socket(wifi_client.client_socket, html_string);
				gs_write_data_to_socket(wifi_client.client_socket, "</head> \n");
				gs_write_data_to_socket(wifi_client.client_socket, "<body> \n");
				/*Page title*/
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
				gs_write_data_to_socket(wifi_client.client_socket, " ");
//				gs_write_data_to_socket(wifi_client.client_socket, "HTTP/1.1 205 Reset Content\n");

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
	/*initialize with blank*/
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


/*!
 * \brief Initialize Gainspan module with default configuration parameters.
 *
 *
 * \details Initializes module by initializing configuration/control parameters to default values.
 * \note Ensure to define an unique SSID, to avoid conflict with neighbouring networks.
 * Default values are:
 * 	- Serial terminal USART = USART_0
 * 	- Serial terminal USART baud rate = BAUD_RATE_9600
 * 	- Gainspan USART = USART_2
 * 	- Gainspan USART baud rate = BAUD_RATE_9600
 * 	- Gainspan device connection status = GAINSPAN_ACTIVE_FALSE
 * 	- Wireless SSID = "GAINSPAN"
 * 	- Wireless security security_key = "napsniag"
 * 	- Wireless mode = WIRELESS_MODE_LIMITEDAP
 * 	- Wireless authentication mode = AUTHENTICATION_MODE_NONE
 * 	- Wireless security configuration = WIRELESS_SECURITY_CONFIGURATION_WPA_PSK_SECURITY
 * 	- Transmission rate = TRANSMISSION_RATE_AUTO
 * 	- Wireless channel = WIRELESS_CHANNEL_11
 * 	- Device local IP address = "192.168.1.1"
 * 	- Device subnet = "255.255.255.0"
 * 	- Device gateway = "192.168.1.1"
 * 	- Device TCP/UDP Protocol gainspan.server_protocol = PROTOCOL_TCP;
 * 	- Device TCP/UCP Port gainspan.server_port = 80;
 * 	- Device TCP/UDP connection gainspan.server_number_of_connection = 1;
 * 	- Web server administrator ID = "admin"
 * 	- Web server administrator password = "nimda"
 * 	- Device operation mode = GAINSPAN_DEVICE_MODE_COMMAND
 * 	- Device TCP/UDP Server CID gainspan.server_cid = 0;
 * 	- Device client socket table
 * 		- gainspan.socket_table[counter].ip_address = "0.0.0.0";
 * 		- gainspan.socket_table[counter].status = SOCKET_STATUS_CLOSED;
 * 		- gainspan.socket_table[counter].protocol = 0; //check for default value
 * 		- gainspan.socket_table[counter].port = 0; //check for default value
 * 		- gainspan.socket_table[counter].cid = 0; //check for default value
 * 	- Device Socket with data available gainspan.socket_with_data = NO_SOCKET_WTIH_DATA;
 * 	- Device Active Socket for communication gainspan.active_socket = 0;
 * 	- Device Active Client CID gainspan.active_client_cid = 0;
 *	- Device Operation Mode gainspan.device_operation_mode gainspan.device_operation_mode = GAINSPAN_DEVICE_MODE_COMMAND;
 *	- Data Transmission Completion Status gainspan.data_transmission_completed = BOOLEAN_TRUE;
 *
 */
void gs_initialize_gainspan(void){
	TCP_SOCKET socket = 0;
	gainspan.serial_terminal_usart_id = USART_0;
	gainspan.serial_terminal_baud_rate = BAUD_RATE_9600;
	gainspan.usart_id = USART_2;
	gainspan.baud_rate = BAUD_RATE_9600;
	gainspan.device_connection_status = GAINSPAN_ACTIVE_FALSE;
	gainspan.ssid = (char *)pvPortMalloc( sizeof(char) * GENERAL_SIZE);
	strcpy(gainspan.ssid, "GAINSPAN");
	gainspan.security_key = (char *)pvPortMalloc( sizeof(char) * GENERAL_SIZE);
	strcpy(gainspan.security_key, "napsniag");
	gainspan.wireless_mode = WIRELESS_MODE_LIMITEDAP;
	gainspan.authentication_mode = AUTHENTICATION_MODE_NONE;
	gainspan.wireless_security_configuration = 	WIRELESS_SECURITY_CONFIGURATION_WPA_PSK_SECURITY;
	gainspan.transmission_rate = TRANSMISSION_RATE_AUTO;
	gainspan.wireless_channel = WIRELESS_CHANNEL_11;
	gainspan.local_ip_address = (char *)pvPortMalloc( sizeof(char) * IP_SIZE);
	strcpy(gainspan.local_ip_address, "192.168.3.1");
	gainspan.subnet = (char *)pvPortMalloc( sizeof(char) * IP_SIZE);
	strcpy(gainspan.subnet, "255.255.255.0");
	gainspan.gateway = (char *)pvPortMalloc( sizeof(char) * IP_SIZE);
	strcpy(gainspan.gateway, "192.168.3.1");
	gainspan.server_protocol = PROTOCOL_TCP;
	gainspan.server_port = 80;
	gainspan.server_number_of_connection = 1;
	gainspan.web_server_administrator_id = "admin";
	gainspan.web_server_administrator_password = "nimda";
	gainspan.device_operation_mode = GAINSPAN_DEVICE_MODE_COMMAND;
	gainspan.server_cid = INVALID_CID;
	for (socket = 0; socket < MAX_SOCKET_NUMBER; socket++){
		gainspan.socket_table[socket].ip_address = (char *)pvPortMalloc( sizeof(char) * IP_SIZE);
		strcpy(gainspan.socket_table[socket].ip_address, "0.0.0.0");
		gainspan.socket_table[socket].status = SOCKET_STATUS_CLOSED;
		gainspan.socket_table[socket].protocol = PROTOCOL_TCP;
		gainspan.socket_table[socket].port = INVALID_PORT;
		gainspan.socket_table[socket].cid = INVALID_CID;
	}
	gainspan.socket_with_data = NO_SOCKET_WTIH_DATA;
	gainspan.active_socket = NO_ACTIVE_SOCKET;
	gainspan.active_client_cid = INVALID_CID;
	gainspan.device_operation_mode = GAINSPAN_DEVICE_MODE_COMMAND;
	gainspan.data_transmission_completed = BOOLEAN_TRUE;
}


/*!
 * \brief Send/submit command to Gainspan WiFi module.
 *
 *
 * \details Sends/submits valid command to Gainspan WiFi module.
 *
 *
 * @param at_command - valid command, refer the list of valid commands.
 *
 */
void gs_send_command(AT_COMMAND at_command){
	char command_buffer[50];
	memset(command_buffer, ' ', 50);

	/*Flush to transmission buffer*/
	gs_flush();

	switch(at_command){
		case AT_OK:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_DISABLE_ECHO:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_VERBOSE_ENABLE:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_SET_USART:
			sprintf(command_buffer,"%s%lu,8,n,1\n\r", gs_at_commands[at_command], (uint32_t) gainspan.baud_rate);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_GET_DEVICE_OEM_ID:
			break;
		case AT_GET_DEVICE_HARDWARE_VERSION:
			break;
		case AT_GET_DEVICE_SOFTWARE_VERSION:
			break;
		case AT_GET_DEVICE_MAC_ADDRESS:
			break;
		case AT_SCAN_NETWORK_FOR_SSID:
			break;
		case AT_SET_WIRELESS_MODE:
			sprintf(command_buffer,"%s%u\n\r", gs_at_commands[at_command], (uint8_t) gainspan.wireless_mode);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_ASSOCIATE_START_NETWORK:
			sprintf(command_buffer,"%s%s,,%u\n\r", gs_at_commands[at_command], gainspan.ssid, (uint8_t) gainspan.wireless_channel);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_DISASSOCIATE_CURRENT_NETWORK:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_GET_CURRENT_NETWORK_STATUS:
			break;
		case AT_GET_CURRENT_WIRELESS_NETWORK_STATUS:
			break;
		case AT_GET_WIRELESS_RSSI:
			break;
		case AT_SET_TRANSMISSION_RATE:
			sprintf(command_buffer,"%s%u\n\r", gs_at_commands[at_command], (uint8_t) gainspan.transmission_rate);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_GET_TRANSMISSION_RATE:
			break;
		case AT_SET_AUTHENTICATION_MODE:
			sprintf(command_buffer,"%s%u\n\r", gs_at_commands[at_command], (uint8_t) gainspan.authentication_mode);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_SET_WIRELESS_SECURITY_CONFIGURATION:
			sprintf(command_buffer,"%s%u\n\r", gs_at_commands[at_command], (uint8_t) gainspan.wireless_security_configuration);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_SET_WPA_PASSPHRASE:
			sprintf(command_buffer,"%s%s\n\r", gs_at_commands[at_command], gainspan.security_key);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_SET_WPA2PSK:
			sprintf(command_buffer,"%s%s,%s\n\r", gs_at_commands[at_command], gainspan.ssid, gainspan.security_key);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_DISABLE_RADIO:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_ENABLE_RADIO:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_DISABLE_RADIO_POWER_SAVER_MODE:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_ENABLE_RADIO_POWER_SAVER_MODE:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_DISABLE_DHCP_IPV4:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_ENABLE_DHCP_IPV4:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_SET_STATIC_NETWORK_PARAMTERS_IPV4:
			sprintf(command_buffer,"%s%s,%s,%s\n\r", gs_at_commands[at_command], gainspan.local_ip_address, gainspan.subnet, gainspan.gateway);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_STOP_DHCP_SERVER_IPV4:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_START_DHCP_SERVER_IPV4:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_STOP_DNS_SERVER:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_START_DNS_SERVER:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_DNS_LOOKUP:
			break;
		case AT_STOP_WEBSERVER:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_START_WEBSERVER:
			sprintf(command_buffer,"%s%s,%s\n\r", gs_at_commands[at_command], gainspan.web_server_administrator_id, gainspan.web_server_administrator_password);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_DISABLE_XML_PARSE:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_ENABLE_XML_PARSE:
			sprintf(command_buffer,"%s\n\r", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_START_TCP_SERVER:
			sprintf(command_buffer,"%s%u\n\r", gs_at_commands[at_command], (uint8_t) gainspan.server_port);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_START_TCP_CLIENT:
			break;
		case AT_START_UDP_SERVER:
			break;
		case AT_START_UDP_CLIENT:
			break;
		case AT_CLOSE_CONNECTION_CID:
			if(gainspan.socket_table[gainspan.active_socket].status != SOCKET_STATUS_CLOSED){
				sprintf(command_buffer,"%s%x\n\r", gs_at_commands[at_command], gainspan.active_client_cid);
				usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			}
			break;
/*
		case AT_START_WEB_PROVISIONING:
			sprintf(command_buffer,"%s%s,%s\n", gs_at_commands[at_command], gainspan.web_provision_administrator_id, gainspan.web_provision_administrator_password);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
		case AT_STOP_WEB_PROVISIONING:
			sprintf(command_buffer,"%s\n", gs_at_commands[at_command]);
			usart_xfprint(gainspan.usart_id, (uint8_t *) command_buffer);
			break;
*/
		default:
			break;
	}
	#if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
		/*Send the actual command to serial terminal for debugging*/
		usart_xfprint(gainspan.serial_terminal_usart_id, (uint8_t *) "\n\r");
		usart_xfprint(gainspan.serial_terminal_usart_id, (uint8_t *) command_buffer);
		//usart_xfprint(gainspan.serial_terminal_usart_id, (uint8_t *) "\n\r");
	#endif
}


/*!
 * \brief Collect the command response from Gainspan WiFi module.
 *
 *
 * \details Collect the response from Gainspan WiFi module for the last submitted command. Attempt will
 * be made to collect the response for the polling period. Polling interval is defined by COMMAND_RESPONSE_POLLING_INTERVAL_IN_MILLISECONDS.
 *
 * \note: returns maximum 128 characters, and rest of the response is discarded.
 *
 * @param gs_command_response - Pointer to string buffer to return the response.
 * @param polling_period_in_milliseconds - Polling period.
 * @return - returns the number of characters read.
 *
 */
uint16_t gs_get_command_response(char *gs_command_response, uint16_t polling_period_in_milliseconds){
	unsigned char character_from_response = ' ';
	uint16_t string_index = 0;
	uint16_t number_of_characters_read = 0;
	uint16_t maximum_polling_cycles = polling_period_in_milliseconds / COMMAND_RESPONSE_POLLING_INTERVAL_IN_MILLISECONDS, polling_cycle_counter = 0;

	for(polling_cycle_counter = 0; polling_cycle_counter <= maximum_polling_cycles; polling_cycle_counter++){
		_delay_ms(COMMAND_RESPONSE_POLLING_INTERVAL_IN_MILLISECONDS);
		while (usart_AvailableCharRx(gainspan.usart_id)){
			usart_xgetChar(gainspan.usart_id, &character_from_response);
			gs_command_response[string_index] = character_from_response;
			string_index++;
			number_of_characters_read++;
			/*Max limit on characters to be captured from response is 128*/
			if (number_of_characters_read >= CHARACTERS_TO_READ_FROM_GAINSPAN_RESPONSE){
				/*Discard the rest of characters from USART buffer*/
				gs_flush();
				break;
			}
		 }
	 }

	 gs_command_response[string_index] = '\0';  //terminate string

	 #if SET_GAINSPAN_TERMINAL_OUTPUT_ON == 1
	 	 /*Send the actual command to serial terminal for debugging*/
 	 	 usart_xfprint(gainspan.serial_terminal_usart_id, (uint8_t *) "\n\rResponse:");
	 	 usart_xfprint(gainspan.serial_terminal_usart_id, (uint8_t *) gs_command_response);
 	 	 usart_xfprint(gainspan.serial_terminal_usart_id, (uint8_t *) "\n\r");
	 #endif

	 return number_of_characters_read;
}


/*!
 * \brief Parse the command response from Gainspan WiFi module.
 *
 *
 * \details Parse the response from Gainspan WiFi module for the last submitted command for outcome.
 * Response are defined in COMMAND_OUTCOME. For more details read the data-sheet or related documentation
 * for Gainspan GS1011M.
 *
 *
 *
 * @param gs_command_response - Pointer to string buffer having the response from module.
 * @return - command outcome, valid values are defined by COMMAND_OUTCOME.
 *
 */
COMMAND_OUTCOME gs_parse_command_response(char *gs_command_response){
	COMMAND_OUTCOME command_result = COMMAND_OUTCOME_NO_RESPONSE;
	char string_buffer[80];
	uint16_t string_buffer_index = 0;
	uint16_t string_index = 0;

	memset(string_buffer, ' ', 80);

	if (strlen(gs_command_response) <= 0 ){
		command_result = COMMAND_OUTCOME_NO_RESPONSE;
	}else{
		for(string_index = 0; string_index <= strlen(gs_command_response); string_index++){
			if ((gs_command_response[string_index] == '\r') || (gs_command_response[string_index] == '\n')){				//end of line
				if (string_buffer_index > 0){ //valid string
					//compare for OK or ERROR
					string_buffer[string_buffer_index] = '\0';
					if (strncmp(string_buffer, "OK", 2) == 0){ //OK
						command_result = COMMAND_OUTCOME_SUCCESS;
					} else if (strncmp(string_buffer, "ERROR", 5) == 0){ //ERROR
						command_result = COMMAND_OUTCOME_ERROR;
					}
					string_buffer_index = 0;
					memset(string_buffer, ' ', 80); //flush
					string_buffer[0] = '\0'; //terminate string
				}
			}else{
				string_buffer[string_buffer_index] = gs_command_response[string_index];
				string_buffer_index++;
			}
		}
	}
	return command_result;
}



/*!
 * \brief Parse the command response from Gainspan WiFi module for TCP connections
 *
 *
 * \details Parse the response from Gainspan WiFi module for the last submitted command for outcome.
 * Response are defined in COMMAND_OUTCOME. For more details read the data-sheet or related documentation
 * for Gainspan GS1011M.
 * This function modifies the other function gs_parse_command_response , as it handles the response for
 * TCP connections.
 *
 *
 *
 * @param gs_command_response - Pointer to string buffer having the response from module.
 * @param socket_mode - specify the socket is under process mode or enable/activation; defined by SOCKET_MODE.
 * @param at_command  - valid command, refer the list of valid commands.
 * @return - command outcome, valid values are defined by COMMAND_OUTCOME.
 *
 */
COMMAND_OUTCOME gs_parse_command_response_tcp(char *gs_command_response, SOCKET_MODE socket_mode, AT_COMMAND at_command){
	COMMAND_OUTCOME command_result = COMMAND_OUTCOME_NO_RESPONSE;
	char string_buffer[MAX_TX_BUFFER];
	uint16_t string_buffer_index = 0;
	uint16_t string_index = 0;
	TCP_SOCKET socket = gainspan.active_socket;
	SUCCESS_ERROR process_result = ERROR;

	memset(string_buffer, ' ', MAX_TX_BUFFER);

	if (strlen(gs_command_response) <= 0 ){
		command_result = COMMAND_OUTCOME_NO_RESPONSE;
	}else{
		for(string_index = 0; string_index <= strlen(gs_command_response); string_index++){
			if ((gs_command_response[string_index] == '\r') || (gs_command_response[string_index] == '\n')){				//end of line
				if (string_buffer_index > 0){ //valid string
					//compare for CONNECT, OK or ERROR
					string_buffer[string_buffer_index] = '\0';
					if (strncmp(string_buffer, "CONNECT", 7) == 0){ //CONNECT
						if(socket_mode == SOCKET_MODE_ENABLE){
							/*Socket Activate/Enable mode*/
							gainspan.server_cid = hex_to_int(string_buffer[8]);
							gainspan.active_client_cid = hex_to_int(string_buffer[8]);
							gainspan.socket_table[gainspan.active_socket].cid = hex_to_int(string_buffer[8]);
							gainspan.socket_table[gainspan.active_socket].status = SOCKET_STATUS_LISTEN;
						}else if(socket_mode == SOCKET_MODE_PROCESS){
							/*Socket Process mode*/
							for(socket = 0; socket  < MAX_SOCKET_NUMBER; socket++){
								if((gainspan.socket_table[socket].status == SOCKET_STATUS_LISTEN) && (gainspan.socket_table[socket].cid == hex_to_int(string_buffer[8]))){
									if((gainspan.socket_table[socket].protocol == PROTOCOL_TCP) && (gainspan.server_cid == hex_to_int(string_buffer[8]))){
										gainspan.active_socket = socket;
										gainspan.active_client_cid = hex_to_int(string_buffer[10]);
										gainspan.socket_table[socket].cid = hex_to_int(string_buffer[10]);
										gainspan.socket_table[socket].status = SOCKET_STATUS_ESTABLISHED;
									}
								}
							}
						}
						command_result = COMMAND_OUTCOME_SUCCESS;
						break;
					}else if (strncmp(string_buffer, "DISCONNECT", 10) == 0){ //DISCONNECT
						for(socket = 0; socket  < MAX_SOCKET_NUMBER; socket++){
							if(((gainspan.socket_table[socket].status == SOCKET_STATUS_ESTABLISHED) || (gainspan.socket_table[socket].status == SOCKET_STATUS_LISTEN)) && (gainspan.active_client_cid == hex_to_int(string_buffer[11]))){
								process_result = gs_reset_socket(socket);
								gainspan.device_operation_mode = GAINSPAN_DEVICE_MODE_COMMAND;
							}
						}
						command_result = COMMAND_OUTCOME_SUCCESS;
						break;
					}else if (strncmp(string_buffer, "Disassociation Event", 20) == 0){ //DISCONNECT
						gainspan.device_connection_status = GAINSPAN_ACTIVE_TRUE_WITH_ERRORS;
						command_result = COMMAND_OUTCOME_SUCCESS;
						break;
					}else if (strncmp(string_buffer, "OK", 2) == 0){ //OK
						command_result = COMMAND_OUTCOME_SUCCESS;
						gainspan.device_operation_mode = GAINSPAN_DEVICE_MODE_COMMAND;
						break;
					} else if (strncmp(string_buffer, "ERROR", 5) == 0){ //ERROR
						/*Put active socket to listen mode*/
						gainspan.socket_table[socket].status = SOCKET_STATUS_LISTEN;
						gainspan.device_operation_mode = GAINSPAN_DEVICE_MODE_COMMAND;
						command_result = COMMAND_OUTCOME_ERROR;
						break;
					} else if (strncmp(string_buffer, "INVALID CID", 11) == 0){ //Invalid CID
						command_result = COMMAND_OUTCOME_ERROR;
						break;
					}
					string_buffer_index = 0;
					memset(string_buffer, ' ', MAX_TX_BUFFER);  //flush
					string_buffer[0] = '\0'; 					//terminate string
				}
			}else{
				string_buffer[string_buffer_index] = gs_command_response[string_index];
				string_buffer_index++;
			}
		}
	}
	return command_result ;
}


/*!
 * \brief Send the message to serial terminal on outcome of the command response from Gainspan WiFi module.
 *
 *
 * \details Sends appropriate message to serial terminal.
 *
 *
 *
 * @param at_command - valid command issued, refer the list of valid commands.
 * @param command_result - outcome of the command, refer the list of valid outcomes from COMMAND_OUTCOME.
 *
 */
void gs_send_command_response_to_serial_terminal(AT_COMMAND at_command, COMMAND_OUTCOME command_result){
	char string_buffer[80] = "", command_response_result[50] = "\0";

	switch (command_result){
	case COMMAND_OUTCOME_ERROR:
		strcpy(command_response_result, "ERROR!");
		break;
	case COMMAND_OUTCOME_SUCCESS:
		strcpy(command_response_result, "SUCCESS!");
		break;
	case COMMAND_OUTCOME_NO_RESPONSE:
		strcpy(command_response_result, "NO RESPONSE CAPTURED!");
		break;
	default:
		strcpy(command_response_result, "NO RESPONSE CAPTURED!");
		break;
	}
	sprintf(string_buffer,"Command-%s: %s\n\r", gs_at_commands[at_command], (char *) command_response_result);
	usart_xfprint(gainspan.serial_terminal_usart_id, (uint8_t *) string_buffer);
}


/*!
 * \brief Send the activation status of Gainspan WiFi module to serial terminal.
 *
 *
 * \details Sends appropriate message to serial terminal.
 *
 *
 * @param gs_active - device activation status, valid values are defined by GAINSPAN_ACTIVE.
 *
 */
void gs_send_activation_status_to_serial_terminal(GAINSPAN_ACTIVE gs_active){
	char string_buffer[80] = "", gs_activate_status[50] = "\0";

	switch (gs_active){
	case GAINSPAN_ACTIVE_FALSE:
		strcpy(gs_activate_status, "activation failure!");
		break;
	case GAINSPAN_ACTIVE_TRUE:
		strcpy(gs_activate_status, "activated successfully!");
		break;
	case GAINSPAN_ACTIVE_TRUE_WITH_ERRORS:
		strcpy(gs_activate_status, "activated with errors!");
		break;
	default:
		strcpy(gs_activate_status, "activation failure!");
		break;
	}

	sprintf(string_buffer,"\n\rGainspan Device: %s\n\r", (char *) gs_activate_status);
	usart_xfprint(gainspan.serial_terminal_usart_id, (uint8_t *) string_buffer);
}



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


/*!
 * \brief Convert Hexadecimal to Integer.
 *
 *
 * \details Convert Hexadecimal to Integer.
 *
 *
 * @param character - byte to convert
 * @return - integer equivalent of hexadecimal number.
 *
 */
uint8_t hex_to_int(char character){
	uint8_t value = 0;

	if (character >= '0' && character <= '9') {
		value = character - '0';
	}
	else if (character >= 'A' && character <= 'F') {
		value = character - 'A' + 10;
	}
	else if (character >= 'a' && character <= 'f') {
		value = character - 'a' + 10;
	}

	return value;
}


/*!
 * \brief Convert Integer to Hexadecimal.
 *
 *
 * \details Convert Integer to Hexadecimal .
 *
 *
 * @param character - byte to convert
 * @return - hexadecimal equivalent of integer number.
 *
 */
char int_to_hex(uint8_t character){
	char value = '0';

	if (character >= 0 && character <= 9) {
		value = character + '0';
	}
	else if (character >= 10 && character <= 15) {
		value = character + 'A' - 10;
	}

	return value;
}

/*---------------------------------------  ISR-Interrupt Service Routines  ---------------------------------------*/
/*define your Interrupt Service Routines here*/

/*(Doxygen help: use \brief to provide short summary, \details for detailed description and \param for parameters */

/*NO ISR's */

/*!@}*/   // end module

