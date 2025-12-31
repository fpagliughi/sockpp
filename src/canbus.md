# [Experimental] CAN Bus on Linux with SocketCAN

The Controller Area Network (CAN bus) is a relatively simple protocol typically used by microcontrollers to communicate inside an automobile or industrial machine. Linux has the _SocketCAN_ package which allows processes to share access to a physical CAN bus interface using Raw sockets in user space. See: [Linux SocketCAN](https://www.kernel.org/doc/html/latest/networking/can.html)

The bus is a simple twisted pair of wires that can have multiple "nodes" (devices) attached to them which can communiate with each other using small, individual packets, called "frames". Each frame is sent to a specific numeric ID (addresses) on the bus. The ID can be a normal one of 11-bits, or an extended ID containing 29 bits. The IDs form a message prioritization where the lower ID is the higher priority. If there is a bus collision, the frame with the lower ID is allowed to proceed, while the other is forced to back-off and try again later.

The two primary flavors of can frames are "standard, CAN 2.0 frames which can contain 8 bytes of data, or CAN-FD frames which can contain up to 64 bytes. In addition, CAN FD can transmit the data payload at a higher bit rate than the framing data, while still allowing the bus to be shared between standard and FD frames.

## Example

As an example, consider a device with a temperature sensor which might read the temperature peirodically and write it to the bus as a raw 32-bit integer. Each temperature frame will use the CAN ID of 0x40.

```cpp
// Use the interface name to get an address, and create a socket for it.
can_address addr("CAN0");
can_socket sock(addr);

// The agreed ID to broadcast temperature on the bus
canid_t canID = 0x40;

while (true) {
    this_thread::sleep_for(1s);

    // Write the time to the CAN bus as a 32-bit int
    int32_t t = read_temperature();

    can_frame frame { canID, &t, sizeof(t) };
    sock.send(frame);
}
```

A receiver to get a frame might look like this:

```
can_address addr("CAN0");
can_socket sock(addr);

can_frame frame;
sock.recv(&frame);
```
