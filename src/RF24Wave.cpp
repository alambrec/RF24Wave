#include "RF24Wave.h"


RF24Wave::RF24Wave(RF24Network& _network, RF24Mesh& _mesh, uint8_t NodeID, uint8_t *groups):
mesh(_mesh), network(_network)
{
  nodeID = NodeID;
  if(groups != NULL){
    groupsID = groups;
  }
};

bool RF24Wave::begin(){
  mesh.setNodeID(nodeID);
  nodeID = mesh.getNodeID();
  Serial.println(F("Connecting to the mesh..."));
  mesh.begin();
  /* Loop to init matrix */
  uint8_t i, j;
  for(i=0; i<MAX_GROUPS; i++){
    for(j=0; j<MAX_NODE_GROUPS; j++){
      listGroupsID[i][j] = 0;
    }
  }
  Serial.println(F("Connecting to the wave..."));
  if(nodeID != 0){
    while(!requestAssociations()){
      delay(1000);
    }
    return confirmAssociations();
  }else{
    return true;
  }
}

bool RF24Wave::requestAssociations()
{
  init_msg_t payload;
  payload.nodeID = nodeID;
  payload.groupsID = groupsID;
  mesh.update();
  if(!mesh.write(&payload, INIT_MSG_T, sizeof(init_msg_t), 0)){
    // If a write fails, check connectivity to the mesh network
    if(!mesh.checkConnection()){
      //refresh the network address
      Serial.println(F("[requestAssociations] ERROR: Renewing Address"));
      mesh.renewAddress();
    }
    return false;
  }else{
    return true;
  }
}

void RF24Wave::listen(){
  mesh.update();
  if(nodeID == 0){
    mesh.DHCP();
  }
  if(network.available()){
    RF24NetworkHeader header;
    network.peek(header);
    switch(header.type){
      case INIT_MSG_T:
        init_msg_t payload;
        network.read(header, &payload, sizeof(payload));
        if(checkAssociations(payload.nodeID, payload.groupsID)){
          // We send update to all nodes
        }
      default:
        break;
    }
  }
}

bool RF24Wave::confirmAssociations(){
  bool available = true;
  uint8_t i = 0;
  uint8_t length = 0;
  while(!associated){
    mesh.update();
    while(network.available()){
      RF24NetworkHeader header;
      network.peek(header);
      switch(header.type){
        case ACK_INIT_MSG_T:
          Serial.println(F("[confirmAssociations] INFO ACK_INIT_MSG_T BEGIN"));
          init_msg_t payload;
          network.read(header, &payload, sizeof(payload));
          if(payload.nodeID != nodeID){
            Serial.println(F("[confirmAssociations] ERROR nodeID"));
            available = false;
          }
          length = countGroups(payload.groupsID);
          for(i=0; i<length; i++){
            if(groupsID[i] != payload.groupsID[i]){
              Serial.print(F("[confirmAssociations] ERROR groupID"));
              Serial.println(groupsID[i]);
              available = false;
            };
          }
          Serial.println(F("[confirmAssociations] ADD LIST BEGIN"));
          addListAssociations(payload);
          printAssociations();
          associated = true;
          return available;
          break;
        default:
          break;
      }
    }
  }
  return available;
}

bool RF24Wave::checkAssociations(uint8_t ID, uint8_t *groups){
  uint8_t i;
  uint8_t length = countGroups(groups);
  bool available = true;
  for(i=0; i<length; i++){
    if(!checkGroup(ID, groups[i])){
      available = false;
      groups[i] = 0;
    }
  }
  init_msg_t payload;
  payload.nodeID = ID;
  payload.groupsID = groups;
  if(!mesh.write(&payload, ACK_INIT_MSG_T, sizeof(init_msg_t), ID)){
    Serial.println(F("[ERROR] unable to send response!"));
  }
  Serial.println(F("[requestAssociations] checkAss END"));
  return available;
}

bool RF24Wave::checkGroup(uint8_t ID, uint8_t group){
  uint8_t i;
  for(i=0; i<5; i++){
    /* We check if we can add ID to group or if ID exist in group ! */
    if(listGroupsID[group][i] == ID || listGroupsID[group][i] == 0){
      return true;
    }
  }
  return false;
}

void RF24Wave::addListAssociations(init_msg_t data){
  uint8_t i, j, length;
  uint8_t temp;
  bool added;
  length = countGroups(data.groupsID);
  for(i=0; i<length; i++){
    added = false;
    j = 0;
    //We add new association only if we found one zero in listGroupsID
    while(j < MAX_NODE_GROUPS && !added){
      temp = listGroupsID[data.groupsID[i]-1][j];
      if(temp == 0){
        listGroupsID[data.groupsID[i]-1][0] = data.nodeID;
        added = true;
      }else if(temp == data.nodeID){
        added = true;
      }else{
        j++;
      }
    }
    if(!added){
      Serial.print(F("[addListAssociations] ERR: Unable to add node to group : "));
      Serial.println(data.groupsID[i]);
    }
  }
}

void RF24Wave::broadcastAssociations(init_msg_t data){
  uint8_t i, length;
  uint8_t group = 0;
  length = countGroups(data.groupsID);
  for(i=0; i<length; i++){
    group = data.groupsID[i];
    if(group > 0){
      sendUpdateGroup(data.nodeID, group, listGroupsID[group-1]);
    }
  }
}

bool RF24Wave::sendUpdateGroup(uint8_t nodeID, uint8_t groupID, uint8_t *listGroupsID){
  uint8_t i;
  bool successful = true;
  for(i=0; i<5; i++){
    // We check that we only send update to other nodes. Not original node or controller !
    if((listGroupsID[i] != 0) && (listGroupsID[i] != nodeID)){
      update_msg_t payload;
      payload.nodeID = nodeID;
      payload.groupID = groupID;
      if(!mesh.write(&payload, UPDATE_MSG_T, sizeof(update_msg_t), 0)){
        successful = false;
      }
    }
  }
  return successful;
}



void RF24Wave::printAssociations(){
  uint8_t i, j;
  Serial.println(F("*** Matrix Associations ***"));
  for(i=0; i<MAX_GROUPS; i++){
    Serial.print(F("Group "));
    Serial.print(i+1);
    Serial.print(F(" : "));
    for(j=0; j<MAX_NODE_GROUPS; j++){
      Serial.print(listGroupsID[i][j]);
      Serial.print(F("|"));
    }
    Serial.println();
  }
  Serial.println();
}

uint8_t RF24Wave::countGroups(uint8_t *groups){
  uint8_t length=0;
  while(groups[length]){
    length++;
  }
  return length;
}
