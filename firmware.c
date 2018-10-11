#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>
#include "base.h"

#define HEADER_LENGTH 16
#define SHA1_LENGTH 40
#define ADDR_LENGTH 6 

uint8_t mac[ADDR_LENGTH];
char macaddr[ADDR_LENGTH*2];

// global sequence number
uint8_t messageCount = 0;

// mode switches
int retransmitEnabled = 0;
int pollingEnabled = 0;
int beaconModeEnabled = 1;
int hashingEnabled = 1;
int beaconModeReached = 0;

// timeout intervals
int bufferInterval = 5;
int helloInterval = 10;
int routeInterval = 25;

// metric variables
float packetSuccessWeight = .4;
float RSSIWeight = .3;
float SNRWeight = .3;

// packet structures
struct Metadata {
    uint8_t rssi;
    uint8_t snr;
};
struct Packet {
    uint8_t ttl;
    uint8_t totalLength;
    uint8_t source[ADDR_LENGTH];
    uint8_t destination[ADDR_LENGTH];
    uint8_t sequence;
    uint8_t type;
    uint8_t data[240];
};

// table structures
uint8_t hashTable[256][SHA1_LENGTH];
uint8_t hashEntry = 0;

struct BufferEntry {
    uint8_t message[256]; 
    uint8_t length;
};
struct BufferEntry messageBuffer[8];
int bufferEntry = 0;

struct NeighborTableEntry{
    uint8_t address[ADDR_LENGTH];
    uint8_t lastReceived;
    uint8_t packet_success;
    uint8_t metric;
};
struct NeighborTableEntry neighborTable[255];
int neighborEntry = 0;

struct RoutingTableEntry{
    uint8_t destination[ADDR_LENGTH];
    uint8_t nextHop[ADDR_LENGTH];
    uint8_t distance;
    uint8_t lastReceived;
    uint8_t metric;
};
struct RoutingTableEntry routeTable[255];
int routeEntry = 0;

int isHashNew(uint8_t hash[SHA1_LENGTH]){

    int hashNew = 1;
    Serial.printf("hash is %x\n", hash);
    for( int i = 0 ; i <= hashEntry ; i++){
        if(strcmp(hash, hashTable[i]) == 0){
            hashNew = 0; 
            Serial.printf("Not new!\n");
        }
    }
    if(hashNew){
        // add to hash table
        Serial.printf("New message received");
        Serial.printf("\r\n");
        for( int i = 0 ; i < SHA1_LENGTH ; i++){
            hashTable[hashEntry][i] = hash[i];
        }
        hashEntry++;
    }
    return hashNew;
}

void pushToBuffer(uint8_t message[256], int length){

    if(bufferEntry > 7){
        bufferEntry = 0;
    }
    messageBuffer[bufferEntry].length = length;
    for( int i = 0 ; i < length ; i++){
        messageBuffer[bufferEntry].message[i] = message[i];    
    }
    bufferEntry++;
}

struct BufferEntry popFromBuffer(){

    bufferEntry--;
    struct BufferEntry pop;
    pop.length = messageBuffer[bufferEntry].length;
    uint8_t* popMessage = malloc(pop.length);
    for( int i = 0 ; i < pop.length ; i++){
        popMessage[i] = messageBuffer[bufferEntry].message[i];
        messageBuffer[bufferEntry].message[i] = 0;
    }
    memcpy(&pop.message, popMessage, pop.length);
    return pop; 
}

void printNeighborTable(){

    Serial.printf("\n");
    Serial.printf("Neighbor Table:\n");
    for( int i = 0 ; i <= neighborEntry ; i++){
        for(int j = 0 ; j < ADDR_LENGTH ; j++){
            Serial.printf("%02x", neighborTable[i].address[j]);
        }
        Serial.printf(" %3d ", neighborTable[i].metric);
        Serial.printf("\n");
    }
    Serial.printf("\n");
}

int checkNeighborTable(struct Packet packet){

    int neighborNew = 1;
    printNeighborTable();
    for( int i = 0 ; i <= neighborEntry ; i++){
        //had to use memcmp instead of strcmp?
        if(memcmp(packet.source, neighborTable[i].address, sizeof(packet.source)) == 0){
            neighborNew = 0; 
        }
    }
    return neighborNew;
}

