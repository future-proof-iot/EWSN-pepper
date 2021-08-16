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
calculation. These as well as faded windows are printed as
a JSON over serial.

By default devices are not time synchronized so if the `MIN_EXPOSE_TIME_S`
is too large with respect to `CONFIG_EBID_ROTATION` they might miss each
other. Optionally [time-advertiser](../../tests/time_advertiser/README.md) can
be started which will advertise a current time to which the `PEPPER` nodes
can synchronize to.

For more details see [PEPPER-draft](../../rdm/pepper_draft.md).

## PRE-requisites

* A valid [FIT IoT-LAB](https://www.iot-lab.info/) account.
* Initialize this repository as described in [README.md](../../README.md)

## Running IoT-LAB

0. Submit an experiment booking 3 nodes, and wait for experiment to start.

```bash
$ iotlab-experiment submit -d 120 -l saclay,dwm1001,3-5
$ iotlab-experiment wait
Waiting that experiment 267994 gets in state Running
"Running"
```

0. Bootstrap all nodes

```bash
$ BUILD_IN_DOCKER=1 IOTLAB_NODE=dwm1001-3.saclay.iot-lab.info make flash
$ BUILD_IN_DOCKER=1 IOTLAB_NODE=dwm1001-4.saclay.iot-lab.info make flash
$ BUILD_IN_DOCKER=1 IOTLAB_NODE=dwm1001-5.saclay.iot-lab.info make flash
```

0. Open a terminal on each node, once they start seeing each other (EBID
is reconstructed ranging requests will begin between those nodes).

```bash
main(): This is RIOT! (Version: 2021.10-devel-296-gd64a1-pepper/rbpf)
[pepper]: delay for 0s to align uwb_epoch start
[pepper]: new uwb_epoch t=0
[pepper]: new ebid generation
[pepper]: local ebid: [175, 145, 13, 230, 7, 163, 102, 247, 182, 38, 72, 11, 65, 111, 17, 240, 207, 2, 178, 243, 145, 50, 112, 147, 83, 52, 34, 111, 146, 64, 68, 91, ]
[pepper]: start adv
[pepper]: start scanning
[pepper]: skip, no neigh
...
[uwb_ed]: saw ebid: [224, 159, 13, 105, 125, 43, 147, 252,201, 69, 195,88, 184, 87, 252, 97, 186, 130, 134, 236, 2, 17, 56, 66, 147, 53, 97, 94, 125, 11, 204, 111, ]
[pepper]: 0x5312 at 28cm
[pepper]: 0x5312 at 25cm
[pepper]: 0x5312 at 26cm
...
[pepper]: 0x5312 at 19cm
[pepper]: 0x5312 at 22cm
[pepper]: end of uwb_epoch
[pepper]: process all uwb_epoch data
[pepper]: new uwb_epoch t=32
[pepper]: new ebid generation
[pepper]: local ebid: [156, 1, 54, 9, 126, 141, 17, 218, 196, 43, 135, 204, 12, 253, 37, 207, 100, 238, 4, 251, 36, 245, 184, 171, 19, 15, 0, 218, 239, 229, 210, 44, ]
[pepper]: dumping epoch data
{"epoch": 0,"pets": [{"pet": {"etl": "56B29D39D65ABEA6B964484608A3F72877356F163441AB7953EB307887469F68","rtl": "ED5B32BC83180711050FF7E2E3BBC00D3683B52BE39163B1FE98938564894413","exposure": 27878,"reqcount": 16,"avg_d_cm": 24}}]}

```

The epoch data is printed in JSON format, beautified it looks like:

```JSON
{
  "epoch": 0,
  "pets": [
    {
      "pet": {
        "etl": "56B29D39D65ABEA6B964484608A3F72877356F163441AB7953EB307887469F68",
        "rtl": "ED5B32BC83180711050FF7E2E3BBC00D3683B52BE39163B1FE98938564894413",
        "exposure": 27878,
        "reqcount": 16,
        "avg_d_cm": 24
      }
    }
  ]
}
```

## Setting up the time Advertiser

0. Flash the time advertiser on one of the devices:

```bash
BUILD_IN_DOCKER=1 IOTLAB_NODE=dwm1001-3.saclay.iot-lab.info make -C tests/time_advertiser flash term
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
