
#include <unistd.h>
#include <stdint.h>

#define STDIN 0
#define STDOUT 1
#define DEBUG 0
#define SHA1_LENGTH 40
#define ADDR_LENGTH 6 

#define LORA // to use Layer1_LoRa.cpp
//#define SIM // to use Layer1_Sim/cpp

#ifdef LORA
#include <Arduino.h>

// for solar-powered module use these settings:
/*
#define LORA_DEFAULT_CS_PIN        2
#define LORA_DEFAULT_RESET_PIN     5
#define LORA_DEFAULT_DIO0_PIN      16
*/

#define LORA_DEFAULT_CS_PIN        18
#define LORA_DEFAULT_RESET_PIN     23
#define LORA_DEFAULT_DIO0_PIN      26

class Layer1Class {
public:
    Layer1Class();
    int debug_printf(const char* format, ...);
    int setLocalAddress(char* macString);
    uint8_t* localAddress();
    int getTime();
    int loraInitialized();
    int loraCSPin();
    int resetPin();
    int DIOPin();
    int init();
    int send_packet(char* data, int len);

private:
    uint8_t hex_digit(char ch);
    int isHashNew(char incoming[SHA1_LENGTH]);
    static void onReceive(int packetSize);

private:
    uint8_t _localAddress[ADDR_LENGTH];
    uint8_t _hashTable[256][SHA1_LENGTH];
    int _hashEntry;
    int _loraInitialized;
    int _csPin;
    int _resetPin;
    int _DIOPin;
};

extern Layer1Class Layer1;

#endif

#ifdef SIM
typedef struct _serial {
  int (*printf)(const char*, ...);
} serial;

extern serial Serial;

// public functions
int nsleep(unsigned int secs, useconds_t usecs);
//int setTimeDistortion(float newDistortion);
//float timeDistortion();
int simulationTime(int realTime);
int getTime();
int isHashNew(uint8_t hash[SHA1_LENGTH]);
int debug_printf(const char* format, ...);
int print_err(const char* format, ...);
int setLocalAddress(char* macString);
uint8_t* localAddress();
int begin_packet();
int send_packet(char* data, uint8_t len);

// you must declare these in your router
int setup(); // called once on startup
int loop(); // called once per event loop iteration
//int packet_received(char* data, size_t len); // called when a packet is received
#endif
