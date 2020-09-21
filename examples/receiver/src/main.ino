#include <stdio.h>
#include <stdlib.h>

//LoRaLayer2
#include <Layer1_LoRa.h>
#include <LoRaLayer2.h>
#define LL2_DEBUG

// In most cases it is recommened that you leave the LORA and SPI pins
// set to their deafult values as defined in pins_arduino.h
// found in https://github.com/espressif/arduino-esp32/blob/master/variants
#ifdef HELTEC_V2
// For Heltec ESP32 LoRa V2 - Update for your board / region
#define LORA_CS 18
#define LORA_RST 14
#define LORA_IRQ 26
#endif
#define LORA_FREQ 915E6
#define LED 25
#define TX_POWER 20

char MAC[9] = "c0d3cafe";
uint8_t LOCAL_ADDRESS[ADDR_LENGTH] = {0xc0, 0xd3, 0xca, 0xfe};
// GATEWAY is the receiver 
uint8_t SENDER[ADDR_LENGTH] = {0xc0, 0xd3, 0xf0, 0x0d};

Layer1Class *Layer1;
LL2Class *LL2;

int counter = 0;

void setup() {
#ifdef HELTEC_V2
  SPI.begin(5, 19, 27, 18); // not needed for ttgo-lora32-v1
#endif
  Serial.begin(9600);
  while (!Serial);

  Serial.println("* Initializing LoRa...");
  Serial.println("LoRa Receiver");

  Layer1 = new Layer1Class();
  Layer1->setPins(LORA_CS, LORA_RST, LORA_IRQ);
  Layer1->setTxPower(TX_POWER);
  Layer1->setLoRaFrequency(LORA_FREQ);
  if (Layer1->init())
  {
    Serial.println(" --> Layer1 initialized");
    LL2 = new LL2Class(Layer1); // initialize Layer2
    LL2->setLocalAddress(MAC); // this should either be randomized or set using the wifi mac address
    LL2->setInterval(10000); // set to zero to disable routing packets
    if (LL2->init() == 0){
      Serial.println(" --> LoRaLayer2 initialized");
    }
    else{
      Serial.println(" --> Failed to initialize LoRaLayer2");
    }
  }
  else
  {
    Serial.println(" --> Failed to initialize LoRa");
  }
}

void loop() {

  char routes[256];
  char neighbors[256];

  LL2->daemon();

  struct Packet packet = LL2->readData();
  if(packet.totalLength > HEADER_LENGTH)
  {
    Serial.println(((char *)packet.datagram.message));

    LL2->getRoutingTable(routes);
    Serial.printf("%s", routes);

    LL2->getNeighborTable(neighbors);
    Serial.println(neighbors);
  }
}
