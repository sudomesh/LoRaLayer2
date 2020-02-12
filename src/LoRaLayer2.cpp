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

uint8_t _loopbackAddr[ADDR_LENGTH] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t _broadcastAddr[ADDR_LENGTH] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
uint8_t _routingAddr[ADDR_LENGTH] = { 0xaf, 0xff, 0xff, 0xff, 0xff, 0xff };

LL2Class::LL2Class() :

    _messageCount(),
    _localAddress(),
    _packetSuccessWeight(.8),
    _randomMetricWeight(.2),
    _L1OutBuffer(),
    _L1OutBufferEntry(0),
    _L1InBuffer(),
    _L1InBufferEntry(0),
    _neighborTable(),
    _neighborEntry(0),
    _routeTable(),
    _routeEntry(0),
    _startTime(),
    _lastRoutingTime(),
    _routingInterval(15000),
    _disableRoutingPackets(0)
{
}

uint8_t LL2Class::messageCount(){
    return _messageCount;
}

uint8_t* LL2Class::localAddress(){
    return _localAddress;
}

uint8_t LL2Class::hex_digit(char ch){
  if(( '0' <= ch ) && ( ch <= '9' )){
    ch -= '0';
  }else{
    if(( 'a' <= ch ) && ( ch <= 'f' )){
      ch += 10 - 'a';
    }else{
      if(( 'A' <= ch ) && ( ch <= 'F' ) ){
        ch += 10 - 'A';
      }else{
        ch = 16;
      }
    }
  }
  return ch;
}

int LL2Class::setLocalAddress(char* macString){
  for( int i = 0; i < sizeof(_localAddress)/sizeof(_localAddress[0]); ++i ){
    _localAddress[i]  = hex_digit( macString[2*i] ) << 4;
    _localAddress[i] |= hex_digit( macString[2*i+1] );
  }
  if(_localAddress){
    return 1;
  }else{
    return 0;
  }
}

int LL2Class::sendToLayer1(struct Packet packet) {

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
    Layer1.send_packet((char*)&packet, packet.totalLength);
    _messageCount++;
    return _messageCount;
}

int LL2Class::pushToL1OutBuffer(struct Packet packet){

    if(_L1OutBufferEntry > 7){
        _L1OutBufferEntry = 0;
    }
    memset(&_L1OutBuffer[_L1OutBufferEntry], 0, sizeof(_L1OutBuffer[_L1OutBufferEntry]));
    memcpy(&_L1OutBuffer[_L1OutBufferEntry], &packet, sizeof(_L1OutBuffer[_L1OutBufferEntry]));
    _L1OutBufferEntry++;
    return _L1OutBufferEntry;
}

struct Packet LL2Class::popFromL1OutBuffer(){

    _L1OutBufferEntry--;
    struct Packet pop;
    memcpy(&pop, &_L1OutBuffer[_L1OutBufferEntry], sizeof(pop));
    return pop; 
}

void LL2Class::checkL1OutBuffer(){

    if (_L1OutBufferEntry > 0){
        struct Packet packet = popFromL1OutBuffer();
        sendToLayer1(packet);
    }
    //else buffer is empty;
}

int LL2Class::pushToL3OutBuffer(struct Packet packet){
    if(_L3OutBufferEntry > 7){
        _L3OutBufferEntry = 0;
    }
    memset(&_L3OutBuffer[_L3OutBufferEntry], 0, sizeof(_L3OutBuffer[_L3OutBufferEntry]));
    memcpy(&_L3OutBuffer[_L3OutBufferEntry], &packet, sizeof(_L3OutBuffer[_L3OutBufferEntry]));
    _L3OutBufferEntry++;
    return _L3OutBufferEntry;
}

struct Packet LL2Class::popFromL3OutBuffer(){
    struct Packet pop = { 0, 0 };
    if(_L3OutBufferEntry > 0){
        _L3OutBufferEntry--;
        memcpy(&pop, &_L3OutBuffer[_L3OutBufferEntry], sizeof(pop));
    }
    return pop;
}

int LL2Class::pushToL1InBuffer(struct Packet packet){
    if(_L1InBufferEntry > 7){
        _L1InBufferEntry = 0;
    }
    memset(&_L1InBuffer[_L1InBufferEntry], 0, sizeof(_L1InBuffer[_L1InBufferEntry]));
    memcpy(&_L1InBuffer[_L1InBufferEntry], &packet, sizeof(_L1InBuffer[_L1InBufferEntry]));
    _L1InBufferEntry++;
    return _L1InBufferEntry;
}

