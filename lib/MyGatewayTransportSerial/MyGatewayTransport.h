#ifndef MyGatewayTransport_h
#define MyGatewayTransport_h

#include <MyProtocol.h>
//#include "MySensorsCore.h"

#ifndef MY_SERIALDEVICE
#define MY_SERIALDEVICE Serial
#endif

#define MSG_GW_STARTUP_COMPLETE "Gateway startup complete."
#define MYSENSORS_LIBRARY_VERSION "RF24Wave"

// Common gateway functions

void gatewayTransportProcess(void);


// Gateway "interface" functions

/**
 * initialize the driver
 */
bool gatewayTransportInit(void);

/**
 * Send message to controller
 */
bool gatewayTransportSend(MyMessage &message);

/*
 * Check if a new message is available from controller
 */
bool gatewayTransportAvailable(void);

/*
 * Pick up last message received from controller
 */
MyMessage& gatewayTransportReceive(void);

/**
* Each node must present all attached sensors before any values can be handled correctly by the controller.
* It is usually good to present all attached sensors after power-up in setup().
*
* @param sensorId Select a unique sensor id for this sensor. Choose a number between 0-254.
* @param sensorType The sensor type. See sensor typedef in MyMessage.h.
* @param description A textual description of the sensor.
* @param ack Set this to true if you want destination node to send ack back to this node. Default is not to request any ack.
* @param description A textual description of the sensor.
* @return true Returns true if message reached the first stop on its way to destination.
*/
bool present(const uint8_t NID, const uint8_t childSensorId, const uint8_t sensorType,
                            const char *description="", const bool ack = false);

/**
 * Sends sketch meta information to the gateway. Not mandatory but a nice thing to do.
 * @param name String containing a short Sketch name or NULL  if not applicable
 * @param version String containing a short Sketch version or NULL if not applicable
 * @param ack Set this to true if you want destination node to send ack back to this node. Default is not to request any ack.
 * @return true Returns true if message reached the first stop on its way to destination.
 */
bool sendSketchInfo(const uint8_t NID, const uint8_t CID, const char *name, const char *version, const bool ack = false);

/**
* Sends a message to gateway or one of the other nodes in the radio network
*
* @param msg Message to send
* @param ack Set this to true if you want destination node to send ack back to this node. Default is not to request any ack.
* @return true Returns true if message reached the first stop on its way to destination.
*/
void send(MyMessage &msg, uint8_t NID=0);

MyMessage& build(MyMessage &msg, const uint8_t NID, const uint8_t destination, const uint8_t CID,
                               const uint8_t command, const uint8_t type, const bool ack = false);

MyMessage& buildGw(MyMessage &msg, const uint8_t type);

#endif /* MyGatewayTransportEthernet_h */