uint8_t calculatePacketLoss(int entry, uint8_t sequence){

    uint8_t packet_loss;
    uint8_t sequence_diff = sequence - neighborTable[entry].lastReceived;
    if(sequence_diff == 0){
        // this is first packet received from neighbor
        // assume perfect packet success
        neighborTable[entry].packet_success = 0xFF; 
        packet_loss = 0x00; 
    }else if(sequence_diff == 1){
        // do not decrease packet success rate
        packet_loss = 0x00; 
    }else if(sequence_diff > 1 && sequence_diff < 16){
        // decrease packet success rate by difference
        packet_loss = 0x10 * sequence_diff; 
    }else if(sequence_diff > 16){
        // no packet received recently
        // assume complete pakcet loss
        packet_loss = 0xFF; 
    }
    return packet_loss;
}

uint8_t calculateMetric(int entry, uint8_t sequence, struct Metadata metadata){

    float weightedPacketSuccess =  ((float) neighborTable[entry].packet_success)*packetSuccessWeight;
    float weightedRSSI =  ((float) metadata.rssi)*RSSIWeight;
    float weightedSNR =  ((float) metadata.snr)*SNRWeight;
    uint8_t metric = weightedPacketSuccess+weightedRSSI+weightedSNR;
    /*
    Serial.printf("weighted packet success: %3f\n", weightedPacketSuccess);
    Serial.printf("weighted RSSI: %3f\n", weightedRSSI);
    Serial.printf("weighted SNR: %3f\n", weightedSNR);
    Serial.printf("metric calculated: %3d\n", metric);
    */
    return metric;
}

void addNeighbor(struct Packet packet, struct Metadata metadata){

    Serial.printf("New neighbor found: ");
    for( int i = 0 ; i < ADDR_LENGTH; i++){
        neighborTable[neighborEntry].address[i] = packet.source[i];
        Serial.printf("%02x", neighborTable[neighborEntry].address[i]);
    }
    Serial.printf("\n");
    neighborTable[neighborEntry].lastReceived = packet.sequence;
    uint8_t packet_loss = calculatePacketLoss(neighborEntry, packet.sequence);
    neighborTable[neighborEntry].packet_success = neighborTable[neighborEntry].packet_success - packet_loss;
    uint8_t metric = calculateMetric(neighborEntry, packet.sequence, metadata); 
    neighborTable[neighborEntry].metric = metric;
    neighborEntry++;
}

void printRoutingTable(){

    Serial.printf("\n");
    Serial.printf("Routing Table:\n");
    for( int i = 0 ; i < routeEntry ; i++){
        Serial.printf("%d hops from ", routeTable[i].distance);
        for(int j = 0 ; j < ADDR_LENGTH ; j++){
            Serial.printf("%02x", routeTable[i].destination[j]);
        }
        Serial.printf(" via ");
        for(int j = 0 ; j < ADDR_LENGTH ; j++){
            Serial.printf("%02x", routeTable[i].nextHop[j]);
        }
        Serial.printf(" metric %3d ", routeTable[i].metric);
        Serial.printf("\n");
    }
    Serial.printf("\n\n");
}

void addRoute(struct RoutingTableEntry route){

    memcpy(&routeTable[routeEntry], &route, sizeof(routeTable[routeEntry]));
    Serial.printf("New route found! ");
    for( int i = 0 ; i < ADDR_LENGTH; i++){
        Serial.printf("%02x", routeTable[routeEntry].destination[i]);
    }
    Serial.printf("\n");
    routeEntry++;
}

int checkRoutingTable(struct RoutingTableEntry route){

    int routeOpt = 1;
    //printRouteTable();
    for( int i = 0 ; i <= routeEntry ; i++){
        
        if(memcmp(route.destination, routeTable[i].destination, sizeof(route.destination)) == 0){
            if(memcmp(route.nextHop, routeTable[i].nextHop, sizeof(route.destination)) == 0){
                routeOpt = 0; 
                // already have this exact route, update metric
            }else{
                routeOpt = 2;
                // already have this destination, but via a different neighbor
                // not sure how to handle this, maybe keep better metric?
            }
        }else{
            // this is a new route 
        }
    }
    return routeOpt;
}

void parseRoutingPacket(struct Packet packet){
    int numberOfRoutes = (packet.totalLength - HEADER_LENGTH) / ADDR_LENGTH;
    Serial.printf("routes in packet: %d\n", numberOfRoutes);

    struct RoutingTableEntry route[40]; 
    for( int i = 0 ; i < numberOfRoutes ; i++){
        memcpy(&route[i].destination, packet.data + (ADDR_LENGTH+1)*i, ADDR_LENGTH);
        memcpy(&route[i].nextHop, packet.source, ADDR_LENGTH);
        route[i].distance = 2;
        route[i].metric = packet.data[(ADDR_LENGTH+1)*i];
        int opt = checkRoutingTable(route[i]);
        if(opt == 0){
            Serial.printf("existing route\n");
        }else if(opt == 1){
            addRoute(route[i]);  
        }else if(opt == 2){
            Serial.printf("existing route from another neighbor\n");
        }
    }
}

