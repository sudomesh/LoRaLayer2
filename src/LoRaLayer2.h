#ifndef LORALAYER2_H
#define LORALAYER2_H

#include <unistd.h>
#include <stdint.h>

//#define DEBUG 0
#define HEADER_LENGTH 17
#define PACKET_LENGTH 256
#define DATA_LENGTH 241
#define MESSAGE_LENGTH 233
#define SHA1_LENGTH 40
#define ADDR_LENGTH 4
#define MAX_ROUTES_PER_PACKET 40 
#define ASYNC_TX 1
#define DEFAULT_TTL 30
#define BUFFERSIZE 16

struct Datagram {
    uint8_t destination[ADDR_LENGTH];
    uint8_t type;
    uint8_t message[MESSAGE_LENGTH];
};

struct Packet {
    uint8_t ttl;
    uint8_t totalLength;
    uint8_t sender[ADDR_LENGTH];
    uint8_t receiver[ADDR_LENGTH];
    uint8_t sequence; // message count of packets tx by sender
    uint8_t source[ADDR_LENGTH];
    uint8_t hopCount; // start 0, incremented with each retransmit
    uint8_t metric; // of source-receiver link
    Datagram datagram;
};

struct NeighborTableEntry{
    uint8_t address[ADDR_LENGTH];
    uint8_t lastReceived;
    uint8_t packet_success;
    uint8_t metric;
};

struct RoutingTableEntry{
    uint8_t destination[ADDR_LENGTH];
    uint8_t nextHop[ADDR_LENGTH];
    uint8_t distance;
    uint8_t lastReceived;
    uint8_t metric;
};

class packetBuffer {
  public:
    Packet read();
    int write(Packet packet);
    packetBuffer();
  private:
    Packet buffer[BUFFERSIZE];
    int head;
    int tail;
};

class LL2Class {
public:
    // Public access to local variables
    uint8_t messageCount();
    uint8_t* localAddress();
    uint8_t* broadcastAddr();
    uint8_t* loopbackAddr();
    uint8_t* routingAddr();
    int getRouteEntry();

    // User configurable settings
    int setLocalAddress(const char* macString);
    long setInterval(long interval);
    void setDutyCycle(double dutyCycle);

    // Wrappers for packetBuffers
    void writePacket(uint8_t* data, size_t length);
    Packet readPacket();
    int writeData(Datagram datagram, size_t length);
    Packet readData();

    // Print out functions
    void getNeighborTable(char *out);
    void getRoutingTable(char *out);
    void printPacketInfo(Packet packet);

    // Packet building functions
    Packet buildPacket(uint8_t ttl, uint8_t nextHop[ADDR_LENGTH], uint8_t source[ADDR_LENGTH], uint8_t hopCount, uint8_t metric, Datagram datagram, size_t length);
    //Packet buildRoutingPacket();

    // Main init and loop functions
    int init();
    int daemon();

    // Constructor
    LL2Class();

private:
    // General purpose utility functions
    uint8_t hexDigit(char ch);
    void setAddress(uint8_t* addr, const char* macString);

    // Routing utility functions
    uint8_t calculatePacketLoss(int entry, uint8_t sequence);
    uint8_t calculateMetric(int entry);
    int checkNeighborTable(NeighborTableEntry neighbor);
    int checkRoutingTable(RoutingTableEntry route);
    int updateNeighborTable(NeighborTableEntry neighbor, int entry);
    int updateRouteTable(RoutingTableEntry route, int entry);
    int selectRoute(uint8_t destination[ADDR_LENGTH]);
    int parseForNeighbor(Packet packet);

    // Main entry point functions
    void parseHeader(Packet packet);
    int route(uint8_t ttl, uint8_t source[ADDR_LENGTH], uint8_t hopCount, Datagram datagram, size_t length, int broadcast);
    void receive();

    // Fifo buffer objects
    packetBuffer L2toL1; // L2 sending to L1
    packetBuffer L1toL2; // L1 sending to L2
    packetBuffer L2toL3; // L2 sending to L3
    // NOTE: there is no L3toL2 buffer because I have not found a need for one yet

    // Local variables and tables
    uint8_t _localAddress[ADDR_LENGTH];
    uint8_t _loopbackAddr[ADDR_LENGTH];
    uint8_t _broadcastAddr[ADDR_LENGTH];
    uint8_t _routingAddr[ADDR_LENGTH];
    uint8_t _messageCount;
    float _packetSuccessWeight;
    NeighborTableEntry _neighborTable[255];
    int _neighborEntry;
    RoutingTableEntry _routeTable[255];
    int _routeEntry;
    long _startTime;
    long _lastRoutingTime;
    int _routingInterval;
    int _disableRoutingPackets;
    long _lastTransmitTime;
    int _dutyInterval;
    double _dutyCycle;
};

extern LL2Class LL2;
#endif
