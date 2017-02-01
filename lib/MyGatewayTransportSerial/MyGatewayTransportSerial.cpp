#include <MyMessage.h>
#include <MyProtocol.h>
#include "MyGatewayTransport.h"


// global variables
MyMessage _msgTmp;

char _serialInputString[MY_GATEWAY_MAX_RECEIVE_LENGTH];    // A buffer for incoming commands from serial interface
uint8_t _serialInputPos;
MyMessage _serialMsg;

bool gatewayTransportSend(MyMessage &message)
{
	//setIndication(INDICATION_GW_TX);
	MY_SERIALDEVICE.print(protocolFormat(message));
	// Serial print is always successful
	return true;
}

bool gatewayTransportInit(void)
{
	(void)gatewayTransportSend(buildGw(_msgTmp, I_GATEWAY_READY).set(MSG_GW_STARTUP_COMPLETE));
	// Send presentation of locally attached sensors (and node if applicable)
	//presentNode();
	return true;
}

bool gatewayTransportAvailable(void)
{
	while (MY_SERIALDEVICE.available()) {
		// get the new byte:
		const char inChar = (char)MY_SERIALDEVICE.read();
		// if the incoming character is a newline, set a flag
		// so the main loop can do something about it:
		if (_serialInputPos < MY_GATEWAY_MAX_RECEIVE_LENGTH - 1) {
			if (inChar == '\n') {
				_serialInputString[_serialInputPos] = 0;
				const bool ok = protocolParse(_serialMsg, _serialInputString);
				if (ok) {
					//setIndication(INDICATION_GW_RX);
				}
				_serialInputPos = 0;
				return ok;
			} else {
				// add it to the inputString:
				_serialInputString[_serialInputPos] = inChar;
				_serialInputPos++;
			}
		} else {
			// Incoming message too long. Throw away
			_serialInputPos = 0;
		}
	}
	return false;
}

MyMessage & gatewayTransportReceive(void)
{
	// Return the last parsed message
	return _serialMsg;
}

bool present(const uint8_t NID, const uint8_t childSensorId, const uint8_t sensorType,
                            const char *description, const bool ack)
{
  return gatewayTransportSend(build(_msgTmp, NID, GATEWAY_ADDRESS, childSensorId, C_PRESENTATION,
      sensorType, ack).set(NID==GATEWAY_ADDRESS?MYSENSORS_LIBRARY_VERSION:description));
}

bool sendSketchInfo(const uint8_t NID, const uint8_t CID, const char *name, const char *version, const bool ack)
{
	bool result = true;
	if (name) {
		result &= gatewayTransportSend(build(_msgTmp, NID, GATEWAY_ADDRESS, CID, C_INTERNAL, I_SKETCH_NAME,
		                           ack).set(name));
	}
	if (version) {
		result &= gatewayTransportSend(build(_msgTmp, NID, GATEWAY_ADDRESS, CID, C_INTERNAL, I_SKETCH_VERSION,
		                           ack).set(version));
	}
	return result;
}

void send(MyMessage &message, uint8_t NID)
{
	//message.sender = getNodeId();
  if(NID > 0){
    message.sender = NID;
  }
	mSetCommand(message, C_SET);
	mSetRequestAck(message, false);
  gatewayTransportSend(message);
}

// Inline function and macros
MyMessage& build(MyMessage &msg, const uint8_t NID, const uint8_t destination, const uint8_t CID,
                               const uint8_t command, const uint8_t type, const bool ack)
{
	msg.sender = NID;
	msg.destination = destination;
	msg.sensor = CID;
	msg.type = type;
	mSetCommand(msg,command);
	mSetRequestAck(msg,ack);
	mSetAck(msg,false);
	return msg;
}

MyMessage& buildGw(MyMessage &msg, const uint8_t type)
{
	msg.sender = GATEWAY_ADDRESS;
	msg.destination = GATEWAY_ADDRESS;
	msg.sensor = 255;
	msg.type = type;
	mSetCommand(msg, C_INTERNAL);
	mSetRequestAck(msg, false);
	mSetAck(msg, false);
	return msg;
}
