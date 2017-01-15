#include "RF24Wave.h"


RF24Wave::RF24Wave(RF24Network& _network, RF24Mesh& _mesh, uint8_t NodeID, uint8_t *groups):
mesh(_mesh), network(_network)
{
  nodeID = NodeID;
  uint8_t i, length;
  length = countGroups(groups);
  for(i=0; i<MAX_GROUPS; i++){
    groupsID[i] = 0;
  }
  if(groups != NULL){
    for(i=0; i<length; i++){
      groupsID[i] = groups[i];
    }
  }
};

void RF24Wave::begin()
{
  mesh.setNodeID(nodeID);
  nodeID = mesh.getNodeID();
  Serial.println(F("Connecting to the mesh..."));
  mesh.begin();
  lastTimer = millis();
  /* Loop to init matrix */
  uint8_t i, j;
  for(i=0; i<MAX_GROUPS; i++){
    for(j=0; j<MAX_NODE_GROUPS; j++){
      listGroupsID[i][j] = 0;
    }
  }
  Serial.println(F("Connecting to the wave..."));
  if(nodeID != 0){
    connect();
  }
}

void RF24Wave::connect()
{
  while(!associated){
    uint32_t currentTimer = millis();
    if(currentTimer - lastTimer > 5000){
      lastTimer = currentTimer;
      requestAssociations();
    }
    confirmAssociations();
  }
  Serial.println(F("Node connected"));
}

bool RF24Wave::requestAssociations()
{
  init_msg_t payload;
  uint8_t i;
  payload.nodeID = nodeID;
  for(i=0; i<MAX_GROUPS; i++){
    payload.groupsID[i] = groupsID[i];
  }
  printAssociation(payload);
  mesh.update();
  if(!mesh.write(&payload, INIT_MSG_T, sizeof(init_msg_t), 0)){
    Serial.println(F("[requestAssociations] ERROR: Unable to send data"));
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
        init_msg_t init_payload;
        network.read(header, &init_payload, sizeof(init_payload));
        printAssociation(init_payload);
        if(checkAssociations(&init_payload)){
          Serial.println(F("[listen] 1"));
          broadcastAssociations(init_payload);
          Serial.println(F("[listen] 2"));
          addListAssociations(init_payload);
          Serial.println(F("[listen] 3"));
          printAssociations();
        }
        break;
      case UPDATE_MSG_T:
        update_msg_t update_payload;
        network.read(header, &update_payload, sizeof(update_payload));
        printUpdate(update_payload);
        addListAssociation(update_payload);
        printAssociations();
        break;
      case NOTIF_MSG_T:
        notif_msg_t notification_payload;
        network.read(header, &notification_payload, sizeof(notification_payload));
        printNotification(notification_payload);
        break;
      default:
        break;
    }
  }
}

bool RF24Wave::confirmAssociations()
{
  bool available = true;
  associated = false;
  uint8_t i = 0;
  uint8_t length = 0;
  mesh.update();
  while(network.available()){
    RF24NetworkHeader header;
    network.peek(header);
    switch(header.type){
      case ACK_INIT_MSG_T:
        //Serial.println(F("[confirmAssociations] INFO ACK_INIT_MSG_T BEGIN"));
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
        //Serial.println(F("[confirmAssociations] ADD LIST BEGIN"));
        addListAssociations(payload);
        printAssociations();
        associated = true;
        break;
      default:
        break;
    }
  }
  return available;
}

bool RF24Wave::checkAssociations(init_msg_t *data)
{
  uint8_t i;
  bool available = true;
  for(i=0; i<MAX_GROUPS; i++){
    if(!checkGroup(data->nodeID, data->groupsID[i])){
      Serial.println(F("[ERROR] Unable group!"));
      available = false;
      data->groupsID[i] = 0;
    }
  }
  if(!mesh.write(data, ACK_INIT_MSG_T, sizeof(init_msg_t), data->nodeID)){
    Serial.println(F("[ERROR] unable to send response!"));
  }
  return available;
}

bool RF24Wave::checkGroup(uint8_t ID, uint8_t group)
{
  uint8_t i;
  /* if group == 0 then it's unnecessary to check if it's available */
  if(group > 0){
    for(i=0; i<MAX_NODE_GROUPS; i++){
      /* We check if we can add ID to group or if ID exist in group ! */
      if(listGroupsID[group-1][i] == ID || listGroupsID[group-1][i] == 0){
        return true;
      }
    }
    return false;
  }else{
    return true;
  }
}

void RF24Wave::addListAssociations(init_msg_t data)
{
  uint8_t i, j, temp, currentGroup;
  bool added;
  for(i=0; i<MAX_GROUPS; i++){
    added = false;
    j = 0;
    currentGroup = data.groupsID[i]-1;
    if(currentGroup >= 0){
      //We add new association only if we found one zero in listGroupsID
      while(j < MAX_NODE_GROUPS && !added){
        temp = listGroupsID[currentGroup][j];
        if(temp == 0){
          listGroupsID[currentGroup][j] = data.nodeID;
          added = true;
        }else if(temp == data.nodeID){
          added = true;
        }else{
          j++;
        }
      }
    }
    /*
    if(!added){
      Serial.print(F("[addListAssociations] ERR: Unable to add node to group : "));
      Serial.println(data.groupsID[i]);
    }else{
      Serial.print(F("[addListAssociations] INFO: Add node to group : "));
      Serial.println(data.groupsID[i]);
    }
    */
  }
}

