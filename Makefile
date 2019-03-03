

all: ping_example firmware

ping_example: ping_example.c base.c base.h
	gcc -o ping_example ping_example.c base.c

firmware: main.cpp LoRaLayer2.cpp LoRaLayer2.h Layer1_Sim.cpp Layer1_Sim.h 
	gcc -lpthread -lssl -lcrypto -o firmware main.cpp LoRaLayer2.cpp Layer1_Sim.cpp 

clean:
	rm -f ping_example
	rm -f firmware 
