# PEPPER

This implements PEPPER (PrEcise Privacy-PresERving Proximity Tracing). The
application is controlled via shell commands. Et the beginning of every
epoch (`CONFIG_EPOCH_DURATION_SEC` (default `15*60s`)) ephemeral unique
identifiers are generated and advertised over BLE. At the same time
devices scan for neighboring advertisements.

When advertisements are received a TWR exchange can optionally be initiated
between the scanner and advertiser.

At the end of of an epoch information on the encounters is logged over serial.

Currently devices are not time synchronized so if the `MIN_EXPOSE_TIME_S`
is too large with respect to `CONFIG_EPOCH_DURATION_SEC` they might miss each
other. A BLE time advertiser can be set up to synchronize devices. See
[../../tests/time_server/README.md].

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

0. Open a terminal on each node at the end of an epoch contact data for
each encounter should be logged to serial.

```bash
# Start with default parameters: 15min epochs, 1adv/s, 20adv/slice, run until
# stopped
$ pepper start
# Run the application for only 5 epochs, shorten advertisement intervals to
# 500ms, and rotate the EBID slice on every advertisement
$ pepper start -c 5 -i 500 -r 1
TODO: update logs
```

To modify more parameters take a look at the `help` command:

```bash
$ help
...
```
