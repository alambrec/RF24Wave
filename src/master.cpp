#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <SPI.h>

#include "RF24Wave.h"
#include "master.h"

// Configure the chosen CE, CSN pins
RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);
RF24Mesh mesh(radio,network);
RF24Wave wave(radio, network, mesh);

void setup(){
  Serial.begin(115200);
  wave.begin();
}

void loop(){
  wave.listen();
  F_DEBUG(wave.printNetwork())
}
