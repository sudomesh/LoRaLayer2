#ifndef LAYER1_H
#define LAYER1_H

#include <stdint.h>

#define STDIN 0
#define STDOUT 1
#define SHA1_LENGTH 40

#include <stdio.h>
#include <string.h> // for memcmp and memset functions
#include <math.h>  // for ceil and pow functions

class Layer1Class {
public:
    Layer1Class();
    int simulationTime(int realTime);
    int setTimeDistortion(float newDistortion);
    int getTime();
    void setTime(int millis);
    int spreadingFactor();
    int setNodeID(int newID);
    int nodeID();

    int parse_metadata(char* data, uint8_t len);
    int begin_packet();
    int transmit();

private:
    int sendPacket(char* data, uint8_t len);
    float timeDistortion();

    int _transmitting;
    int _nodeID;
    float _timeDistortion;
    uint8_t _spreadingFactor;
    long _millis;

};

extern Layer1Class Layer1;
#endif
