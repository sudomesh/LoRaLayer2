#ifdef ARDUINO_LORA
#include <LoRa.h>
#include <Layer1_LoRa.h>
#include <LoRaLayer2.h>

Layer1Class::Layer1Class()
: _csPin(LORA_CS),
  _resetPin(LORA_RST),
  _DIOPin(LORA_IRQ),
  _spreadingFactor(9),
  _loraFrequency(915E6),
  _txPower(17),
  _loraInitialized(0),
  _spiFrequency(100E3),
{}

bool _receivedFlag = false;
bool _enableInterrupt = true;

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
}

void Layer1Class::setTxPower(int txPower){
    _txPower = txPower;
}

/* Private functions
*/
// Send packet function
int Layer1Class::sendPacket(char* data, size_t len){
    int ret = 0;
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
    _receivedFlag = true;
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
    char *data = txBuffer.read();
    size_t len = (size_t)data[1]; //this is a small hack to get the packet length by inspecting the byte where it is store in the packet
    if(len != 0){
        sendPacket(data, len);
    }
    return len;
}

// Receive polling function
int Layer1Class::receive(){
    int ret = 0; 
    if(_receivedFlag) {
        _enableInterrupt = false;
        _receivedFlag = false;

        if (_packetSize != 0){
            char data[PACKET_LENGTH];
            int len = 0;
            while (LoRa.available()) {
                data[len] = (char)LoRa.read();
                len++;
            }
            rxBuffer.write(data, len);
            ret = _packetSize;
        }
        // reset packetSize to zero and renable interrupt
        _packetSize = 0;
        _enableInterrupt = true;
    }
    return ret;
}
#endif
