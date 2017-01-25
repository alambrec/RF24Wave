/**
 * \file RF24Wave.cpp
 * \brief Class definition for RF24Wave
 * \author LAMBRECHT.A
 * \version 0.5
 * \date 01-01-2017
 *
 * Implementation of Z-Wave protocol with nRF24l01
 *
 */

#include "RF24Wave.h"

/***************************** Constructor **********************************/
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



/***************************** Common functions *****************************/

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
#if !defined(WAVE_MASTER)
  connect();
  synchronizeAssociations();
#endif
}

void RF24Wave::listen(){
  mesh.update();
#if defined(WAVE_MASTER)
  mesh.DHCP();
#endif
  if(network.available()){
    RF24NetworkHeader header;
    network.peek(header);
    switch(header.type){
#if defined(WAVE_MASTER)
      case CONNECT_MSG_T:
        info_node_t init_payload;
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
      case SYNCHRONIZE_MSG_T:
        info_node_t synchronize_payload;
        network.read(header, &synchronize_payload, sizeof(synchronize_payload));
        sendSynchronizedList(synchronize_payload);
        break;
#else
      case UPDATE_MSG_T:
        update_msg_t update_payload;
        network.read(header, &update_payload, sizeof(update_payload));
        printUpdate(update_payload);
        addAssociation(update_payload.nodeID, update_payload.groupID);
        printAssociations();
        break;
      case NOTIF_MSG_T:
        notif_msg_t notification_payload;
        network.read(header, &notification_payload, sizeof(notification_payload));
        printNotification(notification_payload);
        break;
#endif
      default:
        break;
    }
  }
}

void RF24Wave::addListAssociations(info_node_t data)
{
  uint8_t i;
  for(i=0; i<MAX_GROUPS; i++){
    addAssociation(data.nodeID, data.groupsID[i]);
  }
}

void RF24Wave::addAssociation(uint8_t NID, uint8_t GID)
{
  uint8_t i, temp;
  bool added = false;
  if(GID > 0 && NID > 0){
    i = 0;
    //We add new association only if we found one zero in listGroupsID
    while(i < MAX_NODE_GROUPS && !added){
      temp = listGroupsID[GID-1][i];
      if(temp == 0){
        listGroupsID[GID-1][i] = NID;
        added = true;
      }else if(temp == NID){
        added = true;
      }
      i++;
    }
    if(!added){
      Serial.print(F("[addListAssociation] ERR: Unable to add node to group : "));
      Serial.println(GID);
    }
  }
}

bool RF24Wave::isPresent(uint8_t NID, uint8_t GID){
  uint8_t i;
  if(NID > 0 && GID > 0){
    for(i=0; i<MAX_GROUPS; i++){
      if(listGroupsID[GID-1][i] == NID){
        return true;
      }
    }
  }
  return false;
}

uint8_t RF24Wave::countGroups(uint8_t *groups)
{
  uint8_t length=0;
  while(groups[length]){
    length++;
  }
  return length;
}

void RF24Wave::printAssociation(info_node_t data)
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

