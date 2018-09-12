#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "base.h"

#define HEADER_LEN 15

int retransmitEnabled = 1;
int pollingEnabled = 0;
int beaconModeEnabled = 1;
int hashingEnabled = 1;

uint8_t hashTable[256][40];
uint8_t hashEntry = 0;

struct Buffer {
    uint8_t message[256]; 
    uint8_t length;
};

struct Buffer messageBuffer[8];

int bufferEntry = 0;

unsigned char mac[6];
unsigned char macaddr[12];

struct Packet {
    uint8_t ttl;
    uint8_t source[6];
    uint8_t destination[6];
    uint8_t sequence;
    uint8_t type;
    uint8_t data[240];
};
    

int packet_received(char* data, size_t len) {

    size_t send_len;
    char foo[256];

    data[len] = '\0';
    Serial.printf("received %d bytes: %s\n", len, data);

    addToBuffer(data, len);

    send_len = snprintf(foo, 256, "Got a packet of %d bytes", (int) len);
    //send_packet(foo, send_len);
}

void addToBuffer(uint8_t message[256], int length){

    if(bufferEntry > 7){
        bufferEntry = 0;
    }
    Serial.printf("adding message to buffer");
    Serial.printf("\r\n");
    messageBuffer[bufferEntry].length = length;
    for( int i = 0 ; i < length ; i++){
        messageBuffer[bufferEntry].message[i] = message[i];    
    }
    bufferEntry++;
}

void wifiSetup(){

    //WiFi.macAddress(mac);
    // generate random mac address
    srand(time(NULL) + getpid());
    for (int i=0; i<6; i++){
        mac[i] = rand()%256;
    }

    sprintf(macaddr, "%02x%02x%02x%02x%02x%02x", mac[5], mac[4], mac[3], mac[2], mac[1], mac [0]);

    Serial.printf("%s\n", macaddr);

    //strcat(ssid, macaddr);
    //WiFi.hostname(hostName);
    //WiFi.mode(WIFI_AP);
    //WiFi.softAPConfig(local_IP, gateway, netmask);
    //WiFi.softAP(ssid);
}


int setup() {

  Serial.printf("initialized\n");

  wifiSetup();

  return 0;
}


int bufferInterval = 2; // check buffer every 10 secs
int sendTimeout = 5; // should be set based on LoRa air time calculator
long lastBufferCheck = 0; 
long lastSendTime = 0; // time of last packet send

uint8_t beaconCount = 0;

int loop() {

    int packetSize;

    if ( time(NULL) - lastBufferCheck >= bufferInterval) {
        
        Serial.printf("Timestamp: %d\n", time(NULL));

        Serial.printf("Checking outbound buffer");
        Serial.printf("\r\n");
        if (bufferEntry > 0){
            Serial.printf("Removing from packet buffer");
            Serial.printf("\r\n");

            int transmitLength = messageBuffer[bufferEntry-1].length;
            uint8_t *transmit = malloc(transmitLength);

            Serial.printf("Transmitting packet: ");
            for( int i = 0 ; i < transmitLength ; i++){
                transmit[i] = messageBuffer[bufferEntry-1].message[i];
                Serial.printf("%x", transmit[i]);
                messageBuffer[bufferEntry-1].message[i] = 0;
            }
            Serial.printf("\r\n");
            bufferEntry--;
            send_packet(transmit, transmitLength);
        }else{
            Serial.printf("Buffer is empty");
            Serial.printf("\r\n");
        }
        
        if (beaconModeEnabled){

            int messageLength = 4;
            int packetLength = HEADER_LEN + messageLength;
            struct Packet testMessage = {
                32,
                mac[5], mac[4], mac[3], mac[2], mac[1], mac[0],
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                beaconCount,
                'b',
                'H', 'o', 'l', 'a'
            };

            uint8_t *sending = malloc(packetLength);
            sending = &testMessage;

            Serial.printf("Sending beacon: ");
            for(int i = 0 ; i < packetLength ; i++){
                Serial.printf("%x", sending[i]);
            }
            Serial.printf("\r\n");
            send_packet(sending, packetLength);
            beaconCount++;
        }
        
        lastBufferCheck = time(NULL);
    }
    sleep(1);
}
