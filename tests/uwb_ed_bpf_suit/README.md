# UWB Encounter Data BPF SUIT Update test application

This test application allows performing small changes to the BPF container
that filters out contacts and updating it via SUIT.

## Native Networking SetUp

For native simply generate a TAP interface and add a globally routable address
to that interface (the test code already adds an address on the same net to the
native instance).

```bash
$ sudo ip tuntap add tap0 mode tap user $USER
$ sudo ip link set tap0 up
$ sudo ip address add 2001:db8::1/64 dev tap0
$ make -C tests/uwb_ed_bpf_suit flash term PORT=tap0
```

## Hardware Networking SetUp

For a hardware setup connectivity is provided via ethos (ethernet over serial),
On one terminal (keep open):

```bash
$ sudo ${RIOTBASE}/dist/tools/ethos/setup_network.sh riot0 2002:db8::/64
```

Then on another terminal flash the `BOARD`:

```bash
BOARD=<board-name> make -C tests/uwb_ed_bpf_suit flash term
```

## Firmware server setup

In the application directory create a `coaproot` directory and lunch the
aiocoap-fileserver (keep the terminal open):

```bash
$ mkdir coaproot
$ aiocoap-fileserver coaproot
```

## Updating the container

The test application creates a `uwb_ed` of value `200cm`, so switching the
limit above this value should change the container function output:

Initial result:

```
uwb_ed_finish_bpf: d=(200), exposure=(601), valid=(1)
```

Modify the threshold through `F12R_CFLAGS`:

```
F12R_CFLAGS=-DMAX_DISTANCE_CM=100 make suit/publish; make suit/notify
```

Modified run result

```
[uwb_ed_bpf]: lock region
[uwb_ed_bpf]: unlock region
> run
uwb_ed_finish_bpf: d=(200), exposure=(601), valid=(0)
```

### Automatic test

If a tap interface is provided according to the networking setup description
then simply run:

```bash
make flash test-with-config
```
