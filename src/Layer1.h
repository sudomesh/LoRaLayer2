#ifndef LAYER1_H
#define LAYER1_H

#include <unistd.h> // maybe uncessasry?
#include <stdint.h>

#define STDIN 0
#define STDOUT 1
#define DEBUG 0
#define SHA1_LENGTH 40

#ifndef SIM  // define SIM to use Layer1_Sim.cpp
#define LORA // to use Layer1_LoRa.cpp
#endif

#ifdef LORA
#include <Arduino.h>
#include <SPI.h>

#define L1_DEFAULT_CS_PIN        18
#define L1_DEFAULT_RESET_PIN     23
#define L1_DEFAULT_DIO0_PIN      26

class Layer1Class {
public:
    Layer1Class();

    int debug_printf(const char* format, ...);

    // Public access to local variables
    int getTime();
    int loraInitialized();
    int loraCSPin();
    int resetPin();
    int DIOPin();
    int spreadingFactor();

    // User configurable settings
    void setPins(int cs, int reset, int dio);
    void setSPIFrequency(uint32_t frequency);
    void setLoRaFrequency(uint32_t frequency);
    void setSpreadingFactor(uint8_t spreadingFactor);
    void setTxPower(int txPower);

    // Main public functions
    int init();
    int transmit();

private:
    // Main private functions
    static void onReceive(int packetSize);
    int sendPacket(char* data, int len);

    // Local variables
    int _loraInitialized;
    int _csPin;
    int _resetPin;
    int _DIOPin;
    uint32_t _spiFrequency;
    uint32_t _loraFrequency;
    uint8_t _spreadingFactor;
    int _txPower;
};

extern Layer1Class Layer1;

#endif

#ifdef SIM
#include <stdio.h>
#include <string.h> // for memcmp and memset functions
#include <math.h>  // for ceil and pow functions
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>

typedef struct _serial {
  int (*printf)(const char*, ...);
} serial;

extern serial Serial;

// you must declare these in your router
int setup(); // called once on startup
int loop(); // called once per event loop iteration

class Layer1Class {
public:
    Layer1Class();
    int nsleep(unsigned int secs, useconds_t usecs);
    int simulationTime(int realTime);
    int setTimeDistortion(float newDistortion);
    int getTime();
    int spreadingFactor();
    int debug_printf(const char* format, ...);
    int setNodeID(char* newID);
    int parse_metadata(char* data, uint8_t len);
    int begin_packet();
    int transmit();

private:
    int sendPacket(char* data, uint8_t len);
    float timeDistortion();

    int _transmitting;
    char* _nodeID;
    float _timeDistortion;
    uint8_t _spreadingFactor;

};

extern Layer1Class Layer1;
#endif
#endif
