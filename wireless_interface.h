/*
 * wireless_interface.h
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
/* CODING STANDARD
 * Header file: Section I. Prologue: description about the file, description author(s), revision control
 * 				information, references, etc.
 * Note: 1. Header files should be functionally organized.
 *		 2. Declarations   for   separate   subsystems   should   be   in   separate
 */

/*(Doxygen help: use \brief to provide short summary and \details command can be used)*/

/*!	\file wireless_interface.h
 * 	\brief This file defines and implements the functions responsible for interface with Gainspan GS1011M WiFi
 * 	module and Web server to interact with clients connecting over WiFi.
 *
 *
 * \details
 * Gainsoan GS1011M WiFi: Defines the functions responsible for interface with WiFi shields based on Gainspan
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
 *
 */


#ifndef WIRELESS_INTERFACE_H_
#define WIRELESS_INTERFACE_H_

/******************************************************************************************************************/
/* CODING STANDARDS:
 * Header file: Section II. Include(s): header file includes. System include files and then user include files.
 * 				Ensure to add comments for an inclusion which is not very obvious. Suggested order of inclusion is
 * 								System -> Other Modules -> Same Module -> Specific to this file
 * Note: Avoid nested inclusions.
 */

#include <avr/pgmspace.h>

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

/*Send commands and respective command response to terminal*/
#define SET_GAINSPAN_TERMINAL_OUTPUT_ON					1				/*!Default - 0; set to 1 to send the commands and respective response from Gainspan device to serial terminal define in Gainspan data structure gainspan.serial_terminal_usart_id*/
#define SET_WEB_SERVER_TERMINAL_OUTPUT_ON				1				/*!Default - 0; set to 1 to send the commands and respective response from Gainspan device to serial terminal define in Gainspan data structure gainspan.serial_terminal_usart_id*/

/*Serial2WiFi: AT commands*/

