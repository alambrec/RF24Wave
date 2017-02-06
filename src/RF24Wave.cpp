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
RF24Wave::RF24Wave(RF24& _radio, RF24Network& _network, RF24Mesh& _mesh, uint8_t NodeID, uint8_t *groups):
radio(_radio), network(_network), mesh(_mesh)
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
  memset(&info_payload, 0, sizeof(info_node_t));
  memset(&update_payload, 0, sizeof(update_msg_t));
  memset(&list_payload, 0, sizeof(send_list_t));

};



/***************************** Common functions *****************************/

void RF24Wave::begin()
{
  mesh.setNodeID(nodeID);
  nodeID = mesh.getNodeID();
  P_DEBUG("Connecting to the mesh...");
  mesh.begin(110, RF24_250KBPS);
  //mesh.begin();
#if defined(AMPLIFICATOR)
  radio.setPALevel(RF24_PA_LOW);
#endif
  lastTimer = millis();
  resetListGroup();
  P_DEBUG("Connecting to the wave...");
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
      case MY_MESSAGE_T:
        P_DEBUG("[listen] MY_MESSAGE_T")
        network.read(header, _fmtBuffer, MY_GATEWAY_MAX_SEND_LENGTH);
        Serial.print(_fmtBuffer);
#if !defined(WAVE_MASTER)
        if(protocolParse(_msgTmp, _fmtBuffer)){
          P_DEBUG("[MY_MESSAGE_T] parse ok !")
          receive(_msgTmp);
        }
#endif
        break;
#if defined(WAVE_MASTER)
      case CONNECT_MSG_T:
        memset(&info_payload, 0, sizeof(info_node_t));
        network.read(header, &info_payload, sizeof(info_node_t));
        F_DEBUG(printAssociation(info_payload))
        if(checkAssociations(&info_payload)){
          P_DEBUG("[listen] broadcastAssociations")
          broadcastAssociations(info_payload);
          P_DEBUG("[listen] addListAssociations")
          addListAssociations(info_payload);
          F_DEBUG(printAssociations())
        }
        break;
      case SYNCHRONIZE_MSG_T:
        memset(&info_payload, 0, sizeof(info_node_t));
        P_DEBUG("[listen] SYNCHRONIZE_MSG_T")
        network.read(header, &info_payload, sizeof(info_node_t));
        P_DEBUG("[listen] sendSynchronizedList")
        sendSynchronizedList(info_payload);
        break;
#else
      case UPDATE_MSG_T:
        memset(&update_payload, 0, sizeof(update_msg_t));
        network.read(header, &update_payload, sizeof(update_msg_t));
        printUpdate(update_payload);
        addAssociation(update_payload.nodeID, update_payload.groupID);
        printAssociations();
        break;
#endif
      default:
        break;
    }
  }

#if defined(WAVE_SERIAL_RECEIVE)
  if (gatewayTransportAvailable()){
    //memset(_fmtBuffer, 0, sizeof(MY_GATEWAY_MAX_SEND_LENGTH));
    mesh.update();
    mesh.write(_fmtBuffer, MY_MESSAGE_T, MY_GATEWAY_MAX_SEND_LENGTH, 1);
    if (_msgTmp.destination != GATEWAY_ADDRESS) {
      transmitMyMessage(_msgTmp, _msgTmp.destination);
    }
  }
