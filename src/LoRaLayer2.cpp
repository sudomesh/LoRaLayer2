#include <Layer1.h>
#include <LoRaLayer2.h>

/* Fifo Buffer Class
*/
packetBuffer::packetBuffer()
{
    head = 0;
    tail = 0;
}

// reads a packet from buffer
Packet packetBuffer::read(){
    Packet packet;
    memset(&packet, 0, sizeof(packet));
    if (head == tail){
        // if buffer empty, return empty packet
        return packet;
    }
    tail = (tail + 1) % BUFFERSIZE;
    return buffer[tail]; // otherwise return packet from tail
}

// writes a packet to buffer
int packetBuffer::write(Packet packet) {
    if (((head + 1) % BUFFERSIZE) == tail){
      // if full, return the size of the buffer
      return BUFFERSIZE;
    }
    head = (head + 1) % BUFFERSIZE;
    memset(&buffer[head], 0, sizeof(buffer[head]));
    buffer[head] = packet; // otherwise store packet in buffer
    return head-tail; // and return the packet's place in line
}

/* LoRaLayer2 Class
*/
LL2Class::LL2Class()
{
    _messageCount = 0;
    _packetSuccessWeight = .8;
    _randomMetricWeight = .2;
    _neighborEntry = 0;
    _routeEntry = 0;
    _routingInterval = 15000;
    _disableRoutingPackets = 0;
}

/* Public access to local variables
*/
uint8_t LL2Class::messageCount(){
    return _messageCount;
}

uint8_t* LL2Class::localAddress(){
    return _localAddress;
}

uint8_t* LL2Class::broadcastAddr(){
    return _broadcastAddr;
}

uint8_t* LL2Class::loopbackAddr(){
    return _loopbackAddr;
}

uint8_t* LL2Class::routingAddr(){
    return _routingAddr;
}

int LL2Class::getRouteEntry(){
    return _routeEntry;
}

