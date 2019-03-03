#include <LoRa.h>
#include "Layer1_LoRa.h"
#include "LoRaLayer2.h"

uint8_t _localAddress[ADDR_LENGTH];
serial Serial;

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

