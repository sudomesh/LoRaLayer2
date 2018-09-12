

all: ping_example firmware

ping_example: ping_example.c base.c
	gcc -o ping_example ping_example.c base.c

firmware: firmware.c base.c
	gcc -o firmware firmware.c base.c

clean:
	rm -f ping_example
	rm -f firmware 
