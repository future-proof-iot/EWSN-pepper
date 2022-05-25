# RIOT-DESIRE

RIOT-based desire implementation. This simple PoC wil start a DESIRE
advertiser and scanner. The EBID slice is rotated according to
`CONFIG_ADV_PER_SLICE` and `ADV_ITVL_MS`, (default 20s). Every
`CONFIG_EPOCH_DURATION_SEC` (default `15*60s`) the EBID is reset and an
epoch ends. At the end of an epoch all EBID seen for more than `MIN_EXPOSURE_TIME`
(default `5*60s`) trigger PET calculation. These as well as faded windows are
printed as a JSON over serial or logged on an sd-card if the `storage` and
`mtd_sdcard` modules are used.

Currently devices are not time synchronized so if the `MIN_EXPOSE_TIME_S`
is too large with respect to `CONFIG_EPOCH_DURATION_SEC` they might miss each
other.

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

or

```
$ iotlab-node --flash bin/dwm1001/pepper_desire.elf -i <exp_id>
```

1. Open a terminal on each node adv_data will be received and at the end of
an epoch PETS as well RTL window data will be printed:

```bash
[{"bn":"DWB2A7:ble:16481c48","bt":313924,"n":"rssi","v":-66,"u":"dBm"}]
[{"bn":"DWB2A7:ble:18797b9","bt":313933,"n":"rssi","v":-69,"u":"dBm"}]
[{"bn":"DWB2A7:ble:39b443d9","bt":313950,"n":"rssi","v":-66,"u":"dBm"}]
[{"bn":"DWB2A7:ble:80468ed","bt":313990,"n":"rssi","v":-70,"u":"dBm"}]
[{"bn":"DWB2A7:ble:3ff1da04","bt":314007,"n":"rssi","v":-62,"u":"dBm"}]
[{"bn":"DWB2A7:ble:27e3b8ed","bt":314014,"n":"rssi","v":-70,"u":"dBm"}]
...
{"tag":"DWB2A7","epoch":0,"pets":[{"pet":{"etl":"ADA57255D6C2B2D5A7CAB072E7BEBF0033838431E3C4CECA827EA30117FEE44F","rtl":"E0D168F776A4E704812F1C2AA503AA56A04365A3445C4F62DB48736778F9A206","ble_win":{"exposure":856,"wins":[{"samples":10,"rssi":-65.1738052},{"samples":14,"rssi":-65.3543014},{"samples":14,"rssi":-65.4659118},{"samples":20,"rssi":-65.3485336},{"samples":24,"rssi":-65.0702285},{"samples":21,"rssi":-64.9768066},{"samples":20,"rssi":-64.9171524},{"samples":22,"rssi":-65.1046066},{"samples":25,"rssi":-64.9858246},{"samples":24,"rssi":-64.8542633},{"samples":23,"rssi":-64.9624100},{"samples":22,"rssi":-64.8809738},{"samples":20,"rssi":-64.9652939},{"samples":24,"rssi":-64.8940125},{"samples":0,"rssi":0}]}}},
...
]}

```

Optionally lunch [serial aggregator](https://iot-lab.github.io/docs/tools/serial-aggregator/) on IoT-LAB:

```bash
$ ssh <login>@gsaclay.iot-lab.info
<login>@saclay:~$ serial_aggregator
1653483949.534585;dwm1001-3;[{"bn":"DWCD65:ble:27e3b8ed","bt":226971,"n":"rssi","v":-61,"u":"dBm"}]
1653483949.534635;dwm1001-6;[{"bn":"DW68AB:ble:27e3b8ed","bt":227018,"n":"rssi","v":-84,"u":"dBm"}]
1653483949.534678;dwm1001-9;[{"bn":"DWF8EB:ble:27e3b8ed","bt":226967,"n":"rssi","v":-70,"u":"dBm"}]
1653483949.534734;dwm1001-4;[{"bn":"DW78EB:ble:27e3b8ed","bt":227004,"n":"rssi","v":-54,"u":"dBm"}]
1653483949.534788;dwm1001-7;[{"bn":"DW27DD:ble:27e3b8ed","bt":227013,"n":"rssi","v":-69,"u":"dBm"}]
1653483949.536828;dwm1001-5;[{"bn":"DW225F:ble:3ff1da04","bt":226992,"n":"rssi","v":-60,"u":"dBm"}]
1653483949.540801;dwm1001-7;[{"bn":"DW27DD:ble:3ff1da04","bt":227019,"n":"rssi","v":-70,"u":"dBm"}]
1653483949.540889;dwm1001-2;[{"bn":"DW186B:ble:3ff1da04","bt":227033,"n":"rssi","v":-68,"u":"dBm"}]
1653483949.540948;dwm1001-1;[{"bn":"DWB2A7:ble:3ff1da04","bt":226996,"n":"rssi","v":-64,"u":"dBm"}]
1653483949.540993;dwm1001-6;[{"bn":"DW68AB:ble:3ff1da04","bt":227024,"n":"rssi","v":-71,"u":"dBm"}]
1653483949.541048;dwm1001-9;[{"bn":"DWF8EB:ble:3ff1da04","bt":226973,"n":"rssi","v":-66,"u":"dBm"}]
1653483949.541130;dwm1001-4;[{"bn":"DW78EB:ble:3ff1da04","bt":227010,"n":"rssi","v":-54,"u":"dBm"}]
```
