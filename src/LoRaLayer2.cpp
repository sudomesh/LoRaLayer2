#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <Layer1.h>
#include <LoRaLayer2.h>


LL2Class::LL2Class() :

    _messageCount(),
    _packetSuccessWeight(.8),
    _randomMetricWeight(.2),
    _outBuffer(),
    _outBufferEntry(0),
    _inBuffer(),
    _inBufferEntry(0),
    _neighborTable(),
    _neighborEntry(0),
    _routeTable(),
    _routeEntry(0)
{
}

uint8_t LL2Class::messageCount(){
    return _messageCount;
}

int LL2Class::sendPacket(struct Packet packet) {

    uint8_t* sending = (uint8_t*) malloc(sizeof(packet));
    memcpy(sending, &packet, sizeof(packet));
    /*
    int send = 1;
    if(hashingEnabled){
        // do not send message if already transmitted once
        //uint8_t hash[SHA1_LENGTH];
        //SHA1(sending, packet.totalLength, hash);
        //if(isHashNew(hash)){
        //  send = 0;
        //}
    }
    */
    Layer1.send_packet((char*) sending, packet.totalLength);
    _messageCount++;
    return _messageCount;
}

int LL2Class::pushToOutBuffer(struct Packet packet){

    if(_outBufferEntry > 7){
        _outBufferEntry = 0;
    }

    memset(&_outBuffer[_outBufferEntry], 0, sizeof(_outBuffer[_outBufferEntry]));
    memcpy(&_outBuffer[_outBufferEntry], &packet, sizeof(_outBuffer[_outBufferEntry]));
    _outBufferEntry++;
    return _outBufferEntry;
}

struct Packet LL2Class::popFromOutBuffer(){

    _outBufferEntry--;
    struct Packet pop;
    memcpy(&pop, &_outBuffer[_outBufferEntry], sizeof(pop));
    return pop; 
}

void LL2Class::checkOutBuffer(){

    if (_outBufferEntry > 0){
        struct Packet packet = popFromOutBuffer();
        sendPacket(packet);
    }
    //else buffer is empty;
}

int LL2Class::pushToInBuffer(struct Packet packet){
    if(_inBufferEntry > 7){
        _inBufferEntry = 0;
    }

    memset(&_inBuffer[_inBufferEntry], 0, sizeof(_inBuffer[_inBufferEntry]));
    memcpy(&_inBuffer[_inBufferEntry], &packet, sizeof(_inBuffer[_inBufferEntry]));
    _inBufferEntry++;
    return _inBufferEntry;
}

struct Packet LL2Class::popFromInBuffer(){
    struct Packet pop = { 0, 0 };
    if(_inBufferEntry > 0){
        _inBufferEntry--;
        memcpy(&pop, &_inBuffer[_inBufferEntry], sizeof(pop));
    }
    return pop;
}

struct Packet LL2Class::buildPacket( uint8_t ttl, uint8_t src[6], uint8_t dest[6], uint8_t sequence, uint8_t type, uint8_t data[240], uint8_t dataLength){

    uint8_t packetLength = HEADER_LENGTH + dataLength;
    uint8_t* buffer = (uint8_t*)  malloc(dataLength);
    buffer = (uint8_t*) data;
    struct Packet packet = {
        ttl,
        packetLength,
        src[0], src[1], src[2], src[3], src[4], src[5],
        dest[0], dest[1], dest[2], dest[3], dest[4], dest[5],
        sequence,
        type 
    };
    memcpy(&packet.data, buffer, packet.totalLength);
    return packet;
}

void LL2Class::printMetadata(struct Metadata metadata){
    Serial.printf("RSSI: %x\n", metadata.rssi);
    Serial.printf("SNR: %x\n", metadata.snr);
}

