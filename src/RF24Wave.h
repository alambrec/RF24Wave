#ifndef __RF24WAVE_H
#define __RF24WAVE_H

#include <arduino.h>
#include <RF24Mesh.h>

/*
 * Message types 1-64 (decimal) will NOT be acknowledged by the network,
 * types 65-127 will be. Use as appropriate to manage traffic:
 * if expecting a response, no ack is needed.
 */

#define INIT_MSG_T 65
#define ACK_INIT_MSG_T 66
#define UPDATE_MSG_T 67

#define MAX_NODE_GROUPS 5
#define MAX_GROUPS 9

typedef struct{
  uint8_t nodeID;
  uint8_t *groupsID;
}init_msg_t;

typedef struct{
  uint8_t nodeID;
  uint8_t groupID;
}update_msg_t;

class RF24Mesh;
class RF24Network;

class RF24Wave
{
  public:
    RF24Wave(RF24Network& _network, RF24Mesh& _mesh, uint8_t NodeID=0, uint8_t *groups=NULL);
    bool begin();
    bool requestAssociations();
    bool confirmAssociations();
    void listen();
    bool checkAssociations(uint8_t ID, uint8_t *groups);
    bool checkGroup(uint8_t ID, uint8_t group);
    void printAssociations();
    void addListAssociations(init_msg_t data);
    uint8_t countGroups(uint8_t *groups);
    void broadcastAssociations(init_msg_t data);
    bool sendUpdateGroup(uint8_t nodeID, uint8_t groupID, uint8_t *listGroupsID);

  private:
    RF24Mesh& mesh;
    RF24Network& network;
    bool associated = false;
    uint8_t nodeID;
    /* Struct to stock different group ID proper to this node */
    uint8_t *groupsID;
    /* Matrix to stock nodeID for each different groupID */
    uint8_t listGroupsID[MAX_GROUPS][MAX_NODE_GROUPS];
};

#endif