#endif
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
  uint8_t i;
  memset(&info_payload, 0, sizeof(info_node_t));
  info_payload.nodeID = nodeID;
  for(i=0; i<MAX_GROUPS; i++){
    info_payload.groupsID[i] = groupsID[i];
  }
  printAssociation(info_payload);
  mesh.update();
  if(!mesh.write(&info_payload, CONNECT_MSG_T, sizeof(info_node_t))){
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
        memset(&info_payload, 0, sizeof(info_node_t));
        network.read(header, &info_payload, sizeof(info_node_t));
        P_DEBUG("[confirmAssociations] Received payload")
        F_DEBUG(printAssociation(info_payload))
        if(info_payload.nodeID != nodeID){
          Serial.println(F("[confirmAssociations] ERROR nodeID"));
          available = false;
        }
        for(i=0; i<MAX_GROUPS; i++){
          if(groupsID[i] != info_payload.groupsID[i]){
            Serial.print(F("[confirmAssociations] ERROR groupID"));
            Serial.println(groupsID[i]);
            available = false;
          };
        }
        //Serial.println(F("[confirmAssociations] ADD LIST BEGIN"));
        addListAssociations(info_payload);
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
    Serial.println(F("[confirmSynchronize] Available packet !"));
    RF24NetworkHeader header;
    network.peek(header);
    if(header.type == ACK_SYNCHRONIZE_MSG_T){
        Serial.println(F("[confirmAssociations] INFO ACK_INIT_MSG_T BEGIN"));
        memset(&list_payload, 0, sizeof(send_list_t));
        network.read(header, &list_payload, sizeof(send_list_t));
        if(list_payload.nodeID != nodeID){
          Serial.println(F("[confirmSynchronize] ERROR nodeID"));
          return;
        }
        receiveSynchronizedList(list_payload);
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
  uint8_t i;
  memset(&info_payload, 0, sizeof(info_node_t));
  info_payload.nodeID = nodeID;
  for(i=0; i<MAX_GROUPS; i++){
    info_payload.groupsID[i] = groupsID[i];
  }
  mesh.update();
  Serial.println(F("[requestSynchronize] Send request"));
  if(!mesh.write(&info_payload, SYNCHRONIZE_MSG_T, sizeof info_payload, 0)){
    Serial.println(F("[requestSynchronize] Send failed"));
    // If a write fails, check connectivity to the mesh network
    if(!mesh.checkConnection()){
      //refresh the network address
      Serial.println(F("[requestSynchronize] ERROR: Renewing Address"));
      mesh.renewAddress();
    }
  }
  Serial.println(F("[requestSynchronize] END"));
}

void RF24Wave::send(MyMessage &message){
  mSetCommand(message, C_SET);
  sendMyMessage(message, GATEWAY_ADDRESS);
}

void RF24Wave::broadcastNotifications(MyMessage &message)
{
  createBroadcastList();
  broadcast_list_t *currentElt = headBroadcastList;
  while(currentElt){
    Serial.print(F("[broadcastNotifications] Send notification to "));
    Serial.println(currentElt->nodeID);
    sendMyMessage(message, currentElt->nodeID);
    currentElt = currentElt->next;
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

void RF24Wave::sendMyMessage(MyMessage &message, uint8_t destID)
{
  bool send = false;
  uint8_t retry;
  message.sender = nodeID;
  // mSetCommand(message, C_SET);
  protocolFormat(message);
  P_DEBUG("[sendMyMessage] Data send :");
#if !defined(WAVE_MASTER) && defined(WAVE_DEBUG)
  Serial.print(_fmtBuffer);
#endif
  //mesh.update();
  //send = mesh.write(_fmtBuffer, MY_MESSAGE_T, MY_GATEWAY_MAX_SEND_LENGTH, destID);
  retry = 0;
  while(!send && (retry < NB_RETRY_SEND)){
    uint32_t currentTimer = millis();
    mesh.update();
    if(currentTimer - lastTimer > 2000){
      lastTimer = currentTimer;
      mesh.update();
      send = mesh.write(_fmtBuffer, MY_MESSAGE_T, MY_GATEWAY_MAX_SEND_LENGTH, destID);
      if(!send){
        Serial.println(F("[sendMyMessage] Unable to send notification - Retry "));
      }
      retry++;
    }
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
      Serial.println(F("[checkAssociations] ERROR Unable group!"));
      available = false;
      data->groupsID[i] = 0;
    }
  }
  P_DEBUG("[checkAssociations] DEBUG Data before send")
  F_DEBUG(printAssociation(*data))
  mesh.update();
  if(!mesh.write(data, ACK_CONNECT_MSG_T, sizeof(info_node_t), data->nodeID)){
    Serial.println(F("[checkAssociations] ERROR unable to send response!"));
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
  //send_list_t payload;
  memset(&list_payload, 0, sizeof(send_list_t));
  uint8_t i, j, currentGroup;
  mesh.update();
  /* We initialize array with zeros */
  for(i=0; i<MAX_GROUPS; i++){
    for(j=0; j<MAX_NODE_GROUPS; j++){
      list_payload.listGroupsID[i][j] = 0;
    }
  }
  list_payload.nodeID = msg.nodeID;
  for(i=0; i<MAX_GROUPS; i++){
    currentGroup = msg.groupsID[i];
    /* We check if node is realy present in group */
    if(isPresent(msg.nodeID, currentGroup)){
      for(j=0; j<MAX_NODE_GROUPS; j++){
        /* We copy all nodes associated with this group */
        list_payload.listGroupsID[currentGroup-1][j] = listGroupsID[currentGroup-1][j];
      }
    }
  }
  if(!mesh.write(&list_payload, ACK_SYNCHRONIZE_MSG_T, sizeof(send_list_t), list_payload.nodeID)){
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
      memset(&update_payload, 0, sizeof(update_msg_t));
      update_payload.nodeID = NID;
      update_payload.groupID = GID;
      if(!mesh.write(&update_payload, UPDATE_MSG_T, sizeof(update_msg_t), currentNID)){
        successful = false;
        Serial.println(F("[sendUpdateGroup] ERR: Unable to send Update !"));
        Serial.print(F("[sendUpdateGroup] NID: "));
        Serial.print(update_payload.nodeID);
        Serial.print(F(" GID: "));
        Serial.print(update_payload.groupID);
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

bool RF24Wave::gatewayTransportAvailable(void)
{
  char inChar;
	while (Serial.available()) {
		// get the new byte:
		inChar = (char)Serial.read();
		// if the incoming character is a newline, set a flag
		// so the main loop can do something about it:
		if (_serialInputPos < MY_GATEWAY_MAX_RECEIVE_LENGTH - 1) {
			if (inChar == '\n') {
				_fmtBuffer[_serialInputPos] = 0;
        _serialInputPos = 0;
				return protocolParse(_msgTmp, _fmtBuffer);
			} else {
				// add it to the inputString:
				_fmtBuffer[_serialInputPos] = inChar;
				_serialInputPos++;
			}
		} else {
			// Incoming message too long. Throw away
			_serialInputPos = 0;
		}
	}
	return false;
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

MyMessage& RF24Wave::gatewayTransportReceive()
{
	// Return the last parsed message
	return _msgTmp;
}

void RF24Wave::transmitMyMessage(MyMessage &message, uint8_t destID)
{
  message.sender = nodeID;
  protocolFormat(message);
  mesh.update();
  if(!mesh.write(_fmtBuffer, MY_MESSAGE_T, MY_GATEWAY_MAX_SEND_LENGTH, destID)){
    Serial.println(F("[sendMyMessage] Unable to send notification - Retry "));
  }
}

#endif
