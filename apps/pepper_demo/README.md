# PEPPER Demo

This application runs PEPPER (PrEcise Privacy-PresERving Proximity Tracing) and
adding the required futures to run with [desire CoAP server](https://gitlab.inria.fr/pepper/desire-coap-server)
in non-secured mode.

In a nutshell devices advertise unique identifiers over BLE
and scan looking for other nodes performing those advertisements. Once a node
is "seen" (enough EBID slices have been scanned to be able to reconstruct it)
then TWR (two way raging) requests are exchanged between those devices to
determine their distance. If a device is seen for enough time and at a close
enough distance then its determined to be a relevant encounter and PET (Private
Encounter Tokens) are generated.

The EBID slice is rotated according to `ADV_PER_SLICE` (default 5).
Every `EPOCH_DURATION_SEC` (default `300s`) the EBID is reset and an
epoch ends. At the end of an epoch all EBID seen for more than `MIN_EXPOSURE_TIME`
(default `30s`) and under `MAX_DISTANCE_CM` (default 200cm) trigger PET
calculation. For demo purposes devices are set to continuously SCAN.

Different than [pepper_simple](../pepper_simple/README.md) `pepper_srv_coap`, which adds
IPV6 connectivity is included, allowing devices to offload the ERTL tables as well
as notify and get information on their infection/exposure status.

| ![](../../static/pepper-demo-overview.png) |
|:-------------------------------------------------------------------------------:|
|                          *Demo Architecture Overview*                           |

For the above:

* a BLE border router needs to be deployed
* a DESIRE CoAP server needs to be deployed

## Setup

### Basic Setup (IPv6)


* Provision 2 or more dwm1001 devices

```
$ make -C apps/pepper_demo flash term
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

```
git clone https://gitlab.inria.fr/pepper/desire-coap-server.git
cd desire-coap-server
pip install .
```

Recover the devices identifiers (this can be done by flashing the devices
and using the `id` shell command):

```
> pepper get uid
[pepper]: uid DW45ED
```

Then start the coap server declaring all devices to enroll, eg:

```
python desire_coap_srv.py --node-uid DW45ED DW...
```

If your host supports has multiple IPV6 addresses then specify the address
on which to bind:

```
python desire_coap_srv.py --node-uid DW45ED DW... --host "fd00:dead:beef::1"
```

If binding on a different address than `fd00:dead:beef::1"` (RIOTs border router
tools assign this address to `lo`), then for the PEPPER devices change `CONFIG_PEPPER_SRV_COAP_HOST`
accordingly.

```
CFLAGS='-DCONFIG_PEPPER_SRV_COAP_HOST=\"2001:660:3207:4bf::17\"' make -C apps/pepper_demo flash term
```

### Running the Demo

With everything setup the demo can be started, devices will begin advertising
their EBID and an scanning for neighboring tokens advertising theirs, once
a neighbor is discovered TWR (two way ranging requests begin). Different than
the [basic pepper examples](../pepper_simple) if there is IPV6 connectivity the devices
will periodically fetch the exposure status as well as offloading there ERTL
tables:

On devices:

```
main(): This is RIOT! (Version: 2022.07-devel-665-gfa0cd8-pepper/develop)
Pepper Demo Applicaiton, start with default parameters
[pepper]: new uwb_epoch t=0
[pepper]: new ebid generation
[pepper]: local ebid:
        0x59 0xf4 0x81 0xe4 0x0d 0xf6 0x00 0x3e
        0x6a 0xe6 0x89 0x22 0x61 0x51 0x92 0x70
        0x17 0x77 0x71 0x05 0x1b 0x49 0x0d 0xe0
        0x5c 0xf8 0x71 0x28 0xd4 0xcf 0xc5 0x6f
[pepper]: end of uwb_epoch
[pepper]: process all uwb_epoch data
...
[pepper_srv] coap: send ertl to /DWDEAF/ertl
```

On server:

```
coap-server - [pet_offloading]: received rtl from uid=DWDEAF
{
  "epoch": 61,
  "pets": [
    {
      "pet": {
        "etl": "",
        "rtl": "OAJbuhDKq23Ae1uQ8Sqeg0ENOjTL4uuzf0rYHmpphyE=",
        "uwb": {
          "exposure": 49,
          "req_count": 39,
          "avg_d_cm": 17
        }
      }
    }
  ]
}

```

If a device is declared positive (by pressing the user button) then the server is
notified and then devices that where in contact should see this and change their
status LED, this can also be done with a shell command:

```
> pepperd inf true
pepperd inf true
[pepper_srv] coap : serialize infected=true, len=(5)
[infected_declaration]: COVID positive!
```

The infected device:

```
[pepper_srv] coap : serialize infected=true, len=(5)
[infected_declaration]: COVID positive!
```

The server:

```
  coap-server - [infected_declaration]: uid=DWDEAF is_infected=(True)
  coap-server - [exposure_status]: uid=DW75ED is_exposed=(True
```

The exposed device:

```
> [pepper_srv] coap: fetch esr at /DW75ED/esr
[exposure_status]: COVID contact!
```

### Contact Tracing Dashboard

A contact tracing dashboard can also be added, its based on a `telegraph`+`influxdb`+`grafana`
stack. An easy way to start is with the provided `docker-compose` file.

1. Create a .env file and populate it with your UID and GUID

```bash
UUID=1001
GID=1001
```

These are easy to obtain through `id -u` and `id -g`, then simply setup the dashboard.

2. Start the dashboard

```bash
cd desire-coap-server/desire_dashboard docker-compose up
```

3. Restart the server specifying where to log contact tracing events:

```bash
desire-coap-server --node-uid DW75ED DW... --host "fd00:dead:beef::1" --event-log http://127.0.0.1:8080/telegraf
```

4. Go to http://localhost:3000/d/SG9hNcNnk/pepper-riot-fp-viz and visualize the devices:

- exposure status
- infection status
- average distance to neighbors

For more refer to the  [desire-dashboard documentation](https://gitlab.inria.fr/pepper/desire-coap-server/-/blob/master/desire_dashboard/README.md)
