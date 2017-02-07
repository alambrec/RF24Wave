#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <SPI.h>
#include <RF24Wave.h>
#include <MyMessage.h>

#include "node2.h"

uint8_t groups[] = {2};

RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);
RF24Mesh mesh(radio, network);
RF24Wave wave(radio, network, mesh, NODE_ID, groups);

bool status = false;

void setup(){
  Serial.begin(115200);
  delay(5000);
  Serial.println(F("Boot amorced !"));
  wave.begin();
  delay(3000);
  wave.sendSketchInfo("Repeater", "1.0");
}



void loop(){
  wave.listen();
}
