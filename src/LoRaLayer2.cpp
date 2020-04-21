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
    _packetSuccessWeight = 1;
    _neighborEntry = 0;
    _routeEntry = 0;
    _routingInterval = 15000;
    _disableRoutingPackets = 0;
    _dutyInterval = 0;
    _dutyCycle = .1;
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

void LL2Class::setDutyCycle(double dutyCycle){
    _dutyCycle = dutyCycle;
}

/* Layer 1 wrappers for packetBuffers
*/
void LL2Class::writePacket(uint8_t* data, size_t length){
    data[length] = '\0';
    if(length <= 0){
        return;
    }
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
int LL2Class::writeData(Datagram datagram, size_t length){
    uint8_t hopCount = 0; // inital hopCount of zero
    int broadcast = 1; // allow broadcasts to be sent
    return route(DEFAULT_TTL, _localAddress, hopCount, datagram, length, broadcast);
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
    buf += sprintf(buf, "\0");
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
    buf += sprintf(buf, "\0");
}

void LL2Class::printPacketInfo(Packet packet){

    Serial.printf("ttl: %d\r\n", packet.ttl);
    Serial.printf("length: %d\r\n", packet.totalLength);
    Serial.printf("source: ");
    for(int i = 0 ; i < ADDR_LENGTH ; i++){
        Serial.printf("%x", packet.sender[i]);
    }
    Serial.printf("\r\n");
    Serial.printf("destination: ");
    for(int i = 0 ; i < ADDR_LENGTH ; i++){
        Serial.printf("%x", packet.receiver[i]);
    }
    Serial.printf("\r\n");
    Serial.printf("sequence: %02x\r\n", packet.sequence);
    /*
    Serial.printf("data: ");
    for(int i = 0 ; i < packet.totalLength-HEADER_LENGTH ; i++){
        Serial.printf("%02x", packet.datagram[i]);
    }
    */
    Serial.printf("\r\n");
}

/* Routing utility functions
*/

double calculateAirtime(double length, double spreadingFactor, double explicitHeader, double lowDR, double codingRate, double bandwidth){
    double timePerSymbol = pow(2, spreadingFactor)/(bandwidth);
    double arg = ceil(((8*length)-(4*spreadingFactor)+28+16-(20*(1-explicitHeader)))/(4*(spreadingFactor-2*lowDR)))*(codingRate);
    double symbolsPerPayload=8+(max(arg, 0.0));
    double timePerPayload = timePerSymbol*symbolsPerPayload;
    return timePerPayload;
}

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

uint8_t LL2Class::calculateMetric(int entry){
    // other metric values could be introduced here, currently only packet success is considered
    float weightedPacketSuccess =  ((float) _neighborTable[entry].packet_success)*_packetSuccessWeight;
    uint8_t metric = weightedPacketSuccess;
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
                if(route.distance < _routeTable[i].distance){ // TODO: this assumes shortest route is best, which may not be true.
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
    memset(&_neighborTable[entry], 0, sizeof(_neighborTable[entry]));
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
    memset(&_routeTable[entry], 0, sizeof(_routeTable[entry]));
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

int LL2Class::parseForNeighbor(Packet packet){
    // Create neighbor table entry with sender address
    NeighborTableEntry neighbor;
    memcpy(neighbor.address, packet.sender, sizeof(neighbor.address));
    // Find neighbor table entry for sender
    int n_entry = checkNeighborTable(neighbor);
    // Calculate packet loss to find metric of link
    uint8_t packet_loss = calculatePacketLoss(n_entry, packet.sequence);
    neighbor.packet_success = _neighborTable[n_entry].packet_success - packet_loss;
    neighbor.lastReceived = packet.sequence;
    neighbor.metric = calculateMetric(n_entry);
    // update neighbor table with neighbor entry
    updateNeighborTable(neighbor, n_entry);  
    // update routing table with neighbor entry also
    RoutingTableEntry route;
    memcpy(route.destination, packet.sender, ADDR_LENGTH);
    memcpy(route.nextHop, packet.sender, ADDR_LENGTH);
    route.distance = 1;
    route.metric = _neighborTable[n_entry].metric;
    int r_entry = checkRoutingTable(route);
    if(r_entry >= 0){
        updateRouteTable(route, r_entry);
    }
    return n_entry;
}

/* Entry point to build routing table
*/
void LL2Class::parseHeader(Packet packet){
    // Parse packet header to update neighbor (i.e. sender address)
    int n_entry = parseForNeighbor(packet);

    if(memcmp(packet.receiver, _localAddress, ADDR_LENGTH) != 0 && 
      memcmp(packet.receiver, _broadcastAddr, ADDR_LENGTH) != 0){
      // packet was meant for someone else, but still parse for reciever route
      RoutingTableEntry rcv_route;
      memcpy(rcv_route.destination, packet.receiver, ADDR_LENGTH);
      memcpy(rcv_route.nextHop, packet.sender, ADDR_LENGTH);
      // assumes the node is still alive and has not moved since the sender discovered this neighbor
      rcv_route.distance = 2;
      // metric unknown until you hear a routed packet from node?
      rcv_route.metric = 0;
      int r_entry = checkRoutingTable(rcv_route);
      if(r_entry >= 0){
        updateRouteTable(rcv_route, r_entry);
      }
    }

    if(memcmp(packet.sender, packet.source, ADDR_LENGTH) != 0){
      // source is different from sender, parse for source route
      RoutingTableEntry src_route;
      memcpy(src_route.destination, packet.source, ADDR_LENGTH);
      memcpy(src_route.nextHop, packet.sender, ADDR_LENGTH);
      src_route.distance = packet.hopCount+1;
      // average neighbor metric with rest of route metric
      float hopRatio = 1/((float)src_route.distance);
      float metric = ((float) _neighborTable[n_entry].metric)*(hopRatio) + ((float)packet.metric)*(1-hopRatio);
      src_route.metric = (uint8_t) metric;
      int r_entry = checkRoutingTable(src_route);
      if(r_entry >= 0){
          updateRouteTable(src_route, r_entry);
      }
    }
    return;
}

Packet LL2Class::buildPacket(uint8_t ttl, uint8_t nextHop[ADDR_LENGTH], uint8_t source[ADDR_LENGTH], uint8_t hopCount, uint8_t metric,  Datagram datagram, size_t length){
    uint8_t totalLength = HEADER_LENGTH + length;
    Packet packet = {ttl, totalLength};
    memcpy(packet.sender, _localAddress, ADDR_LENGTH);
    memcpy(packet.receiver, nextHop, ADDR_LENGTH);
    packet.sequence = messageCount();
    memcpy(packet.source, source, ADDR_LENGTH);
    packet.hopCount = hopCount;
    packet.metric = metric;
    memcpy(&packet.datagram, &datagram, length);
    return packet;
}

/*
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
*/

/* Entry point to route data
*/
int LL2Class::route(uint8_t ttl, uint8_t source[ADDR_LENGTH], uint8_t hopCount, Datagram datagram, size_t length, int broadcast){
    int ret = -1;
    if(ttl > 0){
      int src_entry = selectRoute(source);
      if(memcmp(datagram.destination, _loopbackAddr, ADDR_LENGTH) == 0){
        //Loopback
        // this should push back into the L3 buffer
      }else if(memcmp(datagram.destination, _broadcastAddr, ADDR_LENGTH) == 0){
        //Broadcast packet, only forward if explicity told to
        if(broadcast == 1){
          Packet packet = buildPacket(ttl, _broadcastAddr, source, hopCount, _routeTable[src_entry].metric, datagram, length);
          ret = L2toL1.write(packet);
        }
      }else{
        // Look for route in routing table
        int dst_entry = selectRoute(datagram.destination);
        if(dst_entry >= 0){
          // Route found
          // build packet with new ttl, nextHop, and route metric
          Packet packet = buildPacket(ttl, _routeTable[dst_entry].nextHop, source, hopCount, _routeTable[src_entry].metric, datagram, length);
          // return packet's position in buffer
          ret = L2toL1.write(packet);
        }
      }
    }
    return ret;
}

/* Receive and decide function
*/
void LL2Class::receive(){
  Packet packet = L1toL2.read();
  if(packet.totalLength > 0){
    parseHeader(packet);
    if(memcmp(packet.datagram.destination, _localAddress, ADDR_LENGTH) == 0 || 
      memcmp(packet.receiver, _broadcastAddr, ADDR_LENGTH) == 0){
      // packet is meant for me (or everyone)
      L2toL3.write(packet);
    }else if(memcmp(packet.receiver, _localAddress, ADDR_LENGTH) == 0){
      // packet is meant for someone else
      // but I am the intended forwarder
      // strip header and route new packet
      packet.ttl--;
      packet.hopCount++;
      route(packet.ttl, packet.source, packet.hopCount, packet.datagram, packet.totalLength, 0);
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
    _lastTransmitTime = _startTime;
    setAddress(_loopbackAddr, "00000000");
    setAddress(_broadcastAddr, "ffffffff");
    setAddress(_routingAddr, "afffffff");
    return 0;
}

/* Main loop function
*/
int LL2Class::daemon(){
    int ret = -1;
    // try adding a routing packet to L2toL1 buffer, if interval is up and routing is enabled
    if (Layer1.getTime() - _lastRoutingTime > _routingInterval && _disableRoutingPackets == 0) {
        //Packet routingPacket = buildRoutingPacket();
        // need to init with broadcast addr and then loop through routing table to build routing table
        //L2toL1.write(routingPacket);
        _lastRoutingTime = Layer1.getTime();
    }

    // try transmitting a packet
    if (Layer1.getTime() - _lastTransmitTime > _dutyInterval){
        int length = Layer1.transmit();
        if(length > 0){
          _lastTransmitTime = Layer1.getTime();
          double airtime = calculateAirtime((double)length, (double)Layer1.spreadingFactor(), 1, 0, 5, 125);
          _dutyInterval = (int)ceil(airtime/_dutyCycle);
          _messageCount = (_messageCount + 1) % 256;
        }
    }

    // see if there are any packets to be received (i.e. in L1toL2 buffer)
    receive();

    // returns sequence number of transmitted packet,
    // if no packet transmitted, will return -1
    return ret;
}

LL2Class LL2;