/*Serial-to-WiFi profile configuration*/
#define AT_OK			 								0				/*!<Check for communication, returns OK on success*/
#define AT_DISABLE_ECHO 								1				/*!<Disable ECHO i.e. input commands will not be send back.*/
#define AT_VERBOSE_ENABLE								2				/*!<Enable verbose response to get status response in the form of ASCII strings.*/
#define AT_SET_USART									3				/*!<Set the UART parameters: baudrate,bitsperchar,parity,stopbits. Example-115200,8,n,1.*/
#define AT_GET_DEVICE_OEM_ID							4				/*!<Get OEM identification. *Not implemented*/
#define AT_GET_DEVICE_HARDWARE_VERSION					5				/*!<Get hardware version.  *Not implemented*/
#define AT_GET_DEVICE_SOFTWARE_VERSION					6				/*!<Get software version.  *Not implemented*/
/*WiFi interface configuration*/
#define AT_GET_DEVICE_MAC_ADDRESS						7				/*!<Get MAC address of device. *Not implemented*/
#define AT_SCAN_NETWORK_FOR_SSID						8				/*!<Scan WiFi networks for SSID. *Not implemented*/
#define AT_SET_WIRELESS_MODE							9				/*!<Set wireless mode: 0-Infrastructure, 1-Ad Hoc, 2-limited AP.*/
#define AT_ASSOCIATE_START_NETWORK						10				/*!<Associate with a Network, or Start an Ad Hoc or Infrastructure (AP) Network. Parameters-SSID,BSSID,Ch,Rssi Flag.*/
#define AT_DISASSOCIATE_CURRENT_NETWORK					11				/*!<Disassociate from current network.*/
#define AT_GET_CURRENT_NETWORK_STATUS					12				/*!<Get current network status; returns-MAC, WLAN, Mode, BSSID, SSID, Channel, Security, RSSI, Network configuration, Rx count, Tx count. *Not implemented*/
#define AT_GET_CURRENT_WIRELESS_NETWORK_STATUS			13				/*!<Get wireless network status; returns-Mode, BSSID, SSID, Channel, Security. *Not implemented*/
#define AT_GET_WIRELESS_RSSI							14				/*!<Get wireless RSSI in dBm. *Not implemented */
#define AT_SET_TRANSMISSION_RATE						15				/*!<Set transmission rate: 0-Auto, 2-1 Mbps, 4-2 Mbps, 1-5.5 Mbps, 22-11 Mbps*/
#define AT_GET_TRANSMISSION_RATE						16				/*!<Get transmission rate; returns: 0-Auto, 2-1 Mbps, 4-2 Mbps, 1-5.5 Mbps, 22-11 Mbps. *Not implemented*/
/*WiFi Security Configuration	*/
#define AT_SET_AUTHENTICATION_MODE						17				/*!<Set authentication mode: 0-None, 1-WEP Open, 2-WEP Shared.*/
#define AT_SET_WIRELESS_SECURITY_CONFIGURATION			18				/*!<Set wireless security configuration: 0-Auto security (All), 1-Open security, 2-WEP security, 4-Wpa-psk security, 8-WPA2-PSK security, 16-WPA Enterprise, 32-WPA2 Enterprise.*/
#define AT_SET_WPA_PASSPHRASE							19				/*!<Set WPA passphrase: string of 8-63 characters.*/
#define AT_SET_WPA2PSK									20				/*!<Compute and store WPA2 PSK value from SSID and Passkey.*/
#define AT_DISABLE_RADIO								21				/*!<Disable (0) 802.11 radio receiver.*/
#define AT_ENABLE_RADIO									22				/*!<Enable (1) 802.11 radio receiver.*/
#define AT_DISABLE_RADIO_POWER_SAVER_MODE				23				/*!<Disable (0) 802.11 Power Saver Mode.*/
#define AT_ENABLE_RADIO_POWER_SAVER_MODE				24				/*!<Enable (1) 802.11 Power Saver Mode, by informing AP, AP shall buffer all the incoming unicast traffic during this time.*/
/*Network interface*/
#define AT_DISABLE_DHCP_IPV4							25				/*!<Disable (0) DHCP.*/
#define AT_ENABLE_DHCP_IPV4								26				/*!<Enable (1) DHCP.*/
#define AT_SET_STATIC_NETWORK_PARAMTERS_IPV4			27				/*!<Set static network parameters; parameters: Src Address,Net-mask,Gateway.*/
#define AT_STOP_DHCP_SERVER_IPV4						28				/*!<Stop (0) DHCP Server.*/
#define AT_START_DHCP_SERVER_IPV4						29				/*!<Start (1) DHCP Server.*/
#define AT_START_DNS_SERVER								30				/*!<Stop (0) DNS Server.*/
#define AT_STOP_DNS_SERVER								31				/*!<Start (1) DNS Server; parameters: Start/stop,url.*/
#define AT_DNS_LOOKUP									32				/*!<DNS lookup; parameters: URL,RETRY,TIMEOUT-S,CLEAR CACHE ENTRY. *Not implemented*/
/*GSLink*/
#define AT_STOP_WEBSERVER								33				/*!<Stop (n=0) web server.*/
#define AT_START_WEBSERVER								34				/*!<Start (n=1) web server: n,user name,password,1=SSL enable/0=SSL disable,idle timeout,Response timeout.*/
#define AT_DISABLE_XML_PARSE							35				/*!<Disable (0) XML parser on HTTP data.*/
#define AT_ENABLE_XML_PARSE								36				/*!<Enable (1) XML parser on HTTP data.*/
/*Connection management configuration*/
#define AT_START_TCP_SERVER								37				/*!<Start the TCP server connection with IPv4 address; parameters: Port,max client connection (1-15).*/
#define AT_START_TCP_CLIENT								38				/*!<Create a TCP client connection to the remote server with IPv4; parameters: Dest-Address,Port. *Not implemented*/
#define AT_START_UDP_SERVER								39				/*!<Start the UDP server connection with IPv4 address; parameters: Port. *Not implemented*/
#define AT_START_UDP_CLIENT								40				/*!<Create a UDP client connection to the remote server with IPv4; parameters: Dest-Address,Port,Src.Port. *Not implemented*/
#define AT_CLOSE_CONNECTION_CID							41				/*!<Close the connection associated with current active socket by identifying CID:CID.*/
/*Provisioning*/
#define AT_START_WEB_PROVISIONING						44				/*!<Start support provisioning through web pages:user name , password ,[SSL Enabled,Param StoreOption,idletimeout,ncmautoconnect].  *Not implemented*/
#define AT_STOP_WEB_PROVISIONING						45				/*!<Stop support provisioning through web pages.  *Not implemented*/
/*General identifiers*/
#define TCP_RESPONSE									42				/*!<This is not a command, it is used to identify and send message to serial/terminal*/
#define AT_COMMAND_INVALID								43				/*!<This is not a command, Identifier for Invalid command.*/

