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

0. Open a terminal on each node adv_data will be received and at the end of
an epoch PETS as well RTL window data will be printed:

```bash
TODO: update logs
```