#if !defined(WAVE_MASTER)
/***************************** Node functions *******************************/
//#error NODE_ENABLED
void RF24Wave::connect()
{
  while(!associated){
    uint32_t currentTimer = millis();
    mesh.update();
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
  info_node_t payload;
  uint8_t i;
  payload.nodeID = nodeID;
  for(i=0; i<MAX_GROUPS; i++){
    payload.groupsID[i] = groupsID[i];
  }
  printAssociation(payload);
  mesh.update();
  if(!mesh.write(&payload, CONNECT_MSG_T, sizeof(payload), 0)){
    // Serial.println(F("[requestAssociations] ERROR: Unable to send data"));
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

bool RF24Wave::confirmAssociations()
{
  bool available = true;
  associated = false;
  uint8_t i = 0;
  //mesh.update();
  while(network.available()){
    RF24NetworkHeader header;
    network.peek(header);
    if(header.type == ACK_CONNECT_MSG_T){
        //Serial.println(F("[confirmAssociations] INFO ACK_INIT_MSG_T BEGIN"));
        info_node_t payload;
        network.read(header, &payload, sizeof(payload));
        if(payload.nodeID != nodeID){
          Serial.println(F("[confirmAssociations] ERROR nodeID"));
          available = false;
        }
        for(i=0; i<MAX_GROUPS; i++){
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
    }
  }
  return available;
}

void RF24Wave::synchronizeAssociations(){
  synchronized = false;
  while(!synchronized){
    uint32_t currentTimer = millis();
    mesh.update();
    if(currentTimer - lastTimer > 5000){
      lastTimer = currentTimer;
      requestSynchronize();
    }
    confirmSynchronize();
  }
  Serial.println(F("Node synchronized"));
  printAssociations();
}

void RF24Wave::confirmSynchronize(){
  while(network.available()){
    RF24NetworkHeader header;
    network.peek(header);
    if(header.type == ACK_SYNCHRONIZE_MSG_T){
        //Serial.println(F("[confirmAssociations] INFO ACK_INIT_MSG_T BEGIN"));
        send_list_t payload;
        network.read(header, &payload, sizeof(payload));
        if(payload.nodeID != nodeID){
          Serial.println(F("[confirmSynchronize] ERROR nodeID"));
          return;
        }
        receiveSynchronizedList(payload);
        synchronized = true;
    }
  }
}

void RF24Wave::receiveSynchronizedList(send_list_t msg){
  uint8_t i, j;
  if(msg.nodeID == nodeID){
    for(i=0; i<MAX_GROUPS; i++){
      for(j=0; j<MAX_NODE_GROUPS; j++){
        addAssociation(msg.listGroupsID[i][j], i+1);
      }
    }
  }
}

void RF24Wave::requestSynchronize(){
  info_node_t payload;
  uint8_t i;
  payload.nodeID = nodeID;
  for(i=0; i<MAX_GROUPS; i++){
    payload.groupsID[i] = groupsID[i];
  }
  mesh.update();
  if(!mesh.write(&payload, SYNCHRONIZE_MSG_T, sizeof(payload), 0)){
    // If a write fails, check connectivity to the mesh network
    if(!mesh.checkConnection()){
      //refresh the network address
      Serial.println(F("[requestSynchronize] ERROR: Renewing Address"));
      mesh.renewAddress();
    }
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

void RF24Wave::printUpdate(update_msg_t data)
{
  Serial.println(F("# Print Update :"));
  Serial.print(F("> NodeID: "));
  Serial.println(data.nodeID);
  Serial.print(F("> GroupID: "));
  Serial.println(data.groupID);
  Serial.println();
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



#else
/***************************** Master functions *****************************/

bool RF24Wave::checkAssociations(info_node_t *data)
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
  if(!mesh.write(data, ACK_CONNECT_MSG_T, sizeof(info_node_t), data->nodeID)){
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

void RF24Wave::sendSynchronizedList(info_node_t msg){
  send_list_t payload;
  uint8_t i, j, currentGroup;
  mesh.update();
  /* We initialize array with zeros */
  for(i=0; i<MAX_GROUPS; i++){
    for(j=0; j<MAX_NODE_GROUPS; j++){
      payload.listGroupsID[i][j] = 0;
    }
  }
  payload.nodeID = msg.nodeID;
  for(i=0; i<MAX_GROUPS; i++){
    currentGroup = msg.groupsID[i];
    /* We check if node is realy present in group */
    if(isPresent(msg.nodeID, currentGroup)){
      for(j=0; j<MAX_NODE_GROUPS; j++){
        /* We copy all nodes associated with this group */
        payload.listGroupsID[currentGroup-1][j] = listGroupsID[currentGroup-1][j];
      }
    }
  }
  if(!mesh.write(&payload, ACK_SYNCHRONIZE_MSG_T, sizeof(payload), payload.nodeID)){
    Serial.println(F("[sendSynchronizedList] ERROR: Unable to send response to node !"));
  }
}

void RF24Wave::broadcastAssociations(info_node_t data)
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
      if(!mesh.write(&payload, UPDATE_MSG_T, sizeof(payload), currentNID)){
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

#endif
