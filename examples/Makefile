
all: firmware

LL2LIB=./libs/LoRaLayer2/src

firmware: main.cpp
	gcc -I$(LL2LIB) -lpthread -lssl -lcrypto -o firmware main.cpp $(LL2LIB)/Layer1_Sim.cpp $(LL2LIB)/LoRaLayer2.cpp

clean:
	rm -f firmware 