void RF24Wave::addListAssociation(update_msg_t data)
{
  uint8_t i, temp;
  bool added = false;
  if(data.groupID > 0){
    //We add new association only if we found one zero in listGroupsID
    while(i < MAX_NODE_GROUPS && !added){
      temp = listGroupsID[data.groupID-1][i];
      if(temp == 0){
        listGroupsID[data.groupID-1][i] = data.nodeID;
        added = true;
      }else if(temp == data.nodeID){
        added = true;
      }else{
        i++;
      }
    }
  }
  if(!added){
    Serial.print(F("[addListAssociation] ERR: Unable to add node to group : "));
    Serial.println(data.groupID);
  }else{
    Serial.print(F("[addListAssociation] INFO: Add node to group : "));
    Serial.println(data.groupID);
  }
}

void RF24Wave::broadcastAssociations(init_msg_t data)
{
  uint8_t i;
  uint8_t group = 0;
  for(i=0; i<MAX_GROUPS; i++){
    group = data.groupsID[i];
    if(group > 0){
      if(!sendUpdateGroup(data.nodeID, group, listGroupsID[group-1])){
        Serial.println(F("[broadcastAssociations] ERR: Unable to send Update !"));
      }
    }
  }
}

bool RF24Wave::sendUpdateGroup(uint8_t NID, uint8_t GID, uint8_t *listNID)
{
  uint8_t i, currentNID;
  bool successful = true;
  for(i=0; i<MAX_NODE_GROUPS; i++){
    /* We catch the nodeID associated to groupID and we check if we must send update */
    currentNID = listNID[i];
    /* We check that we only send update to other nodes. Not original node or controller ! */
    if((currentNID > 0) && (currentNID != NID)){
      update_msg_t payload;
      payload.nodeID = NID;
      payload.groupID = GID;
      if(!mesh.write(&payload, UPDATE_MSG_T, sizeof(update_msg_t), currentNID)){
        successful = false;
        Serial.println(F("[sendUpdateGroup] ERR: Unable to send Update !"));
        Serial.print(F("[sendUpdateGroup] NID: "));
        Serial.print(payload.nodeID);
        Serial.print(F(" GID: "));
        Serial.print(payload.groupID);
        Serial.println("");
      }
    }
  }
  return successful;
}



void RF24Wave::printAssociations()
{
  uint8_t i, j;
  Serial.println(F("# Matrix Associations :"));
  for(i=0; i<MAX_GROUPS; i++){
    Serial.print(F("> GroupID "));
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

void RF24Wave::printAssociation(init_msg_t data)
{
  uint8_t i;
  Serial.println(F("# Print Associations :"));
  Serial.print(F("> NodeID: "));
  Serial.println(data.nodeID);
  Serial.print(F("> GroupID: "));
  for(i=0; i<MAX_GROUPS; i++){
    Serial.print(data.groupsID[i]);
    Serial.print("; ");
  }
  Serial.println();
}

void RF24Wave::printUpdate(update_msg_t data)
{
  Serial.println(F("# Print Update :"));
  Serial.print(F("> NodeID: "));
  Serial.println(data.nodeID);
  Serial.print(F("> GroupID: "));
  Serial.println(data.groupID);
  Serial.println();
}

uint8_t RF24Wave::countGroups(uint8_t *groups)
{
  uint8_t length=0;
  while(groups[length]){
    length++;
  }
  return length;
}

void RF24Wave::printNetwork()
{
  uint32_t currentTimer = millis();
  if(currentTimer - lastTimer > 5000){
    lastTimer = currentTimer;
    Serial.println(" ");
    Serial.println(F("********Assigned Addresses********"));
    for(int i=0; i<mesh.addrListTop; i++){
      Serial.print("NodeID: ");
      Serial.print(mesh.addrList[i].nodeID);
      Serial.print(" RF24Network Address: 0");
      Serial.println(mesh.addrList[i].address, OCT);
    }
    Serial.println(F("**********************************"));
  }
}

void RF24Wave::broadcastNotifications(MyMessage &message)
{
  notif_msg_t data;
  uint32_t currentTimer = millis();
  if(currentTimer - lastTimer > 5000){
    lastTimer = currentTimer;
    uint8_t i, j, currentGroup, currentDstID;
    for(i=0; i<MAX_GROUPS; i++){
      currentGroup = groupsID[i];
      if(currentGroup > 0){
        for(j=0; j<MAX_NODE_GROUPS; j++){
          currentDstID = listGroupsID[currentGroup-1][j];
          if((currentDstID > 0) && (currentDstID != nodeID)){
            message.setDestination(currentDstID);
            strncpy(data.myMessage, protocolFormat(message), MY_GATEWAY_MAX_SEND_LENGTH);
            //memcpy(data.myMessage, protocolFormat(message), sizeof(data.myMessage));
            Serial.println(F("[broadcastNotifications] Send notification"));
            mesh.write(&data, NOTIF_MSG_T, sizeof(data), currentDstID);
          }
        }
      }
    }
  }
}

void RF24Wave::printNotification(notif_msg_t data)
{
  MyMessage message;
  if(protocolParse(message, data.myMessage)){
    Serial.println(F("> Notification received !"));
    Serial.print(F("Node Dest: "));
    Serial.println(message.destination);
    Serial.print(F("Type: "));
    Serial.println(message.type);
    // Serial.print(F("Value: "));
    // Serial.println(data.value);
    Serial.println();
  }

}
