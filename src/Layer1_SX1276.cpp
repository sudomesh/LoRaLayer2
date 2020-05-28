#ifdef RL_SX1276
#include <Layer1_SX1276.h>
Layer1Class::Layer1Class(SX1276 *lora, int mode, int cs, int reset, int dio, uint8_t sf, uint32_t frequency, int power) 
: _LoRa{lora}, 
  _mode(mode),
  _csPin(cs),
  _resetPin(reset),
  _DIOPin(dio),
  _spreadingFactor(sf),
  _loraFrequency(frequency),
  _txPower(power),
  _loraInitialized(0),
  _spiFrequency(100E3),
  _bandwidth(125.0), 
  _codingRate(5),
  _syncWord(SX127X_SYNC_WORD), 
  _currentLimit(100),
  _preambleLength(8),
  _gain(0){};

bool _receivedFlag = false;
bool _enableInterrupt = true;

/* Public access to local variables
 */
int Layer1Class::getTime(){
    return millis();
}

int Layer1Class::spreadingFactor(){
    return _spreadingFactor;
}

/* Private functions
*/
// Send packet function
int Layer1Class::sendPacket(char* data, size_t len){
    int ret = 0;
    int state = _LoRa->transmit((byte*)data, len);
    if (state == ERR_PACKET_TOO_LONG) {
      // packet longer than 256 bytes
      ret = 1;
    } else if (state == ERR_TX_TIMEOUT) {
      // timeout occurred while transmitting packet
      ret = 2;
    } else {
      // some other error occurred
      ret = 3;
  }
  return ret;
}

// Receive packet callback
void Layer1Class::setFlag(void) {
    // check if the interrupt is enabled
    if(!_enableInterrupt) {
        return;
    }
    // we got a packet, set the flag
    _receivedFlag = true;
}

/*Main public functions
*/
// Initialization
int Layer1Class::init(){

    int state = _LoRa->begin(_loraFrequency, _bandwidth, _spreadingFactor, _codingRate, SX127X_SYNC_WORD, _txPower, _currentLimit, _preambleLength, _gain); 
    if (state != ERR_NONE) {
      return _loraInitialized;
    }

    _LoRa->setDio0Action(Layer1Class::setFlag);

    state = _LoRa->startReceive();
    if (state != ERR_NONE) {
      return _loraInitialized;
    }

    _loraInitialized = 1;
    return _loraInitialized;
}

// Transmit polling function
int Layer1Class::transmit(){
    BufferEntry entry = txBuffer.read();
    if(entry.length != 0){
        sendPacket(entry.data, entry.length);
    }
    return entry.length;
}

// Receive polling function
int Layer1Class::receive(){
    int ret = 0; 
    if(_receivedFlag) {
        _enableInterrupt = false;
        _receivedFlag = false;
        size_t len =_LoRa->getPacketLength();
        byte data[len];
        int state = _LoRa->readData(data, len);
        if (state == ERR_NONE) {
          BufferEntry entry;
          memcpy(&data, &entry, len);
          entry.length = len;
          rxBuffer.write(entry);
        } else if (state == ERR_CRC_MISMATCH) {
            // packet was received, but is malformed
            ret=1;
        } else {
            // some other error occurred
            ret=2;
        }
        _LoRa->startReceive();
        _enableInterrupt = true;
    }
    return ret;
}
#endif
