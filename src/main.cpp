#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>

#include "main.h"
#include "RF24Wave.h"

#include <MyMessage.h>
#include <MyProtocol.h>


RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

RF24Wave wave(network, mesh, NODE_ID);


uint32_t displayTimer = 0;
bool initialized = false;
//MyMessage msgTemp(1, V_TEMP);

void setup(){
  Serial.begin(115200);
  Serial.println("Boot ok !");
  wave.begin();
  // Serial.print(protocolFormat(msgTemp));
  //
  // //wave.begin();
  // delay(6000);
  // uint8_t i = 0;
  // for(i=0; i<8; i++){
  //     sendSketchInfo(i, "Test_node", "1.0");
  //     present(i, 1, S_TEMP);
  //     send(msgTemp.set((uint8_t)random(30)), i);
  //     delay(1000);
  // }
}

void loop(){
  wave.listen();
}
