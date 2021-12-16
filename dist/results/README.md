## Experimental Procedure Description

The results under [static](static/) where all generated with the same two
dw1001 devices. Each device was battery powered and with an sd-card to
offload epoch data.

The used application was [pepper_experience](../../apps/pepper_experience/README.md), with the difference
that at the time the modules `ed_uwb_los` and `ed_uwb_stats` did not exist.

For that experiment the BLE RSSI path los estimation model model used was:

Rssi(d) = -60.36 - 10x1.47xlog10(d/1.0)) dBm

The above mentioned application was flashed on both devices:

```
$ make -C apps/pepper_experience flash
```

There are two alternative to setup the experiments (configure the tag)
and start the experiment.

- UART shell: nothing extra to be done

```
$ make term
$ pepper set bn "door-120"
$ pepper start -c 5 -i 1000 -d 300 -r 10
```

- GATT shell proxy: `pepper_gatt` must be included in the build:

```
$ python dist/ble_proxy
$ Welcome to RIOT's PEPPER configuration over BLE shell! Type ? to list commands
pepper-gatt> help

Documented commands (type help <topic>):
========================================
config   disconnect  exit  ls    select  stop
connect  discover    help  scan  start
```

Use the `scan` command to scan for pepper devices, they will all advertise under a name of the type `DWXXXX`.

Select one of them with `select` command.

Then configure and start the experiment:

```
pepper-gatt> select 0
pepper-gatt> config -bn door-120
pepper-gatt> start -r 10 -c 5 -d 300
```

Repeat as many times as needed, then remove the SD card and inspect the results. Optionally the logged output could be captured over serial or notified via the GATT interface (but this is not yet supported).

### Experiment parameters

- Advertisement per EBID slice: 10
- Epoch duration: 300s
- Advertisement interval: 1000ms
- Scan window: 1024 ms
- Scan interval: 1024 ms
- UWB listen window: 2ms
- Iterations: 5
