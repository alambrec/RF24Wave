/**
 * \file RF24Wave.h
 * \brief Class declaration for RF24Wave
 * \author LAMBRECHT.A
 * \version 0.5
 * \date 01-01-2017
 *
 * Implementation of Z-Wave protocol with nRF24l01
 *
 */

#ifndef __RF24WAVE_H
#define __RF24WAVE_H

#include <arduino.h>
#include <RF24Mesh.h>
#include <MyMessage.h>

#define MSG_GW_STARTUP_COMPLETE "Gateway startup complete."
#define LIBRARY_VERSION "RF24Wave 1.0"
#define NB_RETRY_SEND 10

#ifdef WAVE_DEBUG
#define P_DEBUG(x) Serial.println(F(x));
#else
#define P_DEBUG(x)
#endif

#ifdef WAVE_DEBUG
#define D_DEBUG(x) Serial.println(x);
#else
#define D_DEBUG(x)
#endif

#ifdef WAVE_DEBUG
#define F_DEBUG(x) x;
#else
#define F_DEBUG(x)
#endif

/**********************************
*  Gateway config
***********************************/

/**
 * @def MY_GATEWAY_MAX_RECEIVE_LENGTH
 * @brief Max buffersize needed for messages coming from controller.
 */
#ifndef MY_GATEWAY_MAX_RECEIVE_LENGTH
#define MY_GATEWAY_MAX_RECEIVE_LENGTH (60u) //100
#endif

/**
 * @def MY_GATEWAY_MAX_SEND_LENGTH
 * @brief Max buffer size when sending messages.
 */
#ifndef MY_GATEWAY_MAX_SEND_LENGTH
#define MY_GATEWAY_MAX_SEND_LENGTH (60u) //120
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


/***
 * Wave Message Types
 * This types determine what message is transmited through the network
 *
 * Message types 1-64 (decimal) will NOT be acknowledged by the network,
 * types 65-127 will be. Use as appropriate to manage traffic:
 * if expecting a response, no ack is needed.
 ***/
#define CONNECT_MSG_T           65
#define ACK_CONNECT_MSG_T       66
#define UPDATE_MSG_T            67
#define NOTIF_MSG_T             68
#define SYNCHRONIZE_MSG_T       69
#define ACK_SYNCHRONIZE_MSG_T   70
#define MY_MESSAGE_T            71

/**
 * \defgroup defConfig Library config
 * \brief Macros which defined some settings for this library
 * @{
 */

/** Maximum node assigned per groups */
#define MAX_NODE_GROUPS         5
/** Maximum groups */
#define MAX_GROUPS              9
/** Delay in ms between two print info */
#define PRINT_DELAY             5000

 /** @} */

void receive(const MyMessage &message)  __attribute__((weak));

/**
 * \struct info_node_t
 * \brief Structure of information message
 *
 * info_node_t is a structure that permits to send initial information of node
 * like nodeID or groups associated with the node.
 */
typedef struct{
  uint8_t nodeID;
  uint8_t groupsID[MAX_GROUPS];
}info_node_t;

typedef struct{
  uint8_t nodeID;
  uint8_t groupID;
}update_msg_t;

typedef struct{
  uint8_t nodeID;
  uint8_t listGroupsID[MAX_GROUPS][MAX_NODE_GROUPS];
}send_list_t;

struct broadcast_list{
  uint8_t nodeID;
  broadcast_list *next;
};

typedef broadcast_list broadcast_list_t;

class RF24Mesh;
class RF24Network;

