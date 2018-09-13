
These are the routers (routing algorithms) written in C.

These communicate with the simulator using `stdin` and `stdout`.

Each router must declare:

```
int setup(); // called once on startup
int loop(); // called once per event loop iteration
int packet_received(char* data, size_t len); // called when a packet is received
```

To send a packet call:

```
int send_packet(char* data, size_t len);
```

If data is a null-terminated string then you can set len to 0 and it will be automatically determined. Remember that packets are maximum 256 bytes.

A minimal router example is available in `ping_example.c` and can be compiled with the command `make`.

You can use the convenience function `Serial.printf()` to print debug output to `stderr`. 

Remember that you cannot use `stdin` or `stdout` so using just `printf()` will break things.

If you use `sleep()` or `usleep()` in `setup()` or `loop()` then this will block the event loop which means that your `packet_received()` function will not be called until after the sleep is over. If you want to avoid this then you can instead call `nsleep(seconds, microseconds)` once inside `loop()` (calling it more than once overrides the previous calls) which will cause `loop()` to be called again the specified amount of time after completion, without blocking packet reception while "sleeping".