void printPacketInfo(struct Packet packet, struct Metadata metadata){

    Serial.printf("\n");
    Serial.printf("Packet Received: \n");
    Serial.printf("RSSI: %x\n", metadata.rssi);
    Serial.printf("SNR: %x\n", metadata.snr);
    Serial.printf("ttl: %d\n", packet.ttl);
    Serial.printf("length: %d\n", packet.totalLength);
    Serial.printf("source: ");
    for(int i = 0 ; i < ADDR_LENGTH ; i++){
        Serial.printf("%x", packet.source[i]);
    }
    Serial.printf("\n");
    Serial.printf("destination: ");
    for(int i = 0 ; i < ADDR_LENGTH ; i++){
        Serial.printf("%x", packet.destination[i]);
    }
    Serial.printf("\n");
    Serial.printf("sequence: %02x\n", packet.sequence);
    Serial.printf("type: %c\n", packet.type);
    Serial.printf("data: ");
    for(int i = 0 ; i < packet.totalLength-HEADER_LENGTH ; i++){
        Serial.printf("%02x", packet.data[i]);
    }
    Serial.printf("\n");
}
    
int packet_received(char* data, size_t len) {

    data[len] = '\0';

    // convert ASCII data to pure bytes
    uint8_t* byteData = ( uint8_t* ) data;
    
    // randomly generate RSSI and SNR values 
    // see https://github.com/sudomesh/disaster-radio-simulator/issues/3
    uint8_t packet_rssi = rand() % (256 - 128) + 128;
    uint8_t packet_snr = rand() % (256 - 128) + 128;

    struct Metadata metadata = {
        packet_rssi,
        packet_snr
    };
    struct Packet packet = {
        byteData[0],
        byteData[1], 
        byteData[2], byteData[3], byteData[4], byteData[5], byteData[6], byteData[7],
        byteData[8], byteData[9], byteData[10], byteData[11], byteData[12], byteData[13],
        byteData[14],
        byteData[15],
    };
    memcpy(&packet.data, byteData + HEADER_LENGTH, packet.totalLength-HEADER_LENGTH);

    printPacketInfo(packet, metadata);
    
    switch(packet.type){
        case 'h' :
            // hello packet;
            if(checkNeighborTable(packet)){
                addNeighbor(packet, metadata);  
            }else{
                // update metric
            }
            break;
        case 'r':
            // routing packet;
            parseRoutingPacket(packet);
            printRoutingTable();
            break;
        case 'c' :
            Serial.printf("this is a chat message\n");
            break;
        case 'm' :
            Serial.printf("this is a map message\n");
            break;
        default :
            Serial.printf("message type not found\n");
    }
    pushToBuffer(data, len);

    return 0;
}

void sendMessage(uint8_t* outgoing, int outgoingLength) {

    //if(!loraInitialized){
    //    return;
    //}
    if(hashingEnabled){
        // do not send message if already transmitted once
        uint8_t hash[SHA1_LENGTH];
        SHA1(outgoing, outgoingLength, hash);
        if(!isHashNew(hash)){
            return;
        }
    }
    Serial.printf("Transmitting packet: ");
    for( int i = 0 ; i < outgoingLength ; i++){
        Serial.printf("%x", outgoing[i]);
    }
    Serial.printf("\r\n");
    /*
    LoRa.beginPacket();
    for( int i = 0 ; i < outgoingLength ; i++){
        LoRa.write(outgoing[i]);
        Serial.printf("%c", outgoing[i]);
    }
    Serial.printf("\r\n");
    LoRa.endPacket();
    LoRa.receive();
    */
    send_packet(outgoing, outgoingLength);
}

long lastCheckTime = 0;
void checkBuffer(){

    if (time(NULL) - lastCheckTime > bufferInterval) {
        if (bufferEntry > 0){
            // Uncomment if you want race condition to determine a single beacon node
            if(!beaconModeReached){
                beaconModeEnabled = 0;
            }

            struct BufferEntry transmit = popFromBuffer();

            if(retransmitEnabled){
                sendMessage(transmit.message, transmit.length);
            }
        }else{
            Serial.printf("Buffer is empty\n");
        }
        lastCheckTime = time(NULL);
    }
}

struct Packet buildPacket( uint8_t ttl, uint8_t dest[6], uint8_t type, uint8_t* data, uint8_t data_length){