/* General purpose utility functions
*/
uint8_t LL2Class::hexDigit(char ch){
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

void LL2Class::setAddress(uint8_t* addr, const char* macString){
  for( int i = 0; i < ADDR_LENGTH; ++i ){
    addr[i]  = hexDigit( macString[2*i] ) << 4;
    addr[i] |= hexDigit( macString[2*i+1] );
  }
}

/* User configurable settings
*/
int LL2Class::setLocalAddress(const char* macString){
  setAddress(_localAddress, macString);
  if(_localAddress){
    return 1;
  }else{
    return 0;
  }
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

/* Layer 1 wrappers for packetBuffers
*/
void LL2Class::writePacket(uint8_t* data, size_t length){
    if(length <= 0){
        return;
    }
    data[length] = '\0';
    Packet packet;
    memcpy(&packet, data, length);
    L1toL2.write(packet);
    return;
}

Packet LL2Class::readPacket(){
    return L2toL1.read();
}

/* Layer 3 wrappers for packetBuffers
*/
int LL2Class::writeData(uint8_t* data, size_t length){
    return route(DEFAULT_TTL, data, length, 1);
}

Packet LL2Class::readData(){
    return L2toL3.read();
}

/* Print out functions, for convenience
*/
void LL2Class::getNeighborTable(char *out){
    char* buf = out;
    buf += sprintf(buf, "Neighbor Table:\n");
    for( int i = 0 ; i < _neighborEntry ; i++){
        for(int j = 0 ; j < ADDR_LENGTH ; j++){
            buf += sprintf(buf, "%02x", _neighborTable[i].address[j]);
        }
        buf += sprintf(buf, " %3d ", _neighborTable[i].metric);
        buf += sprintf(buf, "\r\n");
    }
}

void LL2Class::getRoutingTable(char *out){
    char* buf = out;
    buf += sprintf(buf, "Routing Table: total routes %d\r\n", _routeEntry);
    for( int i = 0 ; i < _routeEntry ; i++){
        buf += sprintf(buf, "%d hops from ", _routeTable[i].distance);
        for(int j = 0 ; j < ADDR_LENGTH ; j++){
            buf += sprintf(buf, "%02x", _routeTable[i].destination[j]);
        }
        buf += sprintf(buf, " via ");
        for(int j = 0 ; j < ADDR_LENGTH ; j++){
            buf += sprintf(buf, "%02x", _routeTable[i].nextHop[j]);
        }
        buf += sprintf(buf, " metric %3d ", _routeTable[i].metric);
        buf += sprintf(buf, "\r\n");
    }
}

void LL2Class::printPacketInfo(Packet packet){

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
}

/* Routing utility functions
*/
uint8_t LL2Class::calculatePacketLoss(int entry, uint8_t sequence){
    uint8_t packet_loss = 0xFF;
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
    }
    // no packet received recently
    // assume complete packet loss
    return packet_loss;
}

uint8_t LL2Class::calculateMetric(int entry, uint8_t sequence, Metadata metadata){
    float weightedPacketSuccess =  ((float) _neighborTable[entry].packet_success)*_packetSuccessWeight;
    float weightedRandomness =  ((float) metadata.randomness)*_randomMetricWeight;
    uint8_t metric = weightedPacketSuccess+weightedRandomness;
    return metric;
}

int LL2Class::checkNeighborTable(NeighborTableEntry neighbor){
    int entry = _routeEntry;
    for( int i = 0 ; i < _neighborEntry ; i++){
        //had to use memcmp instead of strcmp?
        if(memcmp(neighbor.address, _neighborTable[i].address, sizeof(neighbor.address)) == 0){
            entry = i; 
        }
    }
    return entry;
}

int LL2Class::checkRoutingTable(RoutingTableEntry route){
    int entry = _routeEntry; // assume this is a new route
    for( int i = 0 ; i < _routeEntry ; i++){
        if(memcmp(route.destination, localAddress(), sizeof(route.destination)) == 0){
            //this is me don't add to routing table 
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

int LL2Class::updateNeighborTable(NeighborTableEntry neighbor, int entry){
    // copy neighbor into specified entry in neighbor table
    memcpy(&_neighborTable[entry], &neighbor, sizeof(_neighborTable[entry]));
    if(entry == _neighborEntry){
        // if specified entry is the same as current count of neighbors
        // this is a new neighbor, increment neighbor count
        _neighborEntry++;
    }
    //else neighbor is just updated
    return entry;
}

int LL2Class::updateRouteTable(RoutingTableEntry route, int entry){
    // copy route into specified entry in routing table
    memcpy(&_routeTable[entry], &route, sizeof(_routeTable[entry]));
    if(entry == _routeEntry){
        // if specified entry is the same as current count of routes
        // this is a new route, increment route count
        _routeEntry++;
    }
    //else route is just updated
    return entry;
}

int LL2Class::selectRoute(uint8_t destination[ADDR_LENGTH]){
    int entry = -1;
    for( int i = 0 ; i < _routeEntry ; i++){
        if(memcmp(destination, _routeTable[i].destination, ADDR_LENGTH) == 0){
            entry = i;
        }
    }
    return entry;
}

int LL2Class::parseForNeighbor(Packet packet, Metadata metadata){

    NeighborTableEntry neighbor;
    memcpy(neighbor.address, packet.source, sizeof(neighbor.address));
    int n_entry = checkNeighborTable(neighbor);
    neighbor.lastReceived = packet.sequence;
    uint8_t packet_loss = calculatePacketLoss(n_entry, packet.sequence);
    neighbor.packet_success = _neighborTable[n_entry].packet_success - packet_loss;
    uint8_t metric = calculateMetric(n_entry, packet.sequence, metadata); 
    neighbor.metric = metric;
    updateNeighborTable(neighbor, n_entry);  

    RoutingTableEntry route;
    memcpy(route.destination, packet.source, ADDR_LENGTH);
    memcpy(route.nextHop, packet.source, ADDR_LENGTH);
    route.distance = 1;
    route.metric = _neighborTable[n_entry].metric;
    int r_entry = checkRoutingTable(route);
    if(r_entry == -1){
        //do nothing, already have better route
    }else{
        //if(_routeEntry <= 30){
        updateRouteTable(route, r_entry);
        //}
    }
    return n_entry;
}

/* Entry point to build routing table
*/
int LL2Class::parseForRoutes(Packet packet, Metadata metadata){
    int numberOfRoutes = (packet.totalLength - HEADER_LENGTH) / (ADDR_LENGTH+2);
    int n_entry = parseForNeighbor(packet, metadata);
    for( int i = 0 ; i < numberOfRoutes ; i++){
        RoutingTableEntry route;
        memcpy(route.destination, packet.data + (ADDR_LENGTH+2)*i, ADDR_LENGTH);
        memcpy(route.nextHop, packet.source, ADDR_LENGTH);
        route.distance = packet.data[(ADDR_LENGTH+2)*i + ADDR_LENGTH]; 
        route.distance++; // add a hop to distance
        float metric = (float) packet.data[(ADDR_LENGTH+2)*i + ADDR_LENGTH+1];

        int entry = checkRoutingTable(route);
        if(entry == -1){
            // do nothing, already have route
        }else{
            // average neighbor metric with rest of route metric
            float hopRatio = 1/((float)route.distance);
            metric = ((float) _neighborTable[n_entry].metric)*(hopRatio) + ((float)route.metric)*(1-hopRatio);
            route.metric = (uint8_t) metric;
            if(getRouteEntry() <= 30){
                updateRouteTable(route, entry);
            }
        }
    }
    return numberOfRoutes;
}

/* Entry point to route data
*/
int LL2Class::route(uint8_t ttl, uint8_t* data, size_t length, int broadcast){
    ttl--;
    uint8_t nextHop[ADDR_LENGTH];
    uint8_t destination[ADDR_LENGTH];
    memcpy(destination, data, ADDR_LENGTH);
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
    Packet packet = buildPacket(ttl, nextHop, data, length);
    int ret = L2toL1.write(packet);
    return ret;
}


/* Packet building functions
*/
Packet LL2Class::buildPacket(uint8_t ttl, uint8_t next[ADDR_LENGTH], uint8_t data[DATA_LENGTH], uint8_t dataLength){
    uint8_t packetLength = HEADER_LENGTH + dataLength;
    uint8_t* src = localAddress();
    Packet packet = {
        ttl,
        packetLength,
        src[0], src[1], src[2], src[3],
        next[0], next[1], next[2], next[3],
        messageCount()
    };
    memcpy(packet.data, data, dataLength);
    return packet;
}


Packet LL2Class::buildRoutingPacket(){
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
    uint8_t nextHop[ADDR_LENGTH] = { 0xaf, 0xff, 0xff, 0xff };
    Packet packet = buildPacket(1, nextHop, data, dataLength);
    return packet;
}

/* Receive and decide function
*/
void LL2Class::receive(){
    Metadata metadata; // TODO: retrieve from transceiver
    Packet packet = L1toL2.read();
    if(packet.totalLength > 0){
        if(memcmp(packet.nextHop, _routingAddr, ADDR_LENGTH) == 0){
            // packet contains routing table info
            parseForRoutes(packet, metadata);
        }else if(memcmp(packet.nextHop, localAddress(), ADDR_LENGTH) == 0 || 
                 memcmp(packet.nextHop, _broadcastAddr, ADDR_LENGTH) == 0){
            // packet is meant for me (or everyone)
            L2toL3.write(packet);
        }else{
            // packet is meant for someone else
            // strip header and route new packet
            route(packet.ttl, packet.data, packet.totalLength-HEADER_LENGTH, 0);
        }
    }
    //else buffer is empty, do nothing;
    return;
}

/* Initialization function
*/
int LL2Class::init(){
    _startTime = Layer1.getTime();
    _lastRoutingTime = _startTime;
    setAddress(_loopbackAddr, "00000000");
    setAddress(_broadcastAddr, "ffffffff");
    setAddress(_routingAddr, "afffffff");
    return 0;
}

/* Main loop function
*/
int LL2Class::daemon(){
    // try adding a routing packet to L2toL1 buffer, if interval is up and routing is enabled
    if (Layer1.getTime() - _lastRoutingTime > _routingInterval && _disableRoutingPackets == 0) {
        Packet routingPacket = buildRoutingPacket();
        L2toL1.write(routingPacket);
        _lastRoutingTime = Layer1.getTime();
    }

    // try transmitting a packet
    int ret = Layer1.transmit();
    if(ret >= 0){
      // if a packet is transmitted, increment the global message count
      _messageCount = (_messageCount + 1) % 256;
    }

    // see if there are any packets to be received (i.e. in L1toL2 buffer)
    receive();

    // returns sequence number of transmitted packet,
    // if no packet transmitted, will return -1
    return ret;
}

LL2Class LL2;
