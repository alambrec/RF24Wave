#ifndef __RF24WAVE_H
#define __RF24WAVE_H

#include <arduino.h>
#include <RF24Mesh.h>
#include <MyProtocol.h>

/*
 * Message types 1-64 (decimal) will NOT be acknowledged by the network,
 * types 65-127 will be. Use as appropriate to manage traffic:
 * if expecting a response, no ack is needed.
 */

#define CONNECT_MSG_T           65
#define ACK_CONNECT_MSG_T       66
#define UPDATE_MSG_T            67
#define NOTIF_MSG_T             68
#define SYNCHRONIZE_MSG_T       69
#define ACK_SYNCHRONIZE_MSG_T   70

#define MAX_NODE_GROUPS         5
#define MAX_GROUPS              9

/*
 * Define to set the delay between two print serial
 */
#define PRINT_DELAY

typedef struct{
  uint8_t nodeID;
  uint8_t groupsID[MAX_GROUPS];
}info_node_t;

typedef struct{
  char myMessage[MY_GATEWAY_MAX_SEND_LENGTH];
}notif_msg_t;

typedef struct{
  uint8_t nodeID;
  uint8_t groupID;
}update_msg_t;

typedef struct{
  uint8_t nodeID;
  uint8_t listGroupsID[MAX_GROUPS][MAX_NODE_GROUPS];
}send_list_t;

class RF24Mesh;
class RF24Network;

class RF24Wave
{
  public:
    RF24Wave(RF24Network& _network, RF24Mesh& _mesh, uint8_t NodeID=0, uint8_t *groups=NULL);
    void begin();
    void listen();
    void connect();

    bool requestAssociations();
    bool confirmAssociations();

    bool checkAssociations(info_node_t *data);
    bool checkGroup(uint8_t ID, uint8_t group);
    void printAssociations();
    void addListAssociations(info_node_t data);
    uint8_t countGroups(uint8_t *groups);
    void broadcastAssociations(info_node_t data);
    bool sendUpdateGroup(uint8_t NID, uint8_t GID, uint8_t *listNID);
    void printNetwork();
    void printAssociation(info_node_t data);
    void printUpdate(update_msg_t data);
    void addAssociation(uint8_t NID, uint8_t GID);
    void broadcastNotifications(MyMessage &message);
    void printNotification(notif_msg_t data);
    bool isPresent(uint8_t NID, uint8_t GID);
    void requestSynchronize();
    void sendSynchronizedList(info_node_t msg);
    void synchronizeAssociations();
    void receiveSynchronizedList(send_list_t msg);
    void confirmSynchronize();

  private:
    RF24Mesh& mesh;
    RF24Network& network;
    bool associated = false;
    bool synchronized = false;
    uint8_t nodeID;
    /* Struct to stock different group ID proper to this node */
    uint8_t groupsID[MAX_GROUPS];
    /* Matrix to stock nodeID for each different groupID */
    uint8_t listGroupsID[MAX_GROUPS][MAX_NODE_GROUPS];
    uint32_t lastTimer;
};

#endif
