Layer 2 routing protocol for LoRa connected devices 

This library is a general purpose, minimal routing protocol. It is intended for use with https://github.com/sudomesh/disaster-radio and was designed using https://github.com/sudomesh/disaster-radio-simulator.

For documentation on the technical details of the LoRaLayer2 protocol, visit the disaster-radio wiki [Protocol page](https://github.com/sudomesh/disaster-radio/wiki/Protocol).

## Installation

### Using the Arduino IDE Library Manager
* Choose `Sketch` -> `Include Library' -> 'Manage Libraries...'
* Type `LoRaLayer2` into the search box.
* Click the row to select the library.
* Click the `Install` button to install the library.

### Using Git
```
cd ~/Documents/Arduino/libraries/
git clone https://github.com/sudomesh/LoRaLayer2
```

### Using PlatformIO
Include the following line in your `platformio.ini` file,
```
lib_deps = LoRaLayer2
```
or
```
lib_deps = https://github.com/sudomesh/LoRaLayer2
```
You can also specify a certain release or commit in either of these options like so,
```
lib_deps = LoRaLayer2@1.0.1
```
or
```
lib_deps = https://github.com/sudomesh/LoRaLayer2#5fc98c99706b7883b49ece82d9a7abc1b35e0bae
```

## Description of library files

`Layer1_LoRa.cpp` - connection between the physical layer LoRa transceiver and the Layer 2 routing logic. Is little more than a wrapper around https://github.com/sandeepmistry/arduino-LoRa. Should work with all transceivers supported by arudion-LoRa.  

`Layer1_SX1276.cpp` - connection between a physical layer SX1276 LoRa transceiver and the Layer 2 routing logic. Is little more than a wrapper around https://github.com/jgromes/RadioLib. Only works with SX1276 transceivers, should be easy to add additional wrappers for other RadioLib supported transceivers. Is required for dual-lora module support.

`Layer1_Sim.cpp` - connection between simulated Layer1 found in https://github.com/sudomesh/disaster-radio-simulator and the Layer 2 routing logic.  

`LoRaLayer2.cpp` - routing logic, management the routing tables, interfaces with main sketch or Layer3 applications.  

`packetBuffer.cpp`  - 16 entry circular FIFO buffer, read/write logic, used internally to communicate between Layer1 and LL2, as well as between LL2 and the main sketch or Layer3 application.

## API

This library consists of two closely related classes. The Layer1 class and the LoRaLayer2 (or LL2) class. See the most basic example of their usage in [examples/router_beacon](https://github.com/sudomesh/LoRaLayer2/tree/master/examples/router_beacon).

To build this library, you must define one of the following constants depending on your target system,

```
#define ARDUINO_LORA    // to use arduino-LoRa compatible transceivers
#define RL_SX1276       // to use RadioLib with an SX1276 transceiver
#define SIM             // to use disaster-radio-simulator 
```

These may also be passed as build flags when compiling.

### Creating Layer1 and LL2 objects

To create the Layer1Class and LL2Class objects,

```
Layer1Class *Layer1 = new Layer1Class();
LL2Class *LL2 = new LL2Class(Layer1);
```

### Layer1

Prior to initializing the Layer1 object, there are a number of setting you can optionally set.

#### LoRa pins

Override the default `CS`, `RESET`, and `DIO0` pins used by the library.
```
Layer1->setPins(int cs, int reset, int dio);
```
 * `cs` - chip select pin to use, defaults to `18`
 * `reset` - reset pin to use, defaults to `23`
 * `dio0` - DIO0 pin to use, defaults to `26`.  **Must** be interrupt capable via [attachInterrupt(...)](https://www.arduino.cc/en/Reference/AttachInterrupt).

This function is optional and only needs to be used if you need to change the default pins. If you choose you use it, it must be called before `Layer1->init()`.

#### SPI Frequency

Set the frequency of the SPI bus connected to the LoRa transceiver.
```
Layer1->setSPIFrequency(uint32_t frequency);
```
 * `frequency` - SPI frequency to use, defaults to `100E3`.

#### LoRa frequency

Set the frequency at which the LoRa transceiver transmits
```
Layer1->setLoRaFrequency(uint32_t frequency);
```
 * `frequency` - LoRa frequency to use, defaults to `915E6`.

Typically 915E6 for NA/SA/AU/NZ or 866E6 for EU, 433E6 is also an option.

#### LoRa spreading factor

Set the spreading factor for the LoRa transceiver.
```
Layer1->setSpreadingFactor(uint8_t spreadingFactor);
```
 * `spreadingFactor` - LoRa modulation setting, "the duration of the chirp" [reference](https://docs.exploratory.engineering/lora/dr_sf/), defaults to `9`

#### LoRa transmit power

Set the transmit power for the LoRa transceiver
```
Layer1->setTxPower(int txPower);
```
 * `txPower` - TX power in dB, defaults to `17`

#### Initialize
To initialize your Layer 1 interface,  
```
Layer1->init();
```

#### Other Layer 1 features

There are few public funtions that are used by LoRaLayer2 and should not need to be called from the main sketch, 

Check outgoing packet buffer and transmit if packet is available,
```
int ret = Layer1->transmit();
```
 * returns the sequence number of the packet transmitted, if no packet is transmitted, returns `-1`

Get the current time on your Layer 1 device as this may change from device to device (to simulator), 
```
int time = Layer1->getTime()
```
* returns time in milliseconds.

Send a packet using Layer 1 interface. This will bypass LoRaLayer2 buffers and immeadiately transmit the packet. It should only be used if you know what you are doing,
```
Layer1->sendPacket(char* data, int len)
```

## LoRaLayer2 (LL2)

#### Set node address

Manually set the local address of your node, should be run before initializing,
```
LL2->setLocalAddress(char* macString)
```
 * `macString` - a char array containing the node's address in string form

#### Initialize

Intialize LoRaLayer2,
```
LL2->init()
```

#### Set broadcast interval

At any point during operation, you can set the interval between broadcasts of routing packets. To turn off routing packet broadcasts, set the interval to 0. 
```
LL2->setInterval(long interval)
```

 * `interval` - in milliseconds, defaults to 1500ms if not called.

#### Routing daemon

Check in with the LL2 protocol to see if any packets have been received or if any packets need to be sent out. This should be called once inside of your `loop()`. It is non-blocking and acts as a psuedo-asynchronous method for monitoring your packet buffers.
```
LL2->daemon()
```
 * returns `1` if a packet has been received and is read out of the incoming packet buffer
 * returns `0` if incoming packet buffer is empty.

#### Sending datagrams

Send a datagram to LL2. The datagram will be inspected for a destination, will be given a header with the apporiate next hop and then will be added to outgoing packet buffer and eventually transmitted over the Layer1 interface.
```
int ret = LL2->writeData(Datagram data, size_t length)
```
 * `data` - is a datagram, as outlined on [our wiki](https://github.com/sudomesh/disaster-radio/wiki/Layered-Model#layer-3)
 * `len` - length of the entire datagram, typically header length + message length
 * returns `int` representing the packet's place in outgoing buffer

#### Receiving datagrams

To receive the latest datagram, you must pop the latest packet meant for Layer3 from its LL2 buffer and then extract the datagram
```
struct Packet packet = LL2->readData()
```
 * returns entire LL2 packet, if there is one available
 * returns 0, if there are no packets in the buffer

The datagram contained in the returned packet can be then be accessed at `packet.datagram`.

#### Other LL2 features

Get current message count,
```
uint8_t count = LL2->messageCount()
```
 * returnx the number of messages sent by the device since last boot

Get current count of routes,
```
int routes = LL2->getRouteEntry()
```
 * returns the entry to the routing table at which the next route will be added, which corresponds to a count of discovered routes

Retreive the current local address of your node,
```
unint8_t* mac = LL2->localAddress()
```
 * returns a pointer to byte array containing local address of your node

Retreive the loopback address of your node,
```
unint8_t* mac = LL2->loopbackAddr()
```
 * returns a pointer to byte array containing {0x00, 0x00, 0x00, 0x00}

Retreive the broadcast address of your node,
```
unint8_t* mac = LL2->broadcastAddr()
```
 * returns a pointer to byte array containing {0xff, 0xff, 0xff, 0xff}

Retreive the broadcast address of your node,
```
unint8_t* mac = LL2->routingAddr()
```
 * returns a pointer to byte array containing {0xaf, 0xff, 0xff, 0xff}

Get neighbor table information,
```
LL2->getNeighborTable(char *out);
```
 * `out` - a pointer to char array where neighbor table information can be written

Get routing table information,
```
LL2->getRoutingTable(char *out);
```
 * `out` - a pointer to char array where routing table information can be written

## License and copyright
* Copyright 2020 Sudo Mesh
* All files in this repository are dual-licensed under both GPLv3 and AGPLv3
