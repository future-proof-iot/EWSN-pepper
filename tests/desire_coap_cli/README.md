# Desire Coap CLI

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

For a hardware setup connectivity provided by BLE, a BLE border router must be
in the vicinity.

The test assumes that "fd00:dead:beef::1" is the COAP server address, if not
change it from the command line with

```shell
> server xxxx:xxxx:xxxx::x:port
```
