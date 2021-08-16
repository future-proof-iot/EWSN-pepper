# PEPPER mock

This application provides a mock to test PEPPER modules in a network environment.
It does not included BLE adv/scan or TWR but simply generates random `uwb_epoch`
data that can then be sent to the server.

## Native Networking SetUp

For native simply generate a TAP interface and add a globally routable address
to that interface (the test code already adds an address on the same net to the
native instance).

```bash
$ sudo ip tuntap add tap0 mode tap user $USER
$ sudo ip link set tap0 up
$ sudo ip address add 2001:db8::1/64 dev tap0
$ make -C tests/pepper_mock flash term PORT=tap0
```

## Hardware Networking SetUp

For a hardware setup connectivity is provided via ethos (ethernet over serial),On one terminal (keep open):

```bash
$ sudo ${RIOTBASE}/dist/tools/ethos/setup_network.sh riot0 2002:db8::/64
```

Then on another terminal flash the `BOARD`:

```bash
BOARD=<board-name> make -C tests/pepper_mock flash term
```
