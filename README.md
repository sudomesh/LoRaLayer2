Layer 2 routing protocol for LoRa connected devices 

This library is a general purpose, minimal routing protocol. It is intended for use with https://github.com/sudomesh/disaster-radio and was designed using https://github.com/sudomesh/disaster-radio-simulator.

For documentation on the technical details of the LoRaLayer2 protocol, visit the disaster-radio wiki [Protocol page](https://github.com/sudomesh/disaster-radio/wiki/Protocol).

## Usage

To use this library, you will need to clone this repository, include the src files in your C project and compile them using gcc. An example routing sketch and Makefile can be found in the `examples` directory.

(I will publish to arduino once I have a chance to test that dev setup and tag a release) 

## Description of library files

`Layer1.h` contains the public functions for the hardware layer 1 and the simulated layer 1. The change of a single variable to swap which Layer 1 code they is being used. Declaring `#define LORA` will enable Layer1_LoRa, while `#define SIM` will enable Layer1_Sim. Theoretically, additional Layer 1 options could be added as they are developed.

`Layer1_LoRa.cpp` takes features from the real disaster.radio firmware. Acts as the connection between the physical layer LoRa transceiver and the Layer 2 routing logic. Heavily dependent on https://github.com/sandeepmistry/arduino-LoRa 

`Layer1_Sim.cpp` acts as the connection between simulated Layer1 found in https://github.com/sudomesh/disaster-radio-simulator and the Layer 2 routing logic.

`LoRaLayer2.cpp` and `LoRaLayer2.h` (previously known as routing.cpp and routing.h) contain the routing logic and manage the routing tables for the main sketch. 

## API

This library consists of two closely related classes. The Layer1 class and the LoRaLayer2 (or LL2) class. The APIs for interacting with these classes are as follows.

### Layer1

Prior to initializing your Layer1 interface, you can choose to set the following values,  
```
Layer1.setPins(int cs, int reset, int dio);
```
Corresponds to the GPIO pins of your ESP32 board that are conencted to chip select, reset and DIO0 pins of the LoRa transceiver. Defaults to cs = 18, reset = 23, dio = 26.  

```
Layer1.setSPIFrequency(uint32_t frequency);
```
Sets the frequency of the SPI bus connected to the LoRa transceiver. Defaults to 100E3.  

```
Layer1.setLoRaFrequency(uint32_t frequency);
```
Sets the frequency at which the LoRa transceiver transmits. Typically 915E6 for NA/SA/AU/NZ or 866E6 for EU, 433E6 is also an option. Defaults to 915E6.   

```
Layer1.setSpreadingFactor(uint8_t spreadingFactor);
```
Sets the spreading factor for the LoRa transceiver. Defaults to 9.  

```
Layer1.setTxPower(int txPower);
```
Sets the transmit power for the LoRa transceiver. Defaults to 17.  


To initialize your Layer 1 interface,  
```
Layer1.init();
```

Get the current time on your Layer 1 device as this may change from device to device (to simulator), 
```
int time = Layer1.getTime()
```
returns in milliseconds.

Print to serial, only if `DEBUG` is enabled,
```
Layer1.debug_printf(const char* format, ...) 
```

Send a packet using Layer 1 interface. This will bypass LoRaLayer2 buffers and immeadiately transmit the packet. It should only be used if you know what you are doing,
```
Layer1.send_packet(char* data, int len)
```

## LoRaLayer2 (LL2)

Intialize LoRaLayer2,
```
LL2.init()
```

Set interval between broadcasts of routing packets, set to 0 to turn off routing packet broadcasts. Takes interval in milliseconds, defaults to 15s if not called.
```
LL2.setInterval(long interval)
```

Check in with the LL2 protocol to see if any packets have been received or if any packets need to be sent out. This should be called once inside of your `loop()`. It acts as a psuedo-asynchronous method for monitoring your packet buffers.
```
LL2.daemon()
```
returns `1` if a packet has been received and is read out of the incoming packet buffer
returns `0` if incoming packet buffer is empty.

Send a packet to LL2 to be added to outgoing packet buffer and eventually, but usually immeadiately, transmitted over Layer1 interface.
```
LL2.sendToLayer2(uint8_t dest[6], uint8_t type, uint8_t data[240], uint8_t dataLength)
```

Get the current message count, i.e. the number of messages sent by the device since last boot,
```
uint8_t count = LL2.messageCount()
```

Manually set the local address of your node,
```
LL2.setLocalAddress(char* macString)
```

Retreive the current local address of your node,
```
unint8_t* mac = LL2.localAddress()
```

Get the lastest packet meant for Layer3 from its LL2 buffer, 
```
struct Packet packet = LL2.popFromL3OutBuffer()
```
returns entire LL2 packet

## License and copyright
* Copyright 2019 Sudo Mesh 
* All files in this repository are dual-licensed under both GPLv3 and AGPLv3