/*Client ID*/
#define INVALID_CID										255				/*!<Invalid CID.*/

/*Socket*/
#define MAX_SOCKET_NUMBER 								4				/*!<Maximum number of sockets*/
#define NO_SOCKET_WTIH_DATA								255				/*!<Socket value 255, indicates no data on any socket.*/
#define NO_ACTIVE_SOCKET								255				/*!<Indicates no active socket.*/

#define INVALID_PORT									0				/*!<Invalid or no port.*/

/*Maximum buffer length in bytes (characters) for data transmission*/
#define MAX_TX_BUFFER									128				/*!<Maximum transmission buffer*/

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


/*!
 * \brief Type COMMAND
 *
 *
 * \details Type COMMAND
 *
 */
typedef uint8_t AT_COMMAND;


/*!
 * \brief Type socket
 *
 *
 * \details Type SOCKET
 *
 */
typedef uint8_t TCP_SOCKET;


/*!
 * \brief Type PORT
 *
 *
 * \details Type PORT
 *
 */
typedef uint16_t TCP_PORT;


/*Success or Error indicator*/
/*!
 * \brief Success/Error
 *
 *
 * \details Valid values ERROR and SUCCESS.
 *
 */
typedef enum{
	ERROR												= 0,					/*!<Error*/
	SUCCESS												= 1						/*!<Success*/
}SUCCESS_ERROR;


/*Boolean TRUE/FALSE*/
/*!
 * \brief Boolean result
 *
 *
 * \details Valid values FALSE and TRUE.
 *
 */
typedef enum{
	BOOLEAN_FALSE														= 0,			/*!<Boolean FALSE*/
	BOOLEAN_TRUE														= 1				/*!<Boolean TRUE*/
} BOOLEAN_DATA;


/*Command outcome*/
/*!
 * \brief Gainspan command outcome.
 *
 *
 * \details Valid values ERROR, SUCCESS, and NO RESPONSE
 *
 */
typedef enum{
	COMMAND_OUTCOME_ERROR												= 0,			/*!<Command outcome - ERROR*/
	COMMAND_OUTCOME_SUCCESS												= 1,			/*!<Command outcome - SUCCESS*/
	COMMAND_OUTCOME_NO_RESPONSE											= 2				/*!<Command outcome - NO RESPONSE*/
} COMMAND_OUTCOME;


/*Gainspan device connection status*/
/*!
 * \brief Gainspan device connection status.
 *
 *
 * \details Valid values GAINSPAN_ACTIVE_FALSE, GAINSPAN_ACTIVE_TRUE, and GAINSPAN_ACTIVE_TRUE_WITH_ERRORS.
 *
 */
typedef enum{
	GAINSPAN_ACTIVE_FALSE												= 0,			/*!<Gainspan Device NOT ACTIVE*/
	GAINSPAN_ACTIVE_TRUE												= 1,			/*!<Gainspan Device ACTIVE*/
	GAINSPAN_ACTIVE_TRUE_WITH_ERRORS									= 2				/*!<Gainspan Device ACTIVE with ERRORS*/
} GAINSPAN_ACTIVE;




/*Socket data availability status*/
/*!
 * \brief Data availability on socket status.
 *
 *
 * \details Valid values SOCKET_DATA_AVAILABLE_NO, or SOCKET_DATA_AVAILABLE_YES.
 *
 */
typedef enum {
	SOCKET_DATA_AVAILABLE_NO  		  							= 0,			/*!<Socket data not available*/
	SOCKET_DATA_AVAILABLE_YES  									= 1				/*!<Socket data available*/
} DATA_ON_SOCKET_STATUS ;


