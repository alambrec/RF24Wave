#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <SPI.h>
#include <RF24Wave.h>
#include <MyMessage.h>

#include "node3.h"

uint8_t groups[] = {3, 5};

RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);
RF24Mesh mesh(radio, network);
RF24Wave wave(radio, network, mesh, NODE_ID, groups);
//
// RF24Network& network;
// RF24Mesh& mesh;
// RF24Wave& wave;

MyMessage msg(1, V_STATUS);
bool status = false;

void setup(){
  Serial.begin(115200);
  Serial.println(F("Boot in waiting, press any touch to launch boot sequence !"));
  while (!Serial.available()) {
  ; // wait for serial port to connect. Needed for native USB
  }
  Serial.println(F("Boot amorced !"));
  wave.begin();
  delay(3000);
  wave.sendSketchInfo("Node 3", "1.0");
  wave.present(1, S_BINARY, "Light 3");
}



void loop(){
  wave.listen();
  if(Serial.available() > 0){
    Serial.read();
    if(status){
      status = false;
    }else{
      status = true;
    }
    msg.set(status);
    wave.send(msg);
  }
}

void receive(const MyMessage &message){
  if (message.type == V_LIGHT) {
    bool lightState = message.getBool();
    if (lightState) {
      Serial.println(F("Change state to ON"));
      status = true;
    }else{
      Serial.println(F("Change state to OFF"));
      status = false;
    }
    msg.set(status);
    wave.send(msg);
  }
}
