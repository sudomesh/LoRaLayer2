

all: ping_example

ping_example: ping_example.c base.c
	gcc -o ping_example ping_example.c base.c

clean:
	rm -f ping_example
