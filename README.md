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


Note from simulator repo:
If you use `sleep()` or `usleep()` in `setup()` or `loop()` then this will block the event loop which means that your `packet_received()` function will not be called until after the sleep is over. If you want to avoid this then you can instead call `nsleep(seconds, microseconds)` once inside `loop()` (calling it more than once overrides the previous calls) which will cause `loop()` to be called again the specified amount of time after completion, without blocking packet reception while "sleeping".

## License and copyright
* Copyright 2019 Sudo Mesh 
* All files in this repository are dual-licensed under both GPLv3 and AGPLv3
