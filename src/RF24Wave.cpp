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
#if !defined(WAVE_MASTER)
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
#endif

};



/***************************** Common functions *****************************/

void RF24Wave::begin()
{
  mesh.setNodeID(nodeID);
  nodeID = mesh.getNodeID();
#if defined(WAVE_DEBUG)
  Serial.println(F("Connecting to the mesh..."));
#endif
  mesh.begin();
  lastTimer = millis();
  resetListGroup();
#if defined(WAVE_DEBUG)
  Serial.println(F("Connecting to the wave..."));
#endif
#if !defined(WAVE_MASTER)
  connect();
  synchronizeAssociations();
#else
  gatewayTransportInit();
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
#if defined(WAVE_MASTER_DEBUG)
        printAssociation(init_payload);
#endif
        if(checkAssociations(&init_payload)){
#if defined(WAVE_MASTER_DEBUG)
          Serial.println(F("[listen] 1"));
#endif
          broadcastAssociations(init_payload);
#if defined(WAVE_MASTER_DEBUG)
          Serial.println(F("[listen] 2"));
#endif
          addListAssociations(init_payload);
#if defined(WAVE_MASTER_DEBUG)
          Serial.println(F("[listen] 3"));
          printAssociations();
#endif
        }
        break;
      case SYNCHRONIZE_MSG_T:
        info_node_t synchronize_payload;
        network.read(header, &synchronize_payload, sizeof(synchronize_payload));
        sendSynchronizedList(synchronize_payload);
        break;
      case MY_MESSAGE_T:
        //Serial.println(F("[MY_MESSAGE_T] Received !"));
        network.read(header, _fmtBuffer, MY_GATEWAY_MAX_SEND_LENGTH);
        Serial.print(_fmtBuffer);
        if(protocolParse(_msgTmp, _fmtBuffer)){
#if defined(WAVE_MASTER_DEBUG)
          Serial.println(F("[MY_MESSAGE_T] parse ok !"));
#endif
        }
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
        Serial.println(F("[notif received] !"));

        network.read(header, &notification_payload, sizeof(notification_payload));
        printNotification(notification_payload);
        break;
#endif
      default:
        break;
    }
  }
}

