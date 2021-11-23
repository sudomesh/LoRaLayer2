#ifdef RL_SX1262
#include <Layer1_SX126x.h>
Layer1Class::Layer1Class(SX1262 *lora, int mode, int cs, int reset, int dio, uint8_t sf, uint32_t frequency, int power)
: _LoRa(lora),
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
  _syncWord(SX126X_SYNC_WORD_PRIVATE), 
  _currentLimit(60),
  _preambleLength(8),
  _txcoVoltage(0),
  _useRegulatorLDO(false)
{
  txBuffer = new packetBuffer();
  rxBuffer = new packetBuffer();
};
bool _dioFlag = false;
bool _transmitFlag = false;
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
    #ifdef LL2_DEBUG
    Serial.printf("Layer1::sendPacket(): data = ");
    for(int i = 0; i < len; i++){
      Serial.printf("%c", data[i]);
    }
    Serial.printf("\r\n");
    #endif
    //data[len] = '\0';
    int state = _LoRa->startTransmit((uint8_t*)data, len);
    #ifdef LL2_DEBUG
    Serial.printf("Layer1::sendPacket(): state = %d\r\n", state);
    #endif
    if (state == ERR_NONE) {
      _transmitFlag = true;
    }
    else if (state == ERR_PACKET_TOO_LONG) {
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
    _dioFlag = true;
}

/*Main public functions
*/
// Initialization
int Layer1Class::init(){

    int state = _LoRa->begin(_loraFrequency, _bandwidth, _spreadingFactor, _codingRate, SX126X_SYNC_WORD_PRIVATE, _txPower, _currentLimit, _preambleLength, _txcoVoltage, _useRegulatorLDO); 
    #ifdef LL2_DEBUG
    Serial.printf("Layer1::init(): state = %d\r\n", state);
    #endif
    if (state != ERR_NONE) {
      return _loraInitialized;
    }

    _LoRa->setDio1Action(Layer1Class::setFlag);

    state = _LoRa->startReceive();
    if (state != ERR_NONE) {
      return _loraInitialized;
    }

    _loraInitialized = 1;
    return _loraInitialized;
}

// Transmit polling function
int Layer1Class::transmit(){
    BufferEntry entry = txBuffer->read();
    if(entry.length > 0){
        Serial.printf("Layer1::transmit(): entry.length: %d\r\n", entry.length);
        sendPacket(entry.data, entry.length);
    }
    return entry.length;
}

// Receive polling function
int Layer1Class::receive(){
    int ret = 0; 
    if(_dioFlag) {
        if(_transmitFlag){
            // interrupt caused by transmit, clear flags and return 0
            #ifdef LL2_DEBUG
            Serial.printf("Layer1::receive(): transmit complete\r\n");
            #endif
            _transmitFlag = false;
            _dioFlag = false;
            _LoRa->startReceive();
        }
        else{
            // interrupt caused by reception
            _enableInterrupt = false;
            _dioFlag = false;
            size_t len =_LoRa->getPacketLength();
            byte data[len];
            int state = _LoRa->readData(data, len);
            if (state == ERR_NONE) {
                BufferEntry entry;
                memcpy(&entry.data[0], &data[0], len); // copy data to buffer, excluding null terminator
                entry.length = len;
                rxBuffer->write(entry);
                #ifdef LL2_DEBUG
                Serial.printf("Layer1::receive(): data = ");
                for(int i = 0; i < len; i++){
                    Serial.printf("%c", data[i]);
                }
                Serial.printf("\r\n");
                #endif
                ret = len;
            }else if(state == ERR_CRC_MISMATCH) {
                // packet was received, but is malformed
                ret=-1;
            }
            else{
                // some other error occurred
                ret=-2;
            }
            _LoRa->startReceive();
            _enableInterrupt = true;
        }
    }
    return ret;
}
#endif
