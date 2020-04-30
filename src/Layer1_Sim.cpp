#include <Layer1.h>
#include <LoRaLayer2.h>
#ifdef SIM
struct timeval to_sleep;
serial Serial;
serial debug;

Layer1Class::Layer1Class()
{
    _nodeID = 1;
    _transmitting = 0;
    _timeDistortion = 1;
    _spreadingFactor = 9;

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
    Packet packet = LL2.readPacket();
    if(packet.totalLength > 0){
        sendPacket((char*)&packet, packet.totalLength);
    }
    return packet.totalLength;
}

Layer1Class Layer1;

int nsleep(unsigned int secs, useconds_t usecs) {

  to_sleep.tv_sec = secs;
  to_sleep.tv_usec = usecs;
  
  return 0;
}

// STDERR acts as Serial.printf
int print_err(const char* format, ...) {
    int ret;
    va_list args;
    va_start(args, format);
    ret = vfprintf(stderr, format, args);
    va_end(args);
    fflush(stderr);
    return ret;
}

// Debug only printf option
int print_debug(const char* format, ...) {
    #ifdef DEBUG
    int ret;
    va_list args;
    va_start(args, format);
    ret = vfprintf(stderr, format, args);
    va_end(args);
    fflush(stderr);
    return ret;
    #endif
    return 0;
}

int main(int argc, char **argv) {
    int opt;
    // handle getopt arguments
    while ((opt = getopt(argc, argv, "t:a:n:")) != -1) {
        switch (opt) {
            case 't':
                Layer1.setTimeDistortion(strtod(optarg, NULL));
                break;
            case 'a':
                LL2.setLocalAddress(optarg);
                break;
            case 'n':
                Layer1.setNodeID(atoi(optarg));
                break;
            default:
                perror("Bad args\n");
                return 1;
        }
    }
    // initialize main loop variables
    char buffer[257];
    struct timeval tv;
    struct timeval start, end;
	gettimeofday(&start, NULL);
    fd_set fds;
    int flags;
    ssize_t ret;
    ssize_t len = 0;
    ssize_t got;
    ssize_t meta = 0;
    nsleep(0, 0); // set intial sleep time value to zero
    Serial.printf = &print_err;
    debug.printf = &print_debug;
    debug.printf("DEBUG PRINTING ENABLED\r\n");
    flags = fcntl(STDIN, F_GETFL, 0);
    if(flags == -1) {
        perror("Failed to get socket flags\n");
        return 1;
    }
    flags = flags | O_NONBLOCK;
    if(ret = fcntl(STDIN, F_SETFL, flags)) {
        perror("Failed to set non-blocking mode\n");
        return 1;
    }
    // Call main setup function
    if(ret = setup()) {
        return ret;
    }
    // Enter main program loop
    while(1) {
        // add STDIN to file descriptor set
        FD_ZERO(&fds);
        FD_SET(STDIN, &fds);
        // set select timeout to sleep time value
        // typically zero, unless you have called nsleep somewhere
        tv.tv_sec = to_sleep.tv_sec;
        tv.tv_usec = to_sleep.tv_usec;
        // select STDIN to see if data is available
        ret = select(STDIN + 1, &fds, NULL, NULL, &tv);
        if(ret < 0) {
          perror("select() failed");
          return 1;
        }
        // set sleep time value back to zero
        to_sleep.tv_sec = 0;
        to_sleep.tv_usec = 0;
        // if STDIN has data availble
        if(ret && FD_ISSET(STDIN, &fds)) {
            if(!meta){
                // readout the metadata flag
                ret = read(STDIN, &meta, 1);
                if(ret < 0) {
                    perror("failed to read metadata flag");
                    return 1;
                }
            }
            if(!len) {
                // readout the length of the incoming packet
                ret = read(STDIN, &len, 1);
                if(ret < 0) {
                    perror("receiving length of incoming packet failed");
                    return 1;
                }
                got = 0;
            }
            while(got < len) {
                // readout the received packet
                ret = read(STDIN, (void*)(buffer+got), len-got);
                if(ret < 0) {
                    if(errno == EAGAIN) { // we need to wait for more data
                        break;
                    }
                    perror("receiving incoming data failed");
                    return 1;
                }
                got += ret;
            }
            if(got < len) {
                continue;
            }
            if(meta){
                // this is meta data message from simualtor,
                // parse for configuration updates
                if(ret = Layer1.parse_metadata(buffer, len)){
                    return ret;
                }
            }
            else{
                // this is a normal packet, write to LL2 buffer
                LL2.writePacket((uint8_t*)buffer, len);
                #ifdef DEBUG
                Serial.printf("Layer1::receive(): buffer = ");
                for(int i = 0; i < len; i++){
                  Serial.printf("%c", buffer[i]);
                }
                Serial.printf("\r\n");
                #endif
            }
            meta = 0;
            len = 0;
        }
        // Use gettimeofday to find elasped time in milliseconds
        gettimeofday(&end, NULL);
        long seconds = (end.tv_sec - start.tv_sec);
        long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
        Layer1.setTime((int)(micros/1000));
        // call main loop
        if(ret = loop()) {
          return ret;
        }
    }
  return 0;
}
#endif