struct Packet LL2Class::popFromL1InBuffer(){
    struct Packet pop = { 0, 0 };
    if(_L1InBufferEntry > 0){
        _L1InBufferEntry--;
        memcpy(&pop, &_L1InBuffer[_L1InBufferEntry], sizeof(pop));
    }
    return pop;
}

void LL2Class::checkL1InBuffer(){

    if (_L1InBufferEntry > 0){
        struct Metadata metadata; //needs to be retrevied from transceiver
        struct Packet packet = popFromL1InBuffer();
        if(memcmp(packet.nextHop, _routingAddr, ADDR_LENGTH) == 0){
            // packet contains routing table info
            parseForRoutes(packet, metadata);
        }else if(memcmp(packet.nextHop, localAddress(), ADDR_LENGTH) == 0 || 
                 memcmp(packet.nextHop, _broadcastAddr, ADDR_LENGTH) == 0){
            // packet is meant for me (or everyone)
            pushToL3OutBuffer(packet);
        }else{
            // packet is meant for someone else
            routePacket(packet, 0);
        }
    }
    //else buffer is empty;
}

struct Packet LL2Class::buildPacket(uint8_t ttl, uint8_t next[6], uint8_t data[DATA_LENGTH], uint8_t dataLength){

    uint8_t packetLength = HEADER_LENGTH + dataLength;
    uint8_t* src = localAddress();
    struct Packet packet = {
        ttl,
        packetLength,
        src[0], src[1], src[2], src[3], src[4], src[5],
        next[0], next[1], next[2], next[3], next[4], next[5],
        messageCount()
    };
    memcpy(packet.data, data, dataLength);
    return packet;
}

void LL2Class::printMetadata(struct Metadata metadata){
    Serial.printf("RSSI: %x\n", metadata.rssi);
    Serial.printf("SNR: %x\n", metadata.snr);
}

void LL2Class::printPacketInfo(struct Packet packet){

    Serial.printf("ttl: %d\r\n", packet.ttl);
    Serial.printf("length: %d\r\n", packet.totalLength);
    Serial.printf("source: ");
    for(int i = 0 ; i < ADDR_LENGTH ; i++){
        Serial.printf("%x", packet.source[i]);
    }
    Serial.printf("\r\n");
    Serial.printf("destination: ");
    for(int i = 0 ; i < ADDR_LENGTH ; i++){
        Serial.printf("%x", packet.nextHop[i]);
    }
    Serial.printf("\r\n");
    Serial.printf("sequence: %02x\r\n", packet.sequence);
    Serial.printf("data: ");
    for(int i = 0 ; i < packet.totalLength-HEADER_LENGTH ; i++){
        Serial.printf("%02x", packet.data[i]);
    }
    Serial.printf("\r\n");
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

    Serial.printf("\r\n");
    Serial.printf("Routing Table: total routes %d\r\n", _routeEntry);
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
        Serial.printf("\r\n");
    }
    Serial.printf("\r\n");
}

void LL2Class::printAddress(uint8_t address[ADDR_LENGTH]){
    for( int i = 0 ; i < ADDR_LENGTH; i++){
        Serial.printf("%02x", address[i]);
    }
}

