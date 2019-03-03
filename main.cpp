#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Layer1_Sim.h"
#include "LoRaLayer2.h"

int state = 0;
int chance;
long startTime;
long lastRoutingTime;

int _routeInterval = 10;
int _learningTimeout = 200;
int _maxRandomDelay = 20;

int routeInterval(){
        return simulationTime(_routeInterval);
}
int learningTimeout(){
        return simulationTime(_learningTimeout);
}
int maxRandomDelay(){
        return simulationTime(_maxRandomDelay);
}

int setup(){

    uint8_t* myAddress = localAddress();
    Serial.printf("local address: ");
    printAddress(myAddress);
    Serial.printf("\n");

    srand(time(NULL) + getpid());
    // random blocking wait at boot
    int wait = rand()%maxRandomDelay();
    Serial.printf("waiting %d s\n", wait);
    sleep(wait);

    chance=rand()%15;

    startTime = getTime();
    lastRoutingTime = startTime;
    return 0;
}

int loop(){

    if(begin_packet()){
        checkBuffer(); 
        if(state == 0){
            long timestamp = transmitRoutes(routeInterval(), lastRoutingTime);
            if(timestamp){
                lastRoutingTime = timestamp;
            }
            if(getTime() - startTime > learningTimeout()){
                state++;
                printNeighborTable();
                printRoutingTable();
            }
        }else if(state == 1){
            if(chance == 1){
                long timestamp = transmitToRandomRoute(routeInterval(), lastRoutingTime);        
                if(timestamp){
                    lastRoutingTime = timestamp;
                }
            }
        }
    }
    nsleep(0, 1000000*simulationTime(1));
}
