#include <stdio.h>
#include <stdlib.h>

//LoRaLayer2
#include <Layer1_LoRa.h>
#include <LoRaLayer2.h>
#define LL2_DEBUG

// For Heltec ESP32 LoRa V2 - Update for your board / region
#define LORA_CS 18
#define LORA_RST 14
#define LORA_IRQ 26
#define LORA_FREQ 915E6
#define LED 25
#define TX_POWER 20

char MAC[9] = "c0d3f00d";
uint8_t LOCAL_ADDRESS[ADDR_LENGTH] = {0xc0, 0xd3, 0xf0, 0x0d};
// GATEWAY is the receiver 
uint8_t GATEWAY[ADDR_LENGTH] = {0xc0, 0xd3, 0xf0, 0x0c};

Layer1Class *Layer1;
LL2Class *LL2;

int counter = 0;

void setup() {
  SPI.begin(5, 19, 27, 18);
  Serial.begin(9600);
  while (!Serial);

  Serial.println("* Initializing LoRa...");
  Serial.println("LoRa Sender");

  Layer1 = new Layer1Class();
  Layer1->setPins(LORA_CS, LORA_RST, LORA_IRQ);
  Layer1->setTxPower(TX_POWER);
  Layer1->setLoRaFrequency(LORA_FREQ);
  if (Layer1->init())
  {
    Serial.println(" --> LoRa initialized");
    LL2 = new LL2Class(Layer1); // initialize Layer2
    LL2->setLocalAddress(MAC); // this should either be randomized or set using the wifi mac address
    LL2->setInterval(10000); // set to zero to disable routing packets
  }
  else
  {
    Serial.println(" --> Failed to initialize LoRa");
  }
}

void loop() {
  LL2->daemon();
  int msglen = 0;
  int packetsize = 0;

  struct Datagram datagram; 
  msglen = sprintf((char*)datagram.message, "%s,%i", "hello", counter); 
  memcpy(datagram.destination, GATEWAY, ADDR_LENGTH);
  datagram.type = 's'; // can be anything, but 's' for 'sensor'

  packetsize = msglen + HEADER_LENGTH;

  // Send packet
  LL2->writeData(datagram, packetsize);
  counter++;

  Serial.println((char*)datagram.message);
  delay(1000);
}