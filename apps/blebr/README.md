# BLE Border Router

This application sets up a IPv6 over BLE border router application. To run
it requires a BOARD supporting NimBLE (essentially an NRF5x BOARD).

## Uplink

The border router will route packets between a 6Lo network (PAN) and a 'normal'
IPv6 network (i.e. the Internet).

This requires the border router to have two interfaces: A downstream interface
to run 6LoWPAN on and an IPv6 uplink.

This example comes with support for three uplink types pre-configured:

 - [`ethos`](https://doc.riot-os.org/group__drivers__ethos.html) (default)
 - `cdc-ecm`


`ethos` will make use of the existing serial interface that is used for the
RIOT shell to provide an upstream interface. Your computer will act as the upstream
router, stdio is multiplexed over the same line.

`cdc-ecm` will create a Ethernet Control Module subclass (CDC-ECM) class driver
is used to send and receive Ethernet frames over USB. stdio can also be
included in which case a `cdc-acm` interface will also be added, but this
is not required.

## Setup

To compile and flash `gnrc_border_router` to your board:

```bash
make clean all flash
```

The default uplink is `cdc-ecm` so select a different one with `UPLINK` var:

```bash
UPLINK=ethos make clean all flash
```

## Usage

The `start_network.sh` script needed for `ethos` and `cdc-ecd` is automatically run
if you type

```bash
make term
```

This will setup the `tap` (`ethos`) or `enxMACAddress` (`cdc-ecm`) interface and
configure the Border Router.

Notice that this will also configure `2001:db8::/64` as a prefix (can be changed
through `IPV6_PREFIX` make environment variable).
This prefix should be announced to other motes through the wireless interface.

For more information refer to [gnrc_border_router](https://github.com/RIOT-OS/RIOT/blob/master/examples/gnrc_border_router/README.md).

### Running outside of RIOT

To be able to run outside of RIOT a simple script with a systemd service is
provided. To run the script simple:

```bash
sudo usr/local/sbin/blebr [-i interface] [-p ipv6 prefix ] [-s server-address]
```

The default value for the interface is blebr0, change it as required. On recent
linux systems the usb interface would be assigned MAC based address so
something like `enxffffffff`.

#### Systemd service

A systemd service is provided to always start UHCP and so enable the border
router when a network device named blebr0 appears. On linux based systems network
naming is either done via `udev` or `systemd.link` (see [NetworkInterfaceNames]).

Depending on your Debian version you will either need to add an `udev` rule for
this or using `.link` files.

- udev:

```bash
sudo cp etc/udev/rules.d/80-net-setup-link.rules /etc/udev/rules.d/80-net-setup-link.rules
sudo udevadm control --reload-rules
```

- link:

```
sudo cp etc/systemd/network/71-persistent-riot-net.link /etc/systemd/network/71-persistent-riot-net.link
sudo systemctl restart systemd-networkd
```

In both cases we are using the `ID_MODEL` property of the usb device (set vía
`CFLAGS` in the Makefile) to match the device and assign the name blebr0. With this
systemd can launch the service and stop it based on the interface being present.

### DHCPv6 vs UHCP

It should be possible to run a similar setup with DHCPv6 (using [KEA] or
other). But at the time of writing this README.md locally instability
issues where experienced. Therefore for now UHCP is used.

For more checkout [gnrc_border_router](https://github.com/RIOT-OS/RIOT/blob/master/examples/gnrc_border_router/README.md) on setup documentation.

[NetworkInterfaceNames]: https://wiki.debian.org/NetworkInterfaceNames
[KEA]: https://kea.isc.org/