void LL2Class::printPacketInfo(struct Packet packet){

    Serial.printf("\r\n");
    Serial.printf("ttl: %d\r\n", packet.ttl);
    Serial.printf("length: %d\r\n", packet.totalLength);
    Serial.printf("source: ");
    for(int i = 0 ; i < ADDR_LENGTH ; i++){
        Serial.printf("%x", packet.source[i]);
    }
    Serial.printf("\r\n");
    Serial.printf("destination: ");
    for(int i = 0 ; i < ADDR_LENGTH ; i++){
        Serial.printf("%x", packet.destination[i]);
    }
    Serial.printf("\r\n");
    Serial.printf("sequence: %02x\r\n", packet.sequence);
    Serial.printf("type: %c\r\n", packet.type);
    Serial.printf("data: ");
    for(int i = 0 ; i < packet.totalLength-HEADER_LENGTH ; i++){
        Serial.printf("%02x", packet.data[i]);
    }
    Serial.printf("\r\n");
}

void LL2Class::printNeighborTable(){

    Serial.printf("\n");
    Serial.printf("Neighbor Table:\n");
    for( int i = 0 ; i < _neighborEntry ; i++){
        for(int j = 0 ; j < ADDR_LENGTH ; j++){
            Serial.printf("%02x", _neighborTable[i].address[j]);
        }
        Serial.printf(" %3d ", _neighborTable[i].metric);
        Serial.printf("\n");
    }
    Serial.printf("\n");
}

void LL2Class::printRoutingTable(){

    Serial.printf("\n");
    Serial.printf("Routing Table: total routes %d\n", _routeEntry);
    for( int i = 0 ; i < _routeEntry ; i++){
        Serial.printf("%d hops from ", _routeTable[i].distance);
        for(int j = 0 ; j < ADDR_LENGTH ; j++){
            Serial.printf("%02x", _routeTable[i].destination[j]);
        }
        Serial.printf(" via ");
        for(int j = 0 ; j < ADDR_LENGTH ; j++){
            Serial.printf("%02x", _routeTable[i].nextHop[j]);
        }
        Serial.printf(" metric %3d ", _routeTable[i].metric);
        Serial.printf("\n");
    }
    Serial.printf("\n\n");
}

void LL2Class::printAddress(uint8_t address[ADDR_LENGTH]){
    for( int i = 0 ; i < ADDR_LENGTH; i++){
        Serial.printf("%02x", address[i]);
    }
}

