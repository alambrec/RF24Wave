#ifndef MySensorsCore_h
#define MySensorsCore_h

#include "MyGatewayTransport.h"

#include <MyProtocol.h>
#include <MyMessage.h>
#include <stddef.h>
#include <stdarg.h>



// #ifndef GATEWAY_ADDRESS
// #define GATEWAY_ADDRESS			((uint8_t)0)			//!< Node ID for GW sketch
// #endif


#define NODE_SENSOR_ID			((uint8_t)255)			//!< Node child is always created/presented when a node is started
#define MY_CORE_VERSION			((uint8_t)2)			//!< core version
#define MY_CORE_MIN_VERSION		((uint8_t)2)			//!< min core version required for compatibility

#define MY_WAKE_UP_BY_TIMER		((int8_t)-1)			//!< Sleeping wake up by timer
#define MY_SLEEP_NOT_POSSIBLE	((int8_t)-2)			//!< Sleeping not possible
#define INTERRUPT_NOT_DEFINED	((uint8_t)255)			//!< _sleep() param: no interrupt defined
#define MODE_NOT_DEFINED		((uint8_t)255)			//!< _sleep() param: no mode defined
#define VALUE_NOT_DEFINED		((uint8_t)255)			//!< Value not defined
#define MYSENSORS_LIBRARY_VERSION "RF24Wave"


#ifdef MY_DEBUG
#define debug(x,...) hwDebugPrint(x, ##__VA_ARGS__)			//!< debug, to be removed (follow-up PR)
#define CORE_DEBUG(x,...) hwDebugPrint(x, ##__VA_ARGS__)	//!< debug
#else
#define debug(x,...)										//!< debug NULL, to be removed (follow-up PR)
#define CORE_DEBUG(x,...)									//!< debug NULL
#endif

// Inline function and macros
static inline MyMessage& build(MyMessage &msg, const uint8_t NID, const uint8_t destination, const uint8_t sensor,
                               const uint8_t command, const uint8_t type, const bool ack = false)
{
	msg.sender = NID;
	msg.destination = destination;
	msg.sensor = sensor;
	msg.type = type;
	mSetCommand(msg,command);
	mSetRequestAck(msg,ack);
	mSetAck(msg,false);
	return msg;
}

static inline MyMessage& buildGw(MyMessage &msg, const uint8_t type)
{
	msg.sender = GATEWAY_ADDRESS;
	msg.destination = GATEWAY_ADDRESS;
	msg.sensor = NODE_SENSOR_ID;
	msg.type = type;
	mSetCommand(msg, C_INTERNAL);
	mSetRequestAck(msg, false);
	mSetAck(msg, false);
	return msg;
}

#endif
