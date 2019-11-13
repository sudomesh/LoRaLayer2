#include <LoRa.h>
#include <Layer1.h>
#include <LoRaLayer2.h>
#ifdef LORA 
uint8_t _localAddress[ADDR_LENGTH];
uint8_t hashTable[256][SHA1_LENGTH];
uint8_t hashEntry = 0;
int _loraInitialized = 0;

// for portable node (esp32 TTGO v1.6 - see also below) use these settings:
const int _loraCSPin = 18; // LoRa radio chip select, GPIO15 = D8 on WeMos D1 mini
const int _resetPin = 23;       // LoRa radio reset, GPIO0 = D3 
const int _DIOPin = 26;        // interrupt pin for receive callback?, GPIO2 = D4

// for solar-powered module use these settings:
/*
const int csPin = 2;          // LoRa radio chip select, GPIO2
const int resetPin = 5;       // LoRa radio reset (hooked to LED, unused)
const int DIOPin = 16;        // interrupt pin for receive callback?, GPIO16
*/


int debug_printf(const char* format, ...) {

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

uint8_t hex_digit(char ch){
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

int setLocalAddress(char* macString){
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

uint8_t* localAddress(){
    return _localAddress;
}

int getTime(){
    return millis();
}

int isHashNew(char incoming[SHA1_LENGTH]){
    int hashNew = 1;
    for( int i = 0 ; i <= hashEntry ; i++){
        if(strcmp(incoming, (char*) hashTable[i]) == 0){
            hashNew = 0; 
        }
    }
    if( hashNew ){
        Serial.printf("New message received");
        Serial.printf("\r\n");
        for( int i = 0 ; i < SHA1_LENGTH ; i++){
            hashTable[hashEntry][i] = incoming[i];
        }
        hashEntry++;
    }
    return hashNew;
}

void onReceive(int packetSize) {

    if (packetSize == 0) return;          // if there's no packet, return
    char incoming[PACKET_LENGTH];                 // payload of packet
    int incomingLength = 0;
    while (LoRa.available()) { 
        incoming[incomingLength] = (char)LoRa.read(); 
        incomingLength++;
    }
    packet_received(incoming, incomingLength);
}

int loraInitialized(){
    return _loraInitialized;
}

int loraCSPin(){ 
    return _loraCSPin;
}

int resetPin(){ 
    return _resetPin;
}

int DIOPin(){
    return _DIOPin;
}

int loraSetup(){ // maybe this should take the pins and spreading factor as inputs?

    LoRa.setPins(_loraCSPin, _resetPin, _DIOPin); // set CS, reset, DIO pin

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

int send_packet(char* data, int len){

    Serial.printf("Sending: ");
    if(LoRa.beginPacket()){
        for( int i = 0 ; i < len ; i++){
            LoRa.write(data[i]);
            Serial.printf("%02x", data[i]);
        }
        Serial.printf("\r\n");
        LoRa.endPacket(1);
        LoRa.receive();
    }
}
#endif
