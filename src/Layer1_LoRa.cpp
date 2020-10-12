#ifdef ARDUINO_LORA
#include <Layer1_LoRa.h>

Layer1Class::Layer1Class()
: _csPin(18),
  _resetPin(23),
  _DIOPin(26),
  _spreadingFactor(9),
  _loraFrequency(915E6),
  _txPower(17),
  _loraInitialized(0),
  _spiFrequency(100E3)
{
  txBuffer = new packetBuffer();
  rxBuffer = new packetBuffer();
};

bool _dioFlag = false;
bool _enableInterrupt = true;
int _packetSize = 0;

int Layer1Class::debug_printf(const char* format, ...) {

#ifdef DEBUG
        int ret;
        va_list args;
        va_start(args, format);
        ret = vfprintf(stderr, format, args);
        va_end(args);
        fflush(stderr);
        return ret;
#else
        return 0;
#endif
}

/* Public access to local variables
*/
int Layer1Class::getTime(){
    return millis();
}

int Layer1Class::loraInitialized(){
    return _loraInitialized;
}

int Layer1Class::loraCSPin(){
    return _csPin;
}

int Layer1Class::resetPin(){
    return _resetPin;
}

int Layer1Class::DIOPin(){
    return _DIOPin;
}

int Layer1Class::spreadingFactor(){
    return _spreadingFactor;
}

uint32_t Layer1Class::spiFrequency(){
    return _spiFrequency;
}

uint32_t Layer1Class::loraFrequency(){
    return _loraFrequency;
}

int Layer1Class::txPower(){
    return _txPower;
}

/* User configurable settings
*/
void Layer1Class::setPins(int cs, int reset, int dio){
    _csPin = cs;
    _resetPin = reset;
    _DIOPin = dio;
}

void Layer1Class::setSPIFrequency(uint32_t frequency){
    _spiFrequency = frequency;
}

void Layer1Class::setLoRaFrequency(uint32_t frequency){
    _loraFrequency = frequency;
}

void Layer1Class::setSpreadingFactor(uint8_t spreadingFactor){
    _spreadingFactor = spreadingFactor;
    LoRa.setSpreadingFactor(_spreadingFactor); // ranges from 6-12
}

void Layer1Class::setTxPower(int txPower){
    _txPower = txPower;
    LoRa.setTxPower(_txPower);
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

    if((ret = LoRa.beginPacket())){
        for( int i = 0 ; i < len ; i++){
            LoRa.write(data[i]);
        }
        LoRa.endPacket(1);
        LoRa.receive();
    }
    return ret;
}

// Receive packet callback
void Layer1Class::setFlag(int packetSize) {
    // check if the interrupt is enabled
    if(!_enableInterrupt) {
        return;
    }
    // we got a packet, set the flag
    _packetSize = packetSize;
    _dioFlag = true;
}

/*Main public functions
*/
// Initialization
int Layer1Class::init(){
    LoRa.setPins(_csPin, _resetPin, _DIOPin); // set CS, reset, DIO pin
    LoRa.setSPIFrequency(_spiFrequency);
    LoRa.setTxPower(_txPower);

    if (!LoRa.begin(_loraFrequency)) { // defaults to 915MHz, can also be 433MHz or 868Mhz
        return _loraInitialized;
    }

    LoRa.setSpreadingFactor(_spreadingFactor); // ranges from 6-12, default 9
    LoRa.onReceive(setFlag);
    LoRa.receive();

    _loraInitialized = 1;
    return _loraInitialized;
}

// Transmit polling function
int Layer1Class::transmit(){
    BufferEntry entry = txBuffer->read();
    if(entry.length != 0){
        sendPacket(entry.data, entry.length);
    }
    return entry.length;
}

// Receive polling function
int Layer1Class::receive(){
    int ret = 0; 
    if(_dioFlag) {
        #ifdef LL2_DEBUG
        Serial.printf("Layer1Class::receive(): _packetSize = %d\r\n", _packetSize);
        #endif
        _enableInterrupt = false;
        _dioFlag = false;
        if (_packetSize > 0){
            char data[MAX_PACKET_SIZE];
            int len = 0;
            while (LoRa.available()) {
                data[len] = (char)LoRa.read();
                len++;
            }
            BufferEntry entry;
            memcpy(&entry.data[0], &data[0], len);
            entry.length = len;
            rxBuffer->write(entry);

            #ifdef LL2_DEBUG
            Serial.printf("Layer1::receive(): data = ");
            for(int i = 0; i < len; i++){
              Serial.printf("%c", data[i]);
            }
            Serial.printf("\r\n");
            #endif

            ret = _packetSize;
        }
        // reset packetSize to zero and renable interrupt
        _packetSize = 0;
        _enableInterrupt = true;
    }
    return ret;
}
#endif