uint8_t LL2Class::calculatePacketLoss(int entry, uint8_t sequence){

    uint8_t packet_loss;
    uint8_t sequence_diff = sequence - _neighborTable[entry].lastReceived;
    if(sequence_diff == 0){
        // this is first packet received from neighbor
        // assume perfect packet success
        _neighborTable[entry].packet_success = 0xFF;
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

uint8_t LL2Class::calculateMetric(int entry, uint8_t sequence, struct Metadata metadata){

    float weightedPacketSuccess =  ((float) _neighborTable[entry].packet_success)*_packetSuccessWeight;
    float weightedRandomness =  ((float) metadata.randomness)*_randomMetricWeight;
    //float weightedRSSI =  ((float) metadata.rssi)*RSSIWeight;
    //float weightedSNR =  ((float) metadata.snr)*SNRWeight;
    uint8_t metric = weightedPacketSuccess+weightedRandomness;
    Layer1.debug_printf("weighted packet success: %3f\n", weightedPacketSuccess);
    Layer1.debug_printf("weighted randomness: %3f\n", weightedRandomness);
    //Layer1.debug_printf("weighted RSSI: %3f\n", weightedRSSI);
    //Layer1.debug_printf("weighted SNR: %3f\n", weightedSNR);
    Layer1.debug_printf("metric calculated: %3d\n", metric);
    return metric;
}

int LL2Class::checkNeighborTable(struct NeighborTableEntry neighbor){

    int entry = _routeEntry;
    for( int i = 0 ; i < _neighborEntry ; i++){
        //had to use memcmp instead of strcmp?
        if(memcmp(neighbor.address, _neighborTable[i].address, sizeof(neighbor.address)) == 0){
            entry = i; 
        }
    }
    return entry;
}

int LL2Class::checkRoutingTable(struct RoutingTableEntry route){

    int entry = _routeEntry; // assume this is a new route
    for( int i = 0 ; i < _routeEntry ; i++){
        if(memcmp(route.destination, Layer1.localAddress(), sizeof(route.destination)) == 0){
            //this is me don't add to routing table 
            //Layer1.debug_printf("this route is my local address\n");
            entry = -1;
            return entry;
        }else 
        if(memcmp(route.destination, _routeTable[i].destination, sizeof(route.destination)) == 0){
            if(memcmp(route.nextHop, _routeTable[i].nextHop, sizeof(route.nextHop)) == 0){
                // already have this exact route, update metric
                entry = i; 
                return entry;
            }else{
                // already have this destination, but via a different neighbor
                if(route.distance < _routeTable[i].distance){
                    // replace route if distance is better 
                    entry = i;
                }else 
                if(route.distance == _routeTable[i].distance){
                    if(route.metric > _routeTable[i].metric){
                    // replace route if distance is equal and metric is better 
                        entry = i;
                    }else{
                        entry = -1;
                    }
                }else{
                    // ignore route if distance and metric are worse
                    entry = -1;
                }
                return entry;
            }
        } 
    }
    return entry;
}

int LL2Class::updateNeighborTable(struct NeighborTableEntry neighbor, int entry){

    memset(&_neighborTable[entry], 0, sizeof(_neighborTable[entry]));
    memcpy(&_neighborTable[entry], &neighbor, sizeof(_neighborTable[entry]));
    if(entry == _neighborEntry){
        _neighborEntry++;
        Layer1.debug_printf("new neighbor found: ");
    }else{
        Layer1.debug_printf("neighbor updated! ");
    }
    return entry;
}

int LL2Class::updateRouteTable(struct RoutingTableEntry route, int entry){

    memset(&_routeTable[entry], 0, sizeof(_routeTable[entry]));
    memcpy(&_routeTable[entry], &route, sizeof(_routeTable[entry]));
    if(entry == _routeEntry){
        _routeEntry++;
        Layer1.debug_printf("new route found! ");
    }else{
        Layer1.debug_printf("route updated! ");
    }
    printAddress(_routeTable[entry].destination);
    Layer1.debug_printf("\n");
    return entry;
}

int LL2Class::selectRoute(struct Packet packet){

    int entry = -1;
    for( int i = 0 ; i < _routeEntry ; i++){
        if(memcmp(packet.destination, _routeTable[i].destination, sizeof(packet.destination)) == 0){
            entry = i;
        }
    }
    return entry;
}

void LL2Class::retransmitRoutedPacket(struct Packet packet, struct RoutingTableEntry route){

    // decrement ttl
    packet.ttl--;
    Serial.printf("retransmitting\n");
    uint8_t data[240];
    int dataLength = 0;
    for( int i = 0 ; i < ADDR_LENGTH ; i++){
        data[dataLength] = route.nextHop[i];
        dataLength++;
    }
    struct Packet newMessage = buildPacket(packet.ttl, packet.source, packet.destination, packet.sequence, packet.type, data, dataLength); 

    // queue packet to be transmitted
    pushToOutBuffer(newMessage);
}

int LL2Class::parseHelloPacket(struct Packet packet, struct Metadata metadata){

    struct NeighborTableEntry neighbor;
    memcpy(neighbor.address, packet.source, sizeof(neighbor.address));
    int n_entry = checkNeighborTable(neighbor);
    neighbor.lastReceived = packet.sequence;
    uint8_t packet_loss = calculatePacketLoss(n_entry, packet.sequence);
    neighbor.packet_success = _neighborTable[n_entry].packet_success - packet_loss;
    uint8_t metric = calculateMetric(n_entry, packet.sequence, metadata); 
    neighbor.metric = metric;
    updateNeighborTable(neighbor, n_entry);  

    struct RoutingTableEntry route;
    memcpy(route.destination, packet.source, ADDR_LENGTH);
    memcpy(route.nextHop, packet.source, ADDR_LENGTH);
    route.distance = 1;
    route.metric = _neighborTable[n_entry].metric;
    int r_entry = checkRoutingTable(route);
    if(r_entry == -1){
        Layer1.debug_printf("do nothing, already have better route to ");
        printAddress(route.destination);
        Layer1.debug_printf("\n");
    }else{
        //if(_routeEntry <= 30){
        updateRouteTable(route, r_entry);
        //}
    }
    return n_entry;
}

int LL2Class::parseRoutingPacket(struct Packet packet, struct Metadata metadata){
    int numberOfRoutes = (packet.totalLength - HEADER_LENGTH) / (ADDR_LENGTH+2);
    Layer1.debug_printf("routes in packet: %d\n", numberOfRoutes);

    int n_entry = parseHelloPacket(packet, metadata);

    for( int i = 0 ; i < numberOfRoutes ; i++){
        struct RoutingTableEntry route; 
        memcpy(route.destination, packet.data + (ADDR_LENGTH+2)*i, ADDR_LENGTH);
        memcpy(route.nextHop, packet.source, ADDR_LENGTH);
        route.distance = packet.data[(ADDR_LENGTH+2)*i + ADDR_LENGTH]; 
        route.distance++; // add a hop to distance
        float metric = (float) packet.data[(ADDR_LENGTH+2)*i + ADDR_LENGTH+1];

        int entry = checkRoutingTable(route);
        if(entry == -1){
            Layer1.debug_printf("do nothing, already have route to ");
            printAddress(route.destination);
            Layer1.debug_printf("\n");
        }else{
            // average neighbor metric with rest of route metric
            float hopRatio = 1/((float)route.distance);
            metric = ((float) _neighborTable[n_entry].metric)*(hopRatio) + ((float)route.metric)*(1-hopRatio);
            route.metric = (uint8_t) metric;
            //if(_routeEntry <= 30){
            updateRouteTable(route, entry);
            //}
        }
    }
    return numberOfRoutes;
}

void LL2Class::parseChatPacket(struct Packet packet){
    
    if(memcmp(packet.destination, Layer1.localAddress(), sizeof(packet.destination)) == 0){
        Serial.printf("this message is for me\n");
        return;
    }
    uint8_t nextHop[ADDR_LENGTH];
    memcpy(nextHop, packet.data, sizeof(nextHop));
    if(memcmp(nextHop, Layer1.localAddress(), sizeof(nextHop)) == 0){
        Serial.printf("I am the next hop ");
        int entry = selectRoute(packet);
        if(entry == -1){
            Serial.printf(" but I don't have a route\n");
        }else{
            Serial.printf(" and I have a route RETRANSMIT\n");
            retransmitRoutedPacket(packet, _routeTable[entry]);
        }
    }else{
        Serial.printf("I am not the next hop, packet dropped\n");
    }
}
    
int LL2Class::packetReceived(char* data, size_t len) {

    data[len] = '\0';

    if(len <= 0){
        return 0;
    }

    // convert ASCII data to pure bytes
    uint8_t* byteData = ( uint8_t* ) data;
    
    // randomly generate RSSI and SNR values 
    // see https://github.com/sudomesh/disaster-radio-simulator/issues/3
    //uint8_t packet_rssi = rand() % (256 - 128) + 128;
    //uint8_t packet_snr = rand() % (256 - 128) + 128;
    // articial packet loss
    //uint8_t packet_randomness = rand() % (256 - 128) + 128;
    
    struct Metadata metadata;
    struct Packet packet = {
        byteData[0],
        byteData[1], 
        byteData[2], byteData[3], byteData[4], byteData[5], byteData[6], byteData[7],
        byteData[8], byteData[9], byteData[10], byteData[11], byteData[12], byteData[13],
        byteData[14],
        byteData[15],
    };
    memcpy(packet.data, byteData + HEADER_LENGTH, packet.totalLength-HEADER_LENGTH);

    switch(packet.type){
        case 'h' :
            // hello packet;
            parseHelloPacket(packet, metadata);
            //printNeighborTable();
            break;
        case 'r':
            // routing packet;
            //parseHelloPacket(packet, metadata);
            parseRoutingPacket(packet, metadata);
            //printRoutingTable();
            break;
        case 'c' :
            // chat packet
            parseChatPacket(packet);
            //Serial.printf("this is a chat message\n");
            break;
        case 'm' :
            Serial.printf("this is a map message\n");
            break;
        default :
            printPacketInfo(packet);
            Serial.printf("message type not found\n");
    }
    pushToInBuffer(packet);
    return 0;
}

long LL2Class::transmitHello(long interval, long lastTime){

    long newLastTime = 0;
    if (Layer1.getTime() - lastTime > interval) {
        uint8_t data[240] = "Hola";
        int dataLength = 4;
        //TODO: add randomness to message to avoid hashisng issues
        uint8_t destination[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
        struct Packet helloMessage = buildPacket(1, Layer1.localAddress(), destination, _messageCount, 'h', data, dataLength);
        sendPacket(helloMessage);
        newLastTime = Layer1.getTime();
    }
    return newLastTime;
}

long LL2Class::transmitRoutes(long interval, long lastTime){

    long newLastTime = 0;
    if (Layer1.getTime() - lastTime > interval) {
        uint8_t data[240];
        int dataLength = 0;
        Serial.printf("transmitting routes\r\n");
        Layer1.debug_printf("number of routes before transmit: %d\n", _routeEntry);
        int routesPerPacket = _routeEntry;
        if (_routeEntry >= MAX_ROUTES_PER_PACKET-1){
            routesPerPacket = MAX_ROUTES_PER_PACKET-1;
        }
        // random select without replacement of routes
        for( int i = 0 ; i < routesPerPacket ; i++){
            for( int j = 0 ; j < ADDR_LENGTH ; j++){
                data[dataLength] = _routeTable[i].destination[j];
                dataLength++;
            }
            data[dataLength] = _routeTable[i].distance; //distance
            dataLength++;
            data[dataLength] = _routeTable[i].metric;
            dataLength++;
        }
        Layer1.debug_printf("Sending data: ");
        for(int i = 0 ; i < dataLength ; i++){
            Layer1.debug_printf("%02x ", data[i]);
        }
        Layer1.debug_printf("\n");
        uint8_t destination[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
        struct Packet routeMessage = buildPacket(1, Layer1.localAddress(), destination, _messageCount, 'r', data, dataLength);
        printPacketInfo(routeMessage);
        sendPacket(routeMessage);
        newLastTime = Layer1.getTime();
    }
    return newLastTime;
}

long LL2Class::transmitToRoute(long interval, long lastTime, int dest){
    long newLastTime = 0;
    if (Layer1.getTime() - lastTime > interval) {
        if (_routeEntry == 0){
            Serial.printf("trying to send but I have no routes ");
            newLastTime = Layer1.getTime();
            return newLastTime;
        }

        uint8_t destination[ADDR_LENGTH];
        memcpy(destination, &_routeTable[dest].destination, sizeof(destination));

        Serial.printf("trying to send a random message to ");
        for( int j = 0 ; j < ADDR_LENGTH ; j++){
            Serial.printf("%02x", _routeTable[dest].destination[j]);
        }
        Serial.printf(" via ");
        for( int j = 0 ; j < ADDR_LENGTH ; j++){
            Serial.printf("%02x", _routeTable[dest].nextHop[j]);
        }
        Serial.printf("\n");

        uint8_t data[240];
        int dataLength = 0;
        for( int i = 0 ; i < ADDR_LENGTH ; i++){
            data[dataLength] = _routeTable[dest].nextHop[i];
            dataLength++;
        }
        struct Packet randomMessage = buildPacket(32, Layer1.localAddress(), destination, _messageCount, 'c', data, dataLength);
        sendPacket(randomMessage);
        _messageCount++;
        newLastTime = Layer1.getTime();
    }
    return newLastTime;
}

int LL2Class::getRouteEntry(){
    return _routeEntry;
}

LL2Class LL2;
