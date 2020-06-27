#ifdef SIM
#include <Layer1_Sim.h>
#include <LoRaLayer2.h>

long _millis = 0;

Layer1Class::Layer1Class()
{
    _nodeID = 1;
    _transmitting = 0;
    _timeDistortion = 1;
    _spreadingFactor = 9;
    txBuffer = new packetBuffer();
    rxBuffer = new packetBuffer();
}

// 1 second in the simulation == 1 second in real life * timeDistortion
int Layer1Class::setTimeDistortion(float newDistortion){
    _timeDistortion = newDistortion;
	return 0;
}

float Layer1Class::timeDistortion(){
    return _timeDistortion;
}

int Layer1Class::simulationTime(int realTime){
    return realTime * timeDistortion();
}

int Layer1Class::getTime(){
    return _millis;
}

void Layer1Class::setTime(int millis){
    _millis = millis;
}

int Layer1Class::spreadingFactor(){
    return _spreadingFactor;
}

int Layer1Class::setNodeID(int newID){
    _nodeID = newID;
    return 0;
}

int Layer1Class::nodeID(){
    return _nodeID;
}

int Layer1Class::begin_packet(){
    if(_transmitting == 1){
        // transmission in progress, do not begin packet
        return 0;
    }else{
        // transmission done, begin packet
        return 1;
    }
}

int Layer1Class::parse_metadata(char* data, uint8_t len){
    data[len] = '\0';
    switch(data[0]){
        case 't':
            if(data[1] == '0'){
                _transmitting = 0;
            }else{
                _transmitting = 1;
            }
            break;
        case 'd':
            setTimeDistortion(strtod((data + 1), NULL));
            break;
        default:
            perror("invalid metadata");
    }
    return 0;
}

int Layer1Class::sendPacket(char* data, uint8_t len) {
    char packet[258];
    ssize_t written = 0;
    ssize_t ret;
    if(!len) {
        len = strlen(data);
    }
    if(len > 256) {
        fprintf(stderr, "Attempted to send packet larger than 256 bytes\n");
        return -1;
    }
    packet[0] = len;
    memcpy(packet+1, data, len);
    while(written < len) {
        ret = write(STDOUT, (void*) packet, len+1);
        if(ret < 0) {
            return ret;
        }
        written += ret;
    }
    printf("\n");
    fflush(stdout);
    #ifdef DEBUG
    Serial.printf("Layer1::sendPacket(): packet = ");
    for(int i = 0; i < len; i++){
      Serial.printf("%c", packet[i]);
    }
    Serial.printf("\r\n");
    #endif
    return 0;
}

int Layer1Class::transmit(){
    BufferEntry entry = txBuffer->read();
    if(entry.length > 0){
        sendPacket(entry.data, entry.length);
    }
    return entry.length;
}

int Layer1Class::receive(){
    // Simulator receive occurs asynchronously in simulator.c
    // LL2 must poll Layer1 receive buffer
    return 1;
}

#endif