    uint8_t packetLength = HEADER_LENGTH + data_length;
    struct Packet packet = {
        ttl,
        packetLength,
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
        dest[0], dest[1], dest[2], dest[3], dest[4], dest[5],
        messageCount,
        type 
    };

    memcpy(&packet.data, data, packet.totalLength);
    return packet;
}

long lastHelloTime = 0;
void transmitHello(){

    if (time(NULL) - lastHelloTime > helloInterval) {
        beaconModeReached = 1; 
        char buf[256];
        char message[10] = "Hola from\0";
        sprintf(buf, "%s %s", message, macaddr);
        int messageLength = 22;
        uint8_t* byteMessage = malloc(messageLength);
        byteMessage = ( uint8_t* ) buf;
        //TODO: add randomness to message to avoid hashisng issues
        uint8_t destination[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
        struct Packet helloMessage = buildPacket(32, destination, 'h', byteMessage, messageLength); 
        uint8_t* sending = malloc(helloMessage.totalLength);
        sending = &helloMessage;
        send_packet(sending, helloMessage.totalLength);
        messageCount++;
        lastHelloTime = time(NULL);
        /* print in debugging mode
        Serial.printf("Sending beacon: ");
        for(int i = 0 ; i < helloMessage.totalLength ; i++){
            Serial.printf("%02x", sending[i]);
        }
        Serial.printf("\r\n");
        */
    }
}


long lastRouteTime = 0;
void transmitRoutes(){

    if (time(NULL) - lastRouteTime > routeInterval) {
        uint8_t data[240];
        int dataLength = 0;
        // append routes from neighbor table
        for( int i = 0 ; i < neighborEntry ; i++){
            for( int j = 0 ; j < ADDR_LENGTH ; j++){
                data[dataLength] = neighborTable[i].address[j];
                dataLength++;
            }
            data[dataLength] = neighborTable[i].metric;
            dataLength++;
        }
        uint8_t destination[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
        struct Packet routeMessage = buildPacket(32, destination, 'r', data, dataLength); 
        uint8_t* sending = malloc(routeMessage.totalLength);
        sending = &routeMessage;
        send_packet(sending, routeMessage.totalLength);
        messageCount++;
        lastRouteTime = time(NULL);
        /*
        Serial.printf("Sending routes: ");
        for(int i = 0 ; i < routeMessage.totalLength ; i++){
            Serial.printf("%02x", sending[i]);
        }
        Serial.printf("\r\n");
        */
    }
}

void wifiSetup(){

    //WiFi.macAddress(mac);
    // generate random mac address
    srand(time(NULL) + getpid());
    for (int i = 0; i < ADDR_LENGTH ; i++){
        mac[i] = rand()%256;
    }
    // MAC address comes in backwards on ESP8266
    // reverse mac array
    uint8_t tmp;
    int start = 0;
    int end = ADDR_LENGTH-1;
    while (start < end) 
    { 
        tmp = mac[start];    
        mac[start] = mac[end]; 
        mac[end] = tmp; 
        start++; 
        end--; 
    }    
    sprintf(macaddr, "%02x%02x%02x%02x%02x%02x\0", mac[0], mac[1], mac[2], mac[3], mac[4], mac [5]);
    Serial.printf("%s\n", macaddr);
    // dummy neighbor for debugging purposes
    for( int i = 0 ; i < ADDR_LENGTH ; i++){
        neighborTable[0].address[i] = 0xa1;
    }
    neighborTable[0].metric = 10; 
    neighborTable[0].packet_success = 10; 
    neighborTable[0].lastReceived = 10; 
    neighborEntry++;
    //strcat(ssid, macaddr);
    //WiFi.hostname(hostName);
    //WiFi.mode(WIFI_AP);
    //WiFi.softAPConfig(local_IP, gateway, netmask);
    //WiFi.softAP(ssid);
}

int setup() {

    Serial.printf("initialized\n");

    wifiSetup();

    // random wait at boot
    int wait = rand()%15;
    Serial.printf("waiting %d s\n", wait);
    nsleep(wait, 0);

    return 0;
}

int loop() {

    int packetSize; 

    //if(transmitting() == 1){
        // do stuff while LoRa packet is being sent
        //Serial.print("transmitting a packet...\r\n");
        //return;
    //}else{
        // do stuff when LoRa packet is NOT being sent
        checkBuffer(); 

        if (beaconModeEnabled){
            transmitHello();
        }
        if (neighborEntry > 0){
            transmitRoutes();
        }
    //}
    nsleep(1, 0);
}
