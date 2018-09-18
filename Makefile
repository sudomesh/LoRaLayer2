

all: ping_example firmware

ping_example: ping_example.c base.c base.h
	gcc -o ping_example ping_example.c base.c

firmware: firmware.c base.c base.h
	gcc -lpthread -lssl -lcrypto -o firmware firmware.c base.c

clean:
	rm -f ping_example
	rm -f firmware 
