#include <LoRa.h>
#include <Layer1.h>
#include <LoRaLayer2.h>
#ifdef LORA 


Layer1Class::Layer1Class() :

    _localAddress(),
    _hashTable(),
    _hashEntry(0),
    _loraInitialized(0),

    _csPin(LORA_DEFAULT_CS_PIN),
    _resetPin(LORA_DEFAULT_RESET_PIN),
    _DIOPin(LORA_DEFAULT_DIO0_PIN)

{

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

uint8_t Layer1Class::hex_digit(char ch){
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

int Layer1Class::setLocalAddress(char* macString){
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

uint8_t* Layer1Class::localAddress(){
    return _localAddress;
}

int Layer1Class::getTime(){
    return millis();
}

int Layer1Class::isHashNew(char incoming[SHA1_LENGTH]){
    int hashNew = 1;
    for( int i = 0 ; i <= _hashEntry ; i++){
        if(strcmp(incoming, (char*) _hashTable[i]) == 0){
            hashNew = 0; 
        }
    }
    if( hashNew ){
        Serial.printf("New message received");
        Serial.printf("\r\n");
        for( int i = 0 ; i < SHA1_LENGTH ; i++){
            _hashTable[_hashEntry][i] = incoming[i];
        }
        _hashEntry++;
    }
    return hashNew;
}

void Layer1Class::onReceive(int packetSize) {

    if (packetSize == 0) return;          // if there's no packet, return
    char incoming[PACKET_LENGTH];                 // payload of packet
    int incomingLength = 0;
    //Serial.printf("Receiving: ");
    while (LoRa.available()) { 
        incoming[incomingLength] = (char)LoRa.read(); 
        //Serial.printf("%02x", incoming[incomingLength]);
        incomingLength++;
    }
    //Serial.printf("\r\n");
    LL2.packetReceived(incoming, incomingLength);
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

int Layer1Class::init(){ // maybe this should take the pins and spreading factor as inputs?

    pinMode(_csPin, OUTPUT);
    pinMode(_DIOPin, INPUT);

    LoRa.setPins(_csPin, _resetPin, _DIOPin); // set CS, reset, DIO pin

    if (!LoRa.begin(915E6)) {             // initialize ratio at 915 MHz
        Serial.printf("LoRa init failed. Check your connections.\r\n");
        return _loraInitialized;
    }

    LoRa.setSPIFrequency(100E3);
    LoRa.setSpreadingFactor(9);           // ranges from 6-12,default 7 see API docs
    LoRa.onReceive(onReceive);
    LoRa.receive();

    Serial.printf("LoRa init succeeded.\r\n");
    _loraInitialized = 1;
    return _loraInitialized;
}

int Layer1Class::send_packet(char* data, int len){

    //Serial.printf("Sending: ");
    if(LoRa.beginPacket()){
        for( int i = 0 ; i < len ; i++){
            LoRa.write(data[i]);
            //Serial.printf("%02x", data[i]);
        }
        //Serial.printf("\r\n");
        LoRa.endPacket(1);
        LoRa.receive();
    }
}

Layer1Class Layer1;

#endif
