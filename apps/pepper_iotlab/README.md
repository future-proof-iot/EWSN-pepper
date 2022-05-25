# PEPPER IoT-LAB Application

Similar to [pepper_simple](./../pepper_simple/README.md) but where some of
the default parameters have been toggled to run better on IoT-LAB
when many devices are active and to perform as read-eval-print-loops.

- shell echo is disabled: `CFLAGS=-DCONFIG_SHELL_NO_ECHO=1`
- shell operates in blocking mode: `CFLAGS=-DCONFIG_PEPPER_SHELL_BLOCKING=1`
- UWB pan-id is changed from default (`0xcafe` instead of `0xdeca`)
- TWR event buffer `CONFIG_TWR_EVENT_BUF_SIZE` is doubled and so is the
  `CONFIG_ED_BUF_SIZE`
- `current_time_shell`: module is included to set the current time for the
  devices and have accurate EPOCH based timestamps.

By default UWB and BLE data is noy logged, since it might overload the state machine.
(compiled with `PEPPER_LOG_BLE=0 & PEPPER_LOG_UWB=0`).

At the end of of an epoch information on the encounters is logged over serial.

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
```

To modify more parameters take a look at the `help` command:

```bash
$ help
...
```