void RF24Wave::resetListGroup(){
  uint8_t i, j;
  /* Loop to init matrix */
  for(i=0; i<MAX_GROUPS; i++){
    for(j=0; j<MAX_NODE_GROUPS; j++){
      listGroupsID[i][j] = 0;
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

MyMessage& RF24Wave::build(MyMessage &msg, const uint8_t NID, const uint8_t destID,
                            const uint8_t childID, const uint8_t command,
                            const uint8_t type, const bool ack)
{
	msg.sender = NID;
	msg.destination = destID;
	msg.sensor = childID;
	msg.type = type;
	mSetCommand(msg, command);
	mSetRequestAck(msg, ack);
	mSetAck(msg, false);
	return msg;
}

uint8_t RF24Wave::protocolH2i(char c)
{
	uint8_t i = 0;
	if (c <= '9') {
		i += c - '0';
	} else if (c >= 'a') {
		i += c - 'a' + 10;
	} else {
		i += c - 'A' + 10;
	}
	return i;
}

bool RF24Wave::protocolParse(MyMessage &message, char *inputString)
{
	char *str, *p, *value=NULL;
	uint8_t bvalue[MAX_PAYLOAD];
	uint8_t blen = 0;
	int i = 0;
	uint8_t command = 0;

	// Extract command data coming on serial line
	for (str = strtok_r(inputString, ";", &p); // split using semicolon
	        str && i < 6; // loop while str is not null an max 5 times
	        str = strtok_r(NULL, ";", &p) // get subsequent tokens
	    ) {
		switch (i) {
		case 0: // Radioid (destination)
			message.destination = atoi(str);
			break;
		case 1: // Childid
			message.sensor = atoi(str);
			break;
		case 2: // Message type
			command = atoi(str);
			mSetCommand(message, command);
			break;
		case 3: // Should we request ack from destination?
			mSetRequestAck(message, atoi(str)?1:0);
			break;
		case 4: // Data type
			message.type = atoi(str);
			break;
		case 5: // Variable value
			if (command == C_STREAM) {
				blen = 0;
				while (*str) {
					uint8_t val;
					val = protocolH2i(*str++) << 4;
					val += protocolH2i(*str++);
					bvalue[blen] = val;
					blen++;
				}
			} else {
				value = str;
				// Remove trailing carriage return and newline character (if it exists)
				uint8_t lastCharacter = strlen(value)-1;
				if (value[lastCharacter] == '\r') {
					value[lastCharacter] = 0;
				}
				if (value[lastCharacter] == '\n') {
					value[lastCharacter] = 0;
				}
			}
			break;
		}
		i++;
	}
	//debug(PSTR("Received %d"), i);
	// Check for invalid input
	if (i < 5) {
		return false;
	}
	message.sender = GATEWAY_ADDRESS;
	message.last = GATEWAY_ADDRESS;
	mSetAck(message, false);
	if (command == C_STREAM) {
		message.set(bvalue, blen);
	} else {
		message.set(value);
	}
	return true;
}

char* RF24Wave::protocolFormat(MyMessage &message)
{
	snprintf_P(_fmtBuffer, MY_GATEWAY_MAX_SEND_LENGTH, PSTR("%d;%d;%d;%d;%d;%s\n"), message.sender,
	           message.sensor, (uint8_t)mGetCommand(message), (uint8_t)mGetAck(message), message.type,
	           message.getString(_convBuffer));
	return _fmtBuffer;
}

#if !defined(WAVE_MASTER)
/***************************** Node functions *******************************/
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

// void RF24Wave::broadcastNotifications(MyMessage &message)
// {
//   //notif_msg_t data;
//   uint32_t currentTimer = millis();
//   uint8_t i;
//   if(currentTimer - lastTimer > 5000){
//     lastTimer = currentTimer;
//     createBroadcastList();
//     broadcast_list_t *currentElt = headBroadcastList;
//     while(currentElt){
//       i=0;
//       message.setDestination(currentElt->nodeID);
//       //char * temp = message.data;
//       //strncpy(data.myMessage, protocolFormat(message), MY_GATEWAY_MAX_SEND_LENGTH);
//       //memcpy(data.myMessage, protocolFormat(message), sizeof(data.myMessage));
//       Serial.print(F("[broadcastNotifications] Send notification to "));
//       Serial.println(currentElt->nodeID);
//
//       if(message.data){
//         Serial.println(F("[broadcastNotifications] Data send :"));
//         while(message.data[i] != '\0'){
//           Serial.print(message.data[i], HEX);
//           i++;
//         }
//       }
//       else{
//         Serial.println(F("[broadcastNotifications] Null Data send"));
//       }
//       Serial.println("");
//       /*
//       if(!mesh.write(&data, NOTIF_MSG_T, sizeof(data), currentElt->nodeID)){
//         Serial.println(F("[broadcastNotifications] Unable to send notification "));
//       }
//       */
//       currentElt = currentElt->next;
//     }
//   }
// }

void RF24Wave::send(MyMessage &message){
  mSetCommand(message, C_SET);
  sendMyMessage(message, GATEWAY_ADDRESS);
}

void RF24Wave::sendMyMessage(MyMessage &message, uint8_t destID)
{
  bool send = false;
  message.sender = nodeID;
  // mSetCommand(message, C_SET);
  protocolFormat(message);
  Serial.println(F("[sendMyMessage] Data send :"));
  Serial.print(_fmtBuffer);
  //mesh.update();
  //send = mesh.write(_fmtBuffer, MY_MESSAGE_T, MY_GATEWAY_MAX_SEND_LENGTH, destID);
  while(!send){
    uint32_t currentTimer = millis();
    mesh.update();
    if(currentTimer - lastTimer > 2000){
      lastTimer = currentTimer;
      //mesh.update();
      send = mesh.write(_fmtBuffer, MY_MESSAGE_T, MY_GATEWAY_MAX_SEND_LENGTH, destID);
      if(!send){
        Serial.println(F("[sendMyMessage] Unable to send notification - Retry "));
      }
    }
  }
}

void RF24Wave::sendNotifications(mysensor_sensor tSensor, mysensor_data tValue,
  mysensor_payload tPayload, int16_t payload)
{
  notif_msg_t data;
  uint32_t currentTimer = millis();
  if(currentTimer - lastTimer > 5000){
    lastTimer = currentTimer;
    data.tSensor = tSensor;
    data.tValue = tValue;
    data.tPayload = tPayload;
    data.payload.iValue = payload;
    broadcastNotifications(data);
  }
}

void RF24Wave::sendNotifications(mysensor_sensor tSensor, mysensor_data tValue,
  mysensor_payload tPayload, int32_t payload)
{
  notif_msg_t data;
  uint32_t currentTimer = millis();
  if(currentTimer - lastTimer > 5000){
    lastTimer = currentTimer;
    data.tSensor = tSensor;
    data.tValue = tValue;
    data.tPayload = tPayload;
    data.payload.lValue = payload;
    broadcastNotifications(data);
  }
}

void RF24Wave::broadcastNotifications(notif_msg_t data)
{
  createBroadcastList();
  broadcast_list_t *currentElt = headBroadcastList;
  while(currentElt){
    data.destID = currentElt->nodeID;
    Serial.print(F("[broadcastNotifications] Send notification to "));
    Serial.println(data.destID);
    if(!mesh.write(&data, NOTIF_MSG_T, sizeof(data), data.destID)){
      Serial.println(F("[broadcastNotifications] Unable to send notification "));
    }
    currentElt = currentElt->next;
  }
}

void RF24Wave::printNotification(notif_msg_t data)
{
  Serial.println(F("[printNotification] Print notification :"));
  Serial.print(F("[printNotification] destID: "));
  Serial.println(data.destID);
  Serial.print(F("[printNotification] tSensor: "));
  Serial.println(data.tSensor);
  Serial.print(F("[printNotification] tValue: "));
  Serial.println(data.tValue);
  Serial.print(F("[printNotification] tPayload: "));
  Serial.println(data.tPayload);
  Serial.print(F("[broadcastNotifications] payload "));
  switch(data.tPayload){
    case P_STRING:
      Serial.println("STRING PAYLOAD");
      break;
    case P_BYTE:
      Serial.println(data.payload.bValue);
      break;
    case P_INT16:
      Serial.println(data.payload.iValue);
      break;
    case P_UINT16:
      Serial.println(data.payload.uiValue);
      break;
    case P_LONG32:
      Serial.println(data.payload.lValue);
      break;
    case P_ULONG32:
      Serial.println(data.payload.ulValue);
      break;
    case P_CUSTOM:
      Serial.println("CUSTOM PAYLOAD");
      break;
    case P_FLOAT32:
      Serial.println(data.payload.fValue, data.payload.fPrecision);
      break;
    default:
      Serial.println("UNKNOW PAYLOAD");
      break;
  }
}

void RF24Wave::createBroadcastList(){
  Serial.println(F("[createBroadcastList] BEGIN"));
  uint8_t i, j, currentGroup, currentDstID;
  for(i=0; i<MAX_GROUPS; i++){
    currentGroup = groupsID[i];
    if(currentGroup > 0){
      for(j=0; j<MAX_NODE_GROUPS; j++){
        currentDstID = listGroupsID[currentGroup-1][j];
        if((currentDstID > 0) && (currentDstID != nodeID)){
          addNodeToBroadcastList(currentDstID);
        }
      }
    }
  }
}

void RF24Wave::addNodeToBroadcastList(uint8_t NID){
  Serial.println(F("[addNodeToBroadcastList] BEGIN"));
  bool found = false;
  if(!headBroadcastList){
    Serial.println(F("[addNodeToBroadcastList] Head list NULL"));
    headBroadcastList = (broadcast_list_t*) calloc(1, sizeof(broadcast_list_t));
    headBroadcastList->nodeID = NID;
    headBroadcastList->next = NULL;
    Serial.print(F("[addNodeToBroadcastList] Node "));
    Serial.print(NID);
    Serial.println(F(" added !"));
  }
  else{
    broadcast_list_t *currentElt = headBroadcastList;
    while(currentElt->next && !found){
      if(currentElt->nodeID == NID){
        found = true;
      }
      currentElt = currentElt->next;
    }
    if(currentElt->nodeID == NID){
      found = true;
    }
    if(!found){
      currentElt->next = (broadcast_list_t*) calloc(1, sizeof(broadcast_list_t));
      currentElt->next->nodeID = NID;
      currentElt->next->next = NULL;
      lengthBroadcastList++;
      Serial.print(F("[addNodeToBroadcastList] Node "));
      Serial.print(NID);
      Serial.println(F(" added !"));
    }else{
      Serial.print(F("[addNodeToBroadcastList] Node "));
      Serial.print(NID);
      Serial.println(F(" already added !"));
    }
  }
  Serial.println(F("[addNodeToBroadcastList] END"));
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

void RF24Wave::sendSketchInfo(const char *name, const char *version)
{
  sendMyMessage(build(_msgTmp, nodeID, GATEWAY_ADDRESS, 255, C_PRESENTATION, S_ARDUINO_NODE, false), 0);
	if (name) {
		sendMyMessage(build(_msgTmp, nodeID, GATEWAY_ADDRESS, 255, C_INTERNAL, I_SKETCH_NAME, false).set(name), 0);
	}
	if (version) {
		sendMyMessage(build(_msgTmp, nodeID, GATEWAY_ADDRESS, 255, C_INTERNAL, I_SKETCH_VERSION, false).set(version), 0);
	}
}

void RF24Wave::present(const uint8_t childId, const uint8_t sensorType, const char *description)
{
  sendMyMessage(build(_msgTmp, nodeID, GATEWAY_ADDRESS, childId, C_PRESENTATION, sensorType, false).set(description), 0);
}

// void RF24Wave::printNotification(notif_msg_t data)
// {
//   MyMessage message;
//   if(protocolParse(message, data.myMessage)){
//     Serial.println(F("> Notification received !"));
//     Serial.print(F("Node Dest: "));
//     Serial.println(message.destination);
//     Serial.print(F("Type: "));
//     Serial.println(message.type);
//     // Serial.print(F("Value: "));
//     // Serial.println(data.value);
//     Serial.println();
//   }
// }

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

void RF24Wave::gatewayTransportInit()
{
	gatewayTransportSend(buildGw(_msgTmp, I_GATEWAY_READY).set(MSG_GW_STARTUP_COMPLETE));
	// Send presentation of locally attached sensors (and node if applicable)
	//presentNode();
}

MyMessage& RF24Wave::buildGw(MyMessage &msg, const uint8_t type)
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

void RF24Wave::gatewayTransportSend(MyMessage &message)
{
	Serial.print(protocolFormat(message));
}

#endif
