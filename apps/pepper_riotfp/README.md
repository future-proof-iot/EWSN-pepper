# RIOT-PEPPER PoC

This application implements a naive and simple enhancement of the DESIRE contact
tracing protocol. In a nutshell devices advertise unique identifiers over BLE
and scan looking for other nodes performing those advertisements. Once a node
is "seen" (enough EBID slices have been scanned to be able to reconstruct it)
then TWR (two way raging) requests are exchanged between those devices to
determine their distance. If a device is seen for enough time and at a close
enough distance then its determined to be a relevant encounter and PET (Private
Encounter Tokens) are generated.

The EBID slice is rotated according to `CONFIG_SLICE_ROTATION_T_S` (default 20s).
Every `CONFIG_EBID_ROTATION_T_S` (default `15*60s`) the EBID is reset and an
epoch ends. At the end of an epoch all EBID seen for more than `MIN_EXPOSURE_TIME`
(default `10*60s`) and under `MAX_DISTANCE_CM` (default 200cm) trigger PET
calculation.

Different to the base application this demo adds IPV6 connectivity allowing
devices to offload the ERTL tables as well as notify and get information on
their infection/exposure status. A companion python server is required to receive
the ERTL tables as well as perform PET matching to determine the exposure status
(see [desire_coap_server](../../dist/desire_coap_server/README.md).

This application also includes RIOT-fp related functionalities. Mainly:

* BPF containerization: isolates logic determining relevant encounters
* SUIT updates: allow updating the relevant encounter logic
* EDHOC: secure key exchange for a poors-man OSCORE security context

This example will therefore required more setup as seen in the following
overview:

| ![](https://notes.inria.fr/uploads/upload_bfefe396078a30b026f2ade1816ef694.png) |
|:-------------------------------------------------------------------------------:|
|                          *Demo Architecture Overview*                           |

For the above:

* a BLE border router needs to be deployed
* a DESIRE CoAP server needs to be deployed and provisioned with EDHOC credentials
(if used)
* nodes need to be provided with EDHOC credentials (if used)
* an firmware server needs to be provided for SUIT updates

For more details on the demo workflow see [PEPPER-draft](../../rdm/riotfp_demo.md).

By default devices are not time synchronized so if the `MIN_EXPOSE_TIME_S`
is too large with respect to `CONFIG_EBID_ROTATION` they might miss each
other. Optionally [time-advertiser](../../tests/time_advertiser/README.md) can
be started which will advertise a current time to which the `PEPPER` nodes
can synchronize to.

## Setup

### Basic Setup (IPv6)


* Provision 2 or more dwm1001 devices

```
$ make -C apps/pepper_riotfp flash term
```

* Deploy a Border Router

Any RIOT based border router will do, so choose on a BLE capable device any
configuration as described in [gnrc_border_router](../../RIOT/examples/gnrc_border_router/README.md).

For convenience a `cdc-ecm` border router is used here on a `nrf52540-mdk-dongle`.
Its important to include the `nimble_autoconn_ipsp` module. If using `DHCPV6`
`STATIC_ROUTES` are used, otherwise something like `radvd` can be configured.

Bootstrap the border router and keep the terminal open:

```
$ STATIC_ROUTES=1 USE_DHCPV6=0 UPLINK=cdc-ecm USEMODULE="nimble_autoconn_ipsp" BOARD=nrf52840-mdk-dongle make -C RIOT/examples/gnrc_border_router/ flash term
```

The border router should get a globally routable address (if this is the first
time Kea will need to be installed).

```
Iface  9  HWaddr: FE:32:2A:89:4B:8D
          L2-PDU:1500  MTU:1500  HL:64  RTR
          Source address length: 6
          Link type: wired
          inet6 addr: fe80::fc32:2aff:fe89:4b8d  scope: link  VAL
          inet6 addr: fe80::2  scope: link  VAL
          inet6 group: ff02::2
          inet6 group: ff02::1
          inet6 group: ff02::1:ff89:4b8d
          inet6 group: ff02::1:ff00:2
Iface  7  HWaddr: FF:69:5A:5A:17:95
          L2-PDU:1280  MTU:1280  HL:64  RTR
          RTR_ADV  6LO  IPHC
          Source address length: 6
          Link type: wireless
          inet6 addr: fe80::ff69:5aff:fe5a:1795  scope: link  VAL
          inet6 addr: 2001:db8:0:2:ff69:5aff:fe5a:1795  scope: global  VAL
          inet6 group: ff02::2
          inet6 group: ff02::1
          inet6 group: ff02::1:ff5a:1795
```

From host:

```
ping6 2001:db8:0:2:ff69:5aff:fe5a:1795
PING 2001:db8:0:2:ff69:5aff:fe5a:1795(2001:db8:0:2:ff69:5aff:fe5a:1795) 56 data bytes
64 bytes from 2001:db8:0:2:ff69:5aff:fe5a:1795: icmp_seq=1 ttl=64 time=1.84 ms
64 bytes from 2001:db8:0:2:ff69:5aff:fe5a:1795: icmp_seq=2 ttl=64 time=1.04 ms
64 bytes from 2001:db8:0:2:ff69:5aff:fe5a:1795: icmp_seq=3 ttl=64 time=0.992 ms
^C
--- 2001:db8:0:2:ff69:5aff:fe5a:1795 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2003ms
rtt min/avg/max/mdev = 0.992/1.289/1.839/0.388 ms
```

* Deploy the CoAP server

Instal the requirements:

```
pip install -r dist/desire_coap_server/requirements.txt
```

Recover the devices identifiers (this can be done by flashing the devices
and using the `id` shell command):

```
> id
dwm1001 id: DW5FC2
```

Then start the coap server declaring all devices to enroll, eg:

```
python desire_coap_srv.py --node-uid DW5FC2 DWED75
```

### Running the Demo

With everything setup the demo can be started, devices will begin advertising
their EBID and an scanning for neighboring tokens advertising theirs, once
a neighbor is discovered TWR (two way ranging requests begin). Different than
the [basic pepper examples](../pepper) if there is IPV6 connectivity the devices
will periodically fetch the exposure status as well as offloading there ERTL
tables:

On devices:

```
[state_manager]: fetch esr
[state_manager]: esr=(0)
[state_manager]: serialized ertl, len=(81)
```

On server:

```
[DummyRqHandler] is_exposed: uid=DWED75 exposed=False
DEBUG:coap-server:Sending message <aiocoap.Message at 0x7f40abedecd0: Type.NON 2.05 Content (MID 38732, token 3092) remote <UDP6EndpointAddress [2001:db8::d359:5dff:fe10:4cf8] (locally fd00:dead:beef::1%enxfe322a894b8f)>, 1 option(s), 5 byte(s) payload>
DEBUG:coap-server:Incoming message <aiocoap.Message at 0x7f40ac755730: Type.NON POST (MID 6159, token 46d5) remote <UDP6EndpointAddress [2001:db8::d359:5dff:fe10:4cf8] (locally fd00:dead:beef::1%enxfe322a894b8f)>, 3 option(s), 64 byte(s) payload>
DEBUG:coap-server:New unique message received
DEBUG:coap-server:Sending message <aiocoap.Message at 0x7f40abee2970: Type.NON 2.31 Continue (MID 38733, token 46d5) remote <UDP6EndpointAddress [2001:db8::d359:5dff:fe10:4cf8] (locally fd00:dead:beef::1%enxfe322a894b8f)>, 1 option(s)>
DEBUG:coap-server:Incoming message <aiocoap.Message at 0x7f40ac74f4f0: Type.NON POST (MID 6160, token 00fc) remote <UDP6EndpointAddress [2001:db8::d359:5dff:fe10:4cf8] (locally fd00:dead:beef::1%enxfe322a894b8f)>, 3 option(s), 17 byte(s) payload>
DEBUG:coap-server:New unique message received
[DummyRqHandler] update_ertl: uid=DWED75, ertl = ErtlPayload(epoch=242, pets=[PetElement(pet=EncounterData(etl=b"\xda\xc0>2\x05w'\xeb-5\xb3\xbfm\xc0 \x80\x90@\xd6\x86Sh\xd2b7\xf5.2\xcb\xc5%~", rtl=b'\xccP\xb5 \x05\xe7\xddU]\xd21;\x81x7\n\xb0\xc6\xce\xf9\xe1\xfa$O\xad 9\x10\x0c\x06\xc33', exposure=64359, req_count=19, avg_d_cm=0))]), json =
{"epoch": 242, "pets": [{"pet": {"etl": "2sA+MgV3J+stNbO/bcAggJBA1oZTaNJiN/UuMsvFJX4=", "rtl": "zFC1IAXn3VVd0jE7gXg3CrDGzvnh+iRPrSA5EAwGwzM=", "exposure": 64359, "req_count": 19, "avg_d_cm": 0}}]}
DEBUG:coap-server:Sending message <aiocoap.Message at 0x7f40abee7fa0: Type.NON 2.04 Changed (MID 38734, token 00fc) remote <UDP6EndpointAddress [2001:db8::d359:5dff:fe10:4cf8] (locally fd00:dead:beef::1%enxfe322a894b8f)>, 2 option(s)>
```

If a device is declared positive (by pressing the user button) then the server is
notified and then devices that where in contact should see this and change their
status LED:

The infected device:

```
[state_manager]: COVID positive!
[state_manager]: send infected=1, len=(5)
```

The exposed device:

```
[state_manager]: fetch esr
[state_manager]: esr=(1)
[state_manager]: exposed!
```

### Add SUIT BPF updates

If `uwb_ed_bpf` is included devices are running an updatable BPF container, the threshold for contacts
can be lowered or increased (or even bigger changes can be made to [contact_filter.c](../../modules/sys/uwb_ed/bpf/contact_filter.c)).

For this a firmware server needs to be setup, the easiest is using `aiocoap-fileserver`,
in another terminal and in this directory (keep the terminal open)

```
$ mkdir coaproot
$ aiocoap-fileserver coaproot --bind [fd00:dead:beef::1]:5684
```

Now new firmware can be deployed and pushed to the devices, e.g: change the relevant distance threshold:

```
 BPF_CFLAGS=-DMAX_DISTANCE_CM=50 make -C apps/pepper_riotfp/ suit/publish
Warning! EXTERNAL_MODULE_DIRS is a search folder since 2021.07-branch, see https://doc.riot-os.org/creating-modules.html#modules-outside-of-riotbase
rm -f /home/francisco/workspace/pepper/desire/modules/sys/uwb_ed/bpf/contact_filter.o /home/francisco/workspace/pepper/desire/modules/sys/uwb_ed/bpf/contact_filter.bin
published "/home/francisco/workspace/pepper/desire/apps/pepper_riotfp/bin/dwm1001/pepper_driotfp-riot.suit.1629124780.bin"
       as "coap://[fd00:dead:beef::1]/fw/dwm1001/pepper_driotfp-riot.suit.1629124780.bin"
published "/home/francisco/workspace/pepper/desire/apps/pepper_riotfp/bin/dwm1001/pepper_driotfp-riot.suit.latest.bin"
       as "coap://[fd00:dead:beef::1]/fw/dwm1001/pepper_driotfp-riot.suit.latest.bin"
published "/home/francisco/workspace/pepper/desire/apps/pepper_riotfp/bin/dwm1001/pepper_driotfp-riot.suit_signed.1629124780.bin"
       as "coap://[fd00:dead:beef::1]/fw/dwm1001/pepper_driotfp-riot.suit_signed.1629124780.bin"
published "/home/francisco/workspace/pepper/desire/apps/pepper_riotfp/bin/dwm1001/pepper_driotfp-riot.suit_signed.latest.bin"
       as "coap://[fd00:dead:beef::1]/fw/dwm1001/pepper_driotfp-riot.suit_signed.latest.bin"
published "/home/francisco/workspace/pepper/desire/apps/pepper_riotfp/bin/dwm1001/pepper_driotfp.1629124780.bin"
       as "coap://[fd00:dead:beef::1]/fw/dwm1001/pepper_driotfp.1629124780.bin"
```

On the device the updates it should start ignoring devices further than 50cm away:

```
[pepper]: 0xeea8 at 88cm
[pepper]: 0xeea8 at 86cm
[pepper]: end of uwb_epoch
[pepper]: process all uwb_epoch data
[pepper]: new uwb_epoch t=331
[pepper]: new ebid generation
[pepper]: local ebid: [50, 42, 151, 39, 123, 86, 15, 210, 223, 112, 93, 62, 71, 198, 144, 62, 176, 229, 80, 12, 219, 231, 207, 75, 46, 220, 223, 130, 179, 120, 106, 9, ]
[pepper]: not connected, dumping epoch data
{"epoch": 302,"pets": [{"pet": {"etl": "7EF16D77A5B836A613BAAFCCF946CF1C66E29D0D90673405F0EC6596E5530537","rtl": "32E0B4D649659E2857A467185A08E2D9F197662A986D3393F9A932E74460B654","exposure": 64186,"reqcount": 13,"avg_d_cm": 90}}]}
[uwb_ed_bpf]: lock region
[uwb_ed_bpf]: unlock region
[pepper]: 0x2c98 at 102cm
[pepper]: 0x2c98 at 102cm
[pepper]: 0x2c98 at 143cm
[pepper]: 0x2c98 at 88cm
[pepper]: end of uwb_epoch
[pepper]: process all uwb_epoch data
[pepper]: new uwb_epoch t=480
[pepper]: new ebid generation
[pepper]: local ebid: [187, 164, 247, 34, 38, 139, 141, 121, 32, 8, 29, 16, 243, 131, 167, 60, 78, 55, 20, 175, 41, 152, 155, 163, 244, 230, 239, 34, 204, 60, 150, 86, ]
[pepper]: not connected, dumping epoch data
{"epoch": 450,"pets": []}
```

### Add EDHOC, Security Context

A security context used to secure server communication can also be used, this
context will be bootstrapped via an EDHOC handshake.

Before flashing the devices we need to make sure that each device is provided
with credentials for the key exchange as well as the credentials to authenticate
its counterpart (the CoAP server).

0. Generate server side credentials and export them to `modules/sys/edhoc_coap`

```bash
$ cd
$ python tools/edhoc_generate_keys.py
$ tools/edhoc_keys_header.py
```

A file `PEPPER_keys.c` should now be in `modules/sys/edhoc_coap`

0. Generate and export device credentials for every device of id `DWxxxx`

```bash
$ cd
$ echo "DWxxxx" | base64 | xargs python tools/edhoc_generate_keys.py  --out-dir modules/sys/edhoc_coap --kid
```

A file named `DWxxxx_keys.c` should now be in `modules/sys/edhoc_coap`.

0. Bootload the devices including `state_manager_security` module:

```bash
USEMODULE="state_manager_security" make -C apps/pepper_riotfp/ flash term
```

After a while once connected the devices should perform and EDHOC key exchange
with the server... from there on all communication will be encrypted:

```
[initiator]: sending msg1 (41 bytes):
[initiator]: received a message (116 bytes):
[initiator]: sending msg3 (88 bytes):
[initiator]: handshake successfully completed
...
...
[state_manager]: fetch esr
[state_manager]: attempt to decode...success
[state_manager]: esr=(0)
[state_manager]: serialized ertl, len=(155)
[state_manager]: encrypted ertl, len=(187)

```

On the server side, it will now always encrypt messages going to the device.

### Setting up the time Advertiser

0. Flash the time advertiser on a BLE capable device:

```bash
BUILD_IN_DOCKER=1 make -C tests/time_advertiser flash term
```

0. Set a desire time

```
> time
time
Current Time =  2020-01-01 00:00:17, epoch: 17
> time 2021 08 16 14 08 00
time 2021 08 16 14 08 00
```

0. Start advertising

```
> time
time
Current Time =  2020-01-01 00:00:17, epoch: 17
> time 2021 08 16 14 08 00
time 2021 08 16 14 08 00
> start
start
[Tick]
Current Time =  2021-08-16 14:12:29, epoch: 51286349
[Tick]
Current Time =  2021-08-16 14:12:30, epoch: 51286350
[Tick]
Current Time =  2021-08-16 14:12:31, epoch: 51286351
```

Nodes should synchronize and re-start their epochs accordingly

```
[pepper]: time was set back too much, bootstrap from 0
[pepper]: delay for 27s to align uwb_epoch start
[pepper]: new uwb_epoch t=51286350
```
