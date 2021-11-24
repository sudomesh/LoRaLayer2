#ifndef SIM
#ifndef RL_SX1276
#ifndef RL_SX1262
#define ARDUINO_LORA
#endif
#endif
#endif

#ifndef LAYER1_H
#define LAYER1_H

#include <stdint.h>
#include <Arduino.h>
#include <LoRa.h>
#include <SPI.h>
#include <packetBuffer.h>

class Layer1Class {
public:
    Layer1Class();

    int debug_printf(const char* format, ...);

    // Public access to local variables
    static int getTime();
    int loraInitialized();
    int loraCSPin();
    int resetPin();
    int DIOPin();
    int spreadingFactor();
    uint32_t spiFrequency();
    uint32_t loraFrequency();
    int txPower();

    // User configurable settings
    void setPins(int cs, int reset, int dio);
    void setSPIFrequency(uint32_t frequency);
    void setLoRaFrequency(uint32_t frequency);
    void setSpreadingFactor(uint8_t spreadingFactor);
    void setTxPower(int txPower);

    // Fifo buffers
    packetBuffer *txBuffer;
    packetBuffer *rxBuffer;

    // Main public functions
    int init();
    int transmit();
    int receive();

private:
    // Main private functions
    static void setFlag(int packetSize);
    int sendPacket(char *data, size_t len);

    // Local variables
    int _csPin;
    int _resetPin;
    int _DIOPin;
    uint8_t _spreadingFactor;
    uint32_t _loraFrequency;
    int _txPower;
    int _loraInitialized;
    uint32_t _spiFrequency;
};
#endif