/*Protocols*/
/*!
 * \brief Protocols
 *
 *
 * \details Valid values PROTOCOL_TCP, PROTOCOL_UDP and PROTOCOL_UDP_CLIENT.
 * Note: current implementation only supports TCP.
 *
 */
typedef enum{
	PROTOCOL_TCP												= 6,						/*!<Protocol TCP*/
	PROTOCOL_UDP												= 7,						/*!<Protocol UDP*/
	PROTOCOL_UDP_CLIENT 										= 8							/*!<Protocol UDP Client*/
} PROTOCOLS;


/*Socket mode*/
/*!
 * \brief Socket mode - activation/enable or process
 *
 *
 * \details Socket mode - activation/enable or process.
 * 	- Activation/Enable - when socket has been configured for the first time and activated.
 * 	- Process - socket is communicating with client.
 *
 */
typedef enum{
	SOCKET_MODE_ENABLE											= 1,						/*!<Socket mode enable*/
	SOCKET_MODE_PROCESS											= 2 						/*!<Socket mode process*/
} SOCKET_MODE;



/*Socket connection status*/
/*!
 * \brief Socket connection status.
 *
 *
 * \details Valid values SOCKET_STATUS_CLOSED, SOCKET_STATUS_INIT, SOCKET_STATUS_LISTEN, SOCKET_STATUS_ESTABLISHED, or SOCKET_STATUS_CLOSE_WAIT.
 *
 */
typedef enum {
	SOCKET_STATUS_INVALID										= 255,			/*!<Invalid Socket*/
	SOCKET_STATUS_CLOSED    		  							= 0,			/*!<Socket closed*/
	SOCKET_STATUS_INIT        									= 1,			/*!<Socket initializing*/
	SOCKET_STATUS_LISTEN      									= 2,			/*!<Socket listening*/
	SOCKET_STATUS_ESTABLISHED 									= 3,			/*!<Socket connection established*/
	SOCKET_STATUS_CLOSE_WAIT  									= 4				/*!<Socket on close wait*/
} SOCKET_STATUS ;


/*Wireless mode*/
/*!
 * \brief Wireless modes
 *
 *
 * \details Valid wireless modes.
 *
 */
typedef enum {
	WIRELESS_MODE_INFRASTRUCTURE	  							= 0,		/*!<Infrastructure mode*/
	WIRELESS_MODE_ADHOC        									= 1,		/*!<Ad hoc network mode*/
	WIRELESS_MODE_LIMITEDAP    									= 2			/*!<Limited AP mode*/
} WIRELESS_MODE;


/* Baud Rate */
/*!
 * \brief Baud rates
 *
 *
 * \details Valid baud rates.
 *
 */
typedef enum {
	BAUD_RATE_9600												= 9600,		/*!<9600 bits per second*/
	BAUD_RATE_115200											= 115200	/*!<115200 bits per second*/
} BAUD_RATE;


/*Authentication mode*/
/*!
 * \brief Wireless authentication modes.
 *
 *
 * \details Valid Wireless authentication modes.
 *
 */
typedef enum {
	AUTHENTICATION_MODE_NONE	  								= 0,		/*!<NONE*/
	AUTHENTICATION_MODE_WEP_OPEN  								= 1,		/*!<WEP Open*/
	AUTHENTICATION_MODE_WEP_SHARED 		 		 		 		= 2			/*!<WEP shared*/
} AUTHENTICATION_MODE;


/*Transmission rate*/
/*!
 * \brief Wireless transmission rate.
 *
 *
 * \details Valid Wireless transmission rates.
 *
 */
typedef enum {
	TRANSMISSION_RATE_AUTO	  									= 0,		/*!<AUTO*/
	TRANSMISSION_RATE_1_MBPS	  								= 2,		/*!<1 MBPS*/
	TRANSMISSION_RATE_2_MBPS	  								= 4,		/*!<2 MBPS*/
	TRANSMISSION_RATE_5_5_MBPS	  								= 10,		/*!<5.5 MBPS*/
	TRANSMISSION_RATE_11_MBPS	  								= 22		/*!<11 MBPS*/
} TRANSMISSION_RATE;


/*Wireless channel*/
/*!
 * \brief Wireless channel.
 *
 *
 * \details Valid wireless channels for 2.4 GHz
 *
 */
