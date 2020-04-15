#include <LoRa.h>
#include <Layer1.h>
#include <LoRaLayer2.h>
#ifdef LORA 

Layer1Class::Layer1Class()
{
    _loraInitialized = 0;
    _csPin = L1_DEFAULT_CS_PIN;
    _resetPin = L1_DEFAULT_RESET_PIN;
    _DIOPin = L1_DEFAULT_DIO0_PIN;
    _spiFrequency = 100E3;
    _loraFrequency = 915E6;
    _spreadingFactor = 9;
    _txPower = 17;
}

int Layer1Class::debug_printf(const char* format, ...) {

    if(DEBUG){
        int ret;
        va_list args;
        va_start(args, format);
        ret = vfprintf(stderr, format, args);
        va_end(args);
        fflush(stderr);
        return ret;
    }else{
        return 0;
    }
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

/* Receive packet callback
*/
void Layer1Class::onReceive(int packetSize) {
    if (packetSize == 0) return; // if there's no packet, return
    char incoming[PACKET_LENGTH];
    int length = 0;
    while (LoRa.available()) {
        incoming[length] = (char)LoRa.read();
        length++;
    }
    uint8_t* data = ( uint8_t* ) incoming;
    LL2.writePacket(data, length);
    return;
}

/* Initialization
*/
int Layer1Class::init(){
    LoRa.setPins(_csPin, _resetPin, _DIOPin); // set CS, reset, DIO pin
    LoRa.setSPIFrequency(_spiFrequency);
    LoRa.setTxPower(_txPower);

    if (!LoRa.begin(_loraFrequency)) { // defaults to 915MHz, can also be 433MHz or 868Mhz
        return _loraInitialized;
    }

    LoRa.setSpreadingFactor(_spreadingFactor); // ranges from 6-12, default 9
    LoRa.onReceive(onReceive);
    LoRa.receive();

    _loraInitialized = 1;
    return _loraInitialized;
}

/* Send/transmit data
*/
int Layer1Class::sendPacket(char* data, int len){
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

int Layer1Class::transmit(){
    int ret = -1;
    Packet packet = LL2.readPacket();
    if(packet.totalLength != 0){
        if(sendPacket((char*)&packet, packet.totalLength)){
          ret = packet.sequence;
        }
    }
    return ret;
}

Layer1Class Layer1;

#endif
