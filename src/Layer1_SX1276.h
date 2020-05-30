#ifndef LAYER1_H
#define LAYER1_H

#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>

#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>

#include <packetBuffer.h>

class Layer1Class {

public:
    Layer1Class(SX1276 *lora, int mode, int cs, int reset, int dio, uint8_t sf = 9, uint32_t frequency = 915, int power = 17); 
    
    // Public access to local variables
    static int getTime();
    int spreadingFactor();

    // Fifo buffers
    packetBuffer *txBuffer;
    packetBuffer *rxBuffer;

    // Main public functions
    int init();
    int transmit();
    int receive();

private:
    // Main private functions
    static void setFlag(void);
    int sendPacket(char *data, size_t len);

    // Local variables
    SX1276 *_LoRa;
    int _mode;
    int _csPin;
    int _resetPin;
    int _DIOPin;
    uint8_t _spreadingFactor;
    uint32_t _loraFrequency;
    int _txPower;
    int _loraInitialized;
    uint32_t _spiFrequency;
    float _bandwidth; 
    uint8_t _codingRate;
    uint8_t _syncWord; //SX127X_SYNC_WORD, 
    uint8_t _currentLimit;
    uint8_t _preambleLength;
    uint8_t _gain;
};

#endif