typedef enum {
	WIRELESS_CHANNEL_1	  										= 1,		/*!<Channel - 1*/
	WIRELESS_CHANNEL_2	  										= 2,		/*!<Channel - 2*/
	WIRELESS_CHANNEL_3	  										= 3,		/*!<Channel - 3*/
	WIRELESS_CHANNEL_4	  										= 4,		/*!<Channel - 4*/
	WIRELESS_CHANNEL_5	  										= 5,		/*!<Channel - 5*/
	WIRELESS_CHANNEL_6	  										= 6,		/*!<Channel - 6*/
	WIRELESS_CHANNEL_7	  										= 7,		/*!<Channel - 7*/
	WIRELESS_CHANNEL_8	  										= 8,		/*!<Channel - 8*/
	WIRELESS_CHANNEL_9	  										= 9,		/*!<Channel - 9*/
	WIRELESS_CHANNEL_10	  										= 10,		/*!<Channel - 10*/
	WIRELESS_CHANNEL_11	  										= 11		/*!<Channel - 11*/
} WIRELESS_CHANNEL;


/*Wireless security configuration*/
/*!
 * \brief Wireless security configuration.
 *
 *
 * \details Valid wireless security configuration.
 *
 */
typedef enum {
	WIRELESS_SECURITY_CONFIGURATION_AUTO	  					= 0,		/*!<AUTO*/
	WIRELESS_SECURITY_CONFIGURATION_OPEN_SECURITY		  		= 1,		/*!<Open security*/
	WIRELESS_SECURITY_CONFIGURATION_WEP_SECURITY		  		= 2,		/*!<WEP security*/
	WIRELESS_SECURITY_CONFIGURATION_WPA_PSK_SECURITY			= 4,		/*!<WEP PSK security*/
	WIRELESS_SECURITY_CONFIGURATION_WPA2_PSK_SECURITY			= 8,		/*!<WEPs PSK security*/
	WIRELESS_SECURITY_CONFIGURATION_WPA_ENTERPRISE				= 16,		/*!<WEP Enterprise security*/
	WIRELESS_SECURITY_CONFIGURATION_WPA2_ENTERPRISE				= 32,		/*!<WEP2 Enterprise security*/
	WIRELESS_SECURITY_CONFIGURATION_WPA2_AES_TKIP_SECURITY		= 64		/*!<WEP2 AES TKIP security*/
} WIRELESS_SECURITY_CONFIGURATION;


/*Gainspan device operation mode*/
/*!
 * \brief Gainspan device operation mode.
 *
 *
 * \details Valid values GAINSPAN_DEVICE_MODE_COMMADN, GAINSPAN_DEVICE_MODE_DATA, and GAINSPAN_DEVICE_MODE_DATA_RX.
 *
 */
typedef enum{
	GAINSPAN_DEVICE_MODE_COMMAND										= 0,			/*!<Gainspan Device Mode COMMAND*/
	GAINSPAN_DEVICE_MODE_DATA											= 1,			/*!<Gainspan Device Mode DATA*/
	GAINSPAN_DEVICE_MODE_DATA_RX										= 2				/*!<Gainspan Device Mode Data Receive*/
} GAINSPAN_DEVICE_OPERATION_MODE;



/*Gainspan device wireless connection state*/
/*!
 * \brief Gainspan device wireless connection state.
 *
 *
 * \details Valid values DEVICE_CONNECTION_STATUS_DISCONNECTED, and DEVICE_CONNECTION_STATUS_CONNECTED.
 *
 */
typedef enum {
	DEVICE_CONNECTION_STATUS_DISCONNECTED						= 0,
	DEVICE_CONNECTION_STATUS_CONNECTED							= 1
} DEVICE_CONNECTION_STATUS;


/*WiFi - wireless connection profile*/
/*!
 * \brief Wireless connection profile.
 *
 *
 * \details Parameters for establishing wireless communication.
 *
 */