class RF24Wave
{
 /**@}*/
 /**
  * @name RF24Wave
  *
  *  The implementation of Z-Wave with nRF24l01 and class documentation is currently in active development and usage may change.
  */
 /**@{*/
  public:
    /**
     * Construct the network based on Z-Wave protocol:
     *
     * @code
     * RF24 radio(7,8);
     * RF24Network network(radio);
     * RF24Mesh mesh(radio,network);
     * RF24Wave wave(network, mesh)
     * @endcode
     * @param _network The underlying network instance
     * @param _mesh The underlying mesh instance
     * @param NodeID The NodeID associated to node (0 for master)
     * @param groups Tab of groups associated to this node (null for master)
     */
    RF24Wave(RF24& _radio, RF24Network& _network, RF24Mesh& _mesh, uint8_t NodeID=0, uint8_t *groups=NULL);

/***************************** Common functions *****************************/
    void begin();
    void listen();

    void resetListGroup();
    void printAssociations();
    void addListAssociations(info_node_t data);
    void addAssociation(uint8_t NID, uint8_t GID);
    bool isPresent(uint8_t NID, uint8_t GID);
    void printAssociation(info_node_t data);
    uint8_t countGroups(uint8_t *groups);
    MyMessage& build(MyMessage &msg, const uint8_t NID, const uint8_t destID, const uint8_t childID,
                      const uint8_t command, const uint8_t type, const bool ack);
    uint8_t protocolH2i(char c);
    bool protocolParse(MyMessage &message, char *inputString);
    char* protocolFormat(MyMessage &message);


#if !defined(WAVE_MASTER)
/***************************** Node functions *******************************/
    void connect();
    bool requestAssociations();
    bool confirmAssociations();
    void synchronizeAssociations();
    void requestSynchronize();
    void confirmSynchronize();
    void receiveSynchronizedList(send_list_t msg);
    void broadcastNotifications(MyMessage &message);
    void sendNotifications(mysensor_sensor tSensor, mysensor_data tValue,
      mysensor_payload tPayload, int16_t payload);
    void sendNotifications(mysensor_sensor tSensor, mysensor_data tValue,
      mysensor_payload tPayload, int32_t payload);
    void printUpdate(update_msg_t data);
    void addNodeToBroadcastList(uint8_t NID);
    void createBroadcastList();
    void send(MyMessage &message);
    void sendSketchInfo(const char *name, const char *version);
    void present(const uint8_t childId, const uint8_t sensorType, const char *description = "");
    void sendMyMessage(MyMessage &message, uint8_t destID);

#else
/***************************** Master functions *****************************/
    bool checkAssociations(info_node_t *data);
    bool checkGroup(uint8_t ID, uint8_t group);
    void broadcastAssociations(info_node_t data);
    bool sendUpdateGroup(uint8_t NID, uint8_t GID, uint8_t *listNID);
    void sendSynchronizedList(info_node_t msg);
    void printNetwork();
    MyMessage& buildGw(MyMessage &msg, const uint8_t type);
    void gatewayTransportSend(MyMessage &message);
    void gatewayTransportInit();
    bool gatewayTransportAvailable();
    MyMessage& gatewayTransportReceive();
    void transmitMyMessage(MyMessage &message, uint8_t destID);

#endif

  private:
    RF24& radio;
    RF24Network& network;
    RF24Mesh& mesh;
    // global variables
    MyMessage _msgTmp;
    char _fmtBuffer[MY_GATEWAY_MAX_SEND_LENGTH];
    char _convBuffer[MAX_PAYLOAD*2+1];
    info_node_t info_payload;
    update_msg_t update_payload;
    send_list_t list_payload;

#if !defined(WAVE_MASTER)
    bool associated = false;
    bool synchronized = false;
    /* Struct to stock different group ID proper to this node */
    uint8_t groupsID[MAX_GROUPS];
    broadcast_list_t *headBroadcastList = NULL;
    uint8_t lengthBroadcastList = 0;
#else
    uint8_t _serialInputPos;
#endif
    uint8_t nodeID;
    /* Matrix to stock nodeID for each different groupID */
    uint8_t listGroupsID[MAX_GROUPS][MAX_NODE_GROUPS];
    uint32_t lastTimer;
};

#endif