void LL2Class::debug_printAddress(uint8_t address[ADDR_LENGTH]){
    for( int i = 0 ; i < ADDR_LENGTH; i++){
        Layer1.debug_printf("%02x", address[i]);
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
        if(memcmp(route.destination, localAddress(), sizeof(route.destination)) == 0){
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
    debug_printAddress(_routeTable[entry].destination);
    Layer1.debug_printf("\n");
    return entry;
}

int LL2Class::selectRoute(uint8_t destination[6]){

    int entry = -1;
    for( int i = 0 ; i < _routeEntry ; i++){
        if(memcmp(destination, _routeTable[i].destination, sizeof(destination)) == 0){
            entry = i;
        }
    }
    return entry;
}

int LL2Class::parseForNeighbor(struct Packet packet, struct Metadata metadata){

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
        debug_printAddress(route.destination);
        Layer1.debug_printf("\n");
    }else{
        //if(_routeEntry <= 30){
        updateRouteTable(route, r_entry);
        //}
    }
    return n_entry;
}

int LL2Class::parseForRoutes(struct Packet packet, struct Metadata metadata){
    int numberOfRoutes = (packet.totalLength - HEADER_LENGTH) / (ADDR_LENGTH+2);
    Layer1.debug_printf("routes in packet: %d\n", numberOfRoutes);

    int n_entry = parseForNeighbor(packet, metadata);

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
            debug_printAddress(route.destination);
            Layer1.debug_printf("\r\n");
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
    //struct Metadata metadata;
    //TODO save metadata and store with the packet in the inBuffer

    struct Packet packet = {
        byteData[0],
        byteData[1], 
        byteData[2], byteData[3], byteData[4], byteData[5], byteData[6], byteData[7],
        byteData[8], byteData[9], byteData[10], byteData[11], byteData[12], byteData[13],
        byteData[14],
        byteData[15],
    };
    memcpy(packet.data, byteData + HEADER_LENGTH, packet.totalLength-HEADER_LENGTH);
    pushToL1InBuffer(packet);
    return 0;
}

struct Packet LL2Class::buildRoutingPacket(){
    uint8_t data[DATA_LENGTH];
    int dataLength = 0;
    int routesPerPacket = _routeEntry;
    if (_routeEntry >= MAX_ROUTES_PER_PACKET-1){
        routesPerPacket = MAX_ROUTES_PER_PACKET-1;
    }
    for( int i = 0 ; i < routesPerPacket ; i++){
        for( int j = 0 ; j < ADDR_LENGTH ; j++){
            data[dataLength] = _routeTable[i].destination[j];
            dataLength++;
        }
        data[dataLength] = _routeTable[i].distance;
        dataLength++;
        data[dataLength] = _routeTable[i].metric;
        dataLength++;
    }
    uint8_t nextHop[6] = { 0xaf, 0xff, 0xff, 0xff, 0xff, 0xff };
    struct Packet packet = buildPacket(1, nextHop, data, dataLength);
    return packet;
}

int LL2Class::getRouteEntry(){
    return _routeEntry;
}

int LL2Class::routePacket(struct Packet packet, int broadcast){
    uint8_t ttl = packet.ttl--;
    uint8_t nextHop[ADDR_LENGTH];
    uint8_t destination[ADDR_LENGTH];
    memcpy(destination, packet.data, ADDR_LENGTH);
    if(ttl <= 0){
        // time to live expired
        return 1;
    }
    if(memcmp(destination, _loopbackAddr, ADDR_LENGTH) == 0){
        //Loopback
        // this should push back into the L3 buffer
        return 1;
    }else if(memcmp(destination, _broadcastAddr, ADDR_LENGTH) == 0){
        //Broadcast packet, only forward if explicity told to
        if(broadcast == 1){
            memcpy(nextHop, _broadcastAddr, ADDR_LENGTH);
        }else{
            return 1;
        }
    }else{
        int entry = selectRoute(destination);
        if(entry == -1){
            // No route found
            return 1;
        }else{
            // Route found
            memcpy(nextHop, _routeTable[entry].nextHop, ADDR_LENGTH);
        }
    }
    // rebuild packet with new ttl and nextHop
    struct Packet newPacket = buildPacket(ttl, nextHop, packet.data, packet.totalLength-HEADER_LENGTH);
    pushToL1OutBuffer(newPacket);
    return 0;
}

int LL2Class::sendToLayer2(uint8_t data[DATA_LENGTH], uint8_t dataLength){

    uint8_t ttl = DEFAULT_TTL+1;
    uint8_t nextHop[ADDR_LENGTH];
    // build a dummy packet to pass to routePacket()
    struct Packet packet = buildPacket(ttl, nextHop, data, dataLength);
    return routePacket(packet, 1);
}

int LL2Class::init(){
    _startTime = Layer1.getTime();
    _lastRoutingTime = _startTime;
    return 0;
}

long LL2Class::setInterval(long interval){
    if(interval == 0){
        _disableRoutingPackets = 1;
    }else{
        _disableRoutingPackets = 0;
        _routingInterval = interval;
    }
    return _routingInterval;
}

int LL2Class::daemon(){
    if (Layer1.getTime() - _lastRoutingTime > _routingInterval && _disableRoutingPackets == 0) {
        Layer1.debug_printf("number of routes before transmit: %d\n", _routeEntry);
        struct Packet routingPacket = buildRoutingPacket();
        pushToL1OutBuffer(routingPacket);

        // this is a hack to send the routing table up to Layer3
        struct Packet packet = routingPacket;
        packet.totalLength += 11;
        memcpy(packet.data, &_broadcastAddr, ADDR_LENGTH);
        packet.data[6] = 'r';
        packet.data[7] = 0x0;
        packet.data[8] = 0x0;
        packet.data[9] = 'r';
        packet.data[10] = '|';
        memcpy(packet.data+11, routingPacket.data, routingPacket.totalLength-HEADER_LENGTH);
        pushToL3OutBuffer(packet);

        _lastRoutingTime = Layer1.getTime();
        return 1;
    }
    checkL1OutBuffer();
    checkL1InBuffer();
    return 0;
}

LL2Class LL2;
