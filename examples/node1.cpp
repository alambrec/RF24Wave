#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <SPI.h>
#include <RF24Wave.h>
#include <MyMessage.h>

#include "node1.h"

uint8_t groups[] = {1, 3};

RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);
RF24Mesh mesh(radio, network);
RF24Wave wave(radio, network, mesh, NODE_ID, groups);

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
  wave.sendSketchInfo("Node 1", "1.0");
  wave.present(1, S_BINARY, "Relay 1");
}



void loop(){
  wave.listen();
  if(Serial.available() > 0){
    char input = (char) Serial.read();
    switch(input){
      case 'a':
        Serial.println(F("Change status !"));
        if(status){
          Serial.println(F("Set to OFF!"));
          status = false;
        }else{
          Serial.println(F("Set to ON!"));
          status = true;
        }
        msg.set(status);
        wave.send(msg);
        break;
      case 'i':
        Serial.println(F("Set groupe to ON!"));
        wave.broadcastNotifications(msg.set(true));
        break;
      case 'o':
        Serial.println(F("Set groupe to OFF!"));
        wave.broadcastNotifications(msg.set(false));
        break;
      default:
        break;
    }
  }
}
