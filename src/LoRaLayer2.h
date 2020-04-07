#include <unistd.h>
#include <stdint.h>

//#define DEBUG 0
#define HEADER_LENGTH 11
#define PACKET_LENGTH 256
#define DATA_LENGTH 241
#define SHA1_LENGTH 40
#define ADDR_LENGTH 4
#define MAX_ROUTES_PER_PACKET 40 
#define ASYNC_TX 1;
#define DEFAULT_TTL 30;

struct Metadata {
    uint8_t rssi;
    uint8_t snr;
    uint8_t randomness;
};

struct Packet {
    uint8_t ttl;
    uint8_t totalLength;
    uint8_t source[ADDR_LENGTH];
    uint8_t nextHop[ADDR_LENGTH];
    uint8_t sequence;
    uint8_t data[DATA_LENGTH];
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

class LL2Class {
public:
    LL2Class();
    uint8_t messageCount();
    uint8_t* broadcastAddr();
    uint8_t* loopbackAddr();
    uint8_t* routingAddr();
    uint8_t hex_digit(char ch);
    int setLocalAddress(char* macString);
    uint8_t* localAddress();
    int pushToL1OutBuffer(struct Packet packet);
    int sendToLayer2(uint8_t data[DATA_LENGTH], uint8_t dataLength);
    struct Packet popFromL3OutBuffer();
    struct Packet buildPacket(uint8_t ttl, uint8_t next[ADDR_LENGTH], uint8_t data[DATA_LENGTH], uint8_t dataLength);
    void printAddress(uint8_t address[ADDR_LENGTH]);
    void printRoutingTable();
    void debug_printAddress(uint8_t address[ADDR_LENGTH]);
    int packetReceived(char* data, size_t len);
    int daemon();
    int getRouteEntry();
    long setInterval(long interval);
    int init();

private:
    int sendToLayer1(struct Packet packet);
    struct Packet popFromL1OutBuffer();
    int pushToL3OutBuffer(struct Packet packet);
    int pushToL1InBuffer(struct Packet packet);
    struct Packet popFromL1InBuffer();
    void printMetadata(struct Metadata metadata);
    void printPacketInfo(struct Packet packet);
    void printNeighborTable();
    struct Packet buildRoutingPacket();
    void checkL1OutBuffer();
    void checkL1InBuffer();

    uint8_t calculatePacketLoss(int entry, uint8_t sequence);
    uint8_t calculateMetric(int entry, uint8_t sequence, struct Metadata metadata);
    int checkNeighborTable(struct NeighborTableEntry neighbor);
    int checkRoutingTable(struct RoutingTableEntry route);
    int updateNeighborTable(struct NeighborTableEntry neighbor, int entry);
    int updateRouteTable(struct RoutingTableEntry route, int entry);
    int selectRoute(uint8_t destination[ADDR_LENGTH]);
    int routePacket(struct Packet packet, int broadcast);
    int parseForNeighbor(struct Packet packet, struct Metadata metadata);
    int parseForRoutes(struct Packet packet, struct Metadata metadata);

private:
    uint8_t _messageCount;
    uint8_t _localAddress[ADDR_LENGTH];
    float _packetSuccessWeight;
    float _randomMetricWeight;
    struct Packet _L1OutBuffer[8];
    int _L1OutBufferEntry;
    struct Packet _L1InBuffer[8];
    int _L1InBufferEntry;
    struct Packet _L3OutBuffer[8];
    int _L3OutBufferEntry;
    struct NeighborTableEntry _neighborTable[255];
    int _neighborEntry;
    struct RoutingTableEntry _routeTable[255];
    int _routeEntry;
    long _startTime;
    long _lastRoutingTime;
    int _routingInterval;
    int _disableRoutingPackets;
};

extern LL2Class LL2;