typedef struct _WIRELESS_PROFILE {
	char *ssid;																/*!<SSID*/
	char *security_key;														/*!<Security key*/
	WIRELESS_MODE wireless_mode;											/*!<Wirless mode*/
	AUTHENTICATION_MODE authentication_mode;								/*!<Wireless authentication mode*/
	WIRELESS_SECURITY_CONFIGURATION wireless_security_configuration;		/*!<Wireless security configuration*/
	TRANSMISSION_RATE transmission_rate;									/*!<Wireless transmission rate*/
	WIRELESS_CHANNEL wireless_channel;										/*!<Wireless channel*/
} WIRELESS_PROFILE;


/*Network profile*/
/*!
 * \brief Network connection profile.
 *
 *
 * \details Parameters for establishing network communication.
 *
 */
typedef struct _NETWORK_PROFILE {
	char *local_ip_address;													/*!<Device IP local address*/
	char *subnet;															/*!<Subnet*/
	char *gateway;															/*!<Gateway*/
} NETWORK_PROFILE;


/*Web-server authentication profile*/
/*!
 * \brief Web Server connection or authentication profile.
 *
 *
 * \details Parameters for establishing web server authentication for communication.
 *
 */
typedef struct _WEBSERVER_AUTHENTICATION_PROFILE {
	char *web_server_administrator_id;										/*!<Web server administration ID*/
	char *web_server_administrator_password;								/*!<Web server administration password*/
} WEBSERVER_AUTHENTICATION_PROFILE;


/******************************************************************************************************************/
/* CODING STANDARDS:
 * Header file: Section IV. Global   or   external   data   declarations -> externs, non­static globals, and then
 * 				static globals.
 *
 * Naming convention: variables names must be meaningful lower case and words joined with an underscore (_). Limit
 * 					  the  use  of  abbreviations.
 */


/* NO GLOBAL VARIABLES*/

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

/*Gainspan GS1011M WiFi APIs*/

void gs_initialize_module(USART_ID target_usart_id, BAUD_RATE target_baud_rate, USART_ID target_serial_terminal_usart_id, BAUD_RATE target_serial_terminal_baud_rate);

void gs_set_usart(USART_ID target_usart_id, BAUD_RATE target_baud_rate, USART_ID target_serial_terminal_usart_id, BAUD_RATE target_serial_terminal_baud_rate);

void gs_set_network_configuration(NETWORK_PROFILE target_network_profile);

void gs_set_wireless_configuration(WIRELESS_PROFILE target_wireless_profile);

void gs_set_wireless_ssid(char *wireless_ssid);

void gs_set_webserver_authentication(WEBSERVER_AUTHENTICATION_PROFILE target_webserver_profile);

GAINSPAN_ACTIVE gs_activate_wireless_connection(void);

SOCKET_STATUS gs_get_socket_status(TCP_SOCKET socket);

SUCCESS_ERROR gs_activate_socket(TCP_SOCKET socket);

TCP_SOCKET gs_get_active_socket(void);

PROTOCOLS gs_get_socket_protocol(TCP_SOCKET socket);

TCP_PORT gs_get_socket_port(TCP_SOCKET socket);

SUCCESS_ERROR gs_get_data_on_socket_status(TCP_SOCKET socket);

void gs_configure_socket(TCP_SOCKET socket, PROTOCOLS socket_protocol, TCP_PORT socket_port);

SUCCESS_ERROR  gs_enable_activate_socket(TCP_SOCKET socket);

SUCCESS_ERROR gs_reset_socket(TCP_SOCKET socket);

SUCCESS_ERROR gs_disconnect_deactivate_socket(TCP_SOCKET socket);

SUCCESS_ERROR gs_read_data_from_socket(char *data_string);

TCP_SOCKET gs_get_socket_having_active_connection_and_data(void);

SUCCESS_ERROR gs_get_socket_connection_status(TCP_SOCKET socket);

void gs_write_data_to_socket(TCP_SOCKET socket, char *data_string);

void gs_write_complete_to_socket(TCP_SOCKET socket);

void gs_flush(void);

/*Web server APIs*/

void configure_web_page(char *page_title, char *menu_title, HTML_ELEMENT_TYPE element_type);

void add_element_choice(char choice_identifier, char *element_label);

void start_web_server(void);

void process_client_request(void);

char get_next_client_response(void);

#endif /* WIRELESS_INTERFACE_H_ */


/*!@}*/   // end module
