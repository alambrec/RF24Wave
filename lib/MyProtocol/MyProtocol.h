/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef MyProtocol_h
#define MyProtocol_h

#include <MyMessage.h>
#define snprintf_P(...) snprintf( __VA_ARGS__ )

/**********************************
*  Gateway config
***********************************/

/**
 * @def MY_GATEWAY_MAX_RECEIVE_LENGTH
 * @brief Max buffersize needed for messages coming from controller.
 */
#ifndef MY_GATEWAY_MAX_RECEIVE_LENGTH
#define MY_GATEWAY_MAX_RECEIVE_LENGTH (100u)
#endif

/**
 * @def MY_GATEWAY_MAX_SEND_LENGTH
 * @brief Max buffer size when sending messages.
 */
#ifndef MY_GATEWAY_MAX_SEND_LENGTH
#define MY_GATEWAY_MAX_SEND_LENGTH (120u)
#endif

/**
 * @def MY_GATEWAY_MAX_CLIENTS
 * @brief Max number of parallel clients (sever mode).
 */
#ifndef MY_GATEWAY_MAX_CLIENTS
#define MY_GATEWAY_MAX_CLIENTS (1u)
#endif


/**
 * @def GATEWAY_ADDRESS
 * @brief NODE_ID = 0 for gateway
 */
#ifndef GATEWAY_ADDRESS
#define GATEWAY_ADDRESS (0u)
#endif

uint8_t protocolH2i(char c);

// parse(message, inputString)
// parse a string into a message element
// returns true if successfully parsed the input string
bool protocolParse(MyMessage &message, char *inputString);

// Format MyMessage to the protocol represenataion
char *protocolFormat(MyMessage &message);

#endif
