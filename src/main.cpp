#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <SPI.h>

#include "main.h"
#include "RF24Wave.h"

uint8_t groups[] = {1, 3, 4, 5};

RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

RF24Wave wave(network, mesh, NODE_ID, groups);

uint32_t displayTimer = 0;
bool initialized = false;

void setup(){
  Serial.begin(115200);
  wave.begin();
}

void loop(){

}
