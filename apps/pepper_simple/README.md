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

or

```
$ iotlab-node --flash bin/dwm1001/pepper_desire.elf -i <exp_id>
```

0. Open a terminal on each node at the end of an epoch contact data for
each encounter should be logged to serial.

```bash
# Start with default parameters: 15min epochs, 1adv/s, 20adv/slice, run until
# stopped
$ pepper start
# Run the application for only 2, 60s epochs, shorten advertisement intervals to
# 500ms, and rotate the EBID slice on every advertisement
$ pepper start -c 2 -i 500 -r 1 -d 60

[{"bn":"DW27DD:uwb:36a955cf","bt":167846,"n":"d_cm","v":319,"u":"cm"}]
[{"bn":"DWF8EB:uwb:3ddb0371","bt":167778,"n":"d_cm","v":484,"u":"cm"}]
[{"bn":"DW68AB:uwb:2f9c24de","bt":167859,"n":"d_cm","v":688,"u":"cm"}]
[{"bn":"DWF8EB:uwb:123d5688","bt":167809,"n":"d_cm","v":319,"u":"cm"}]
[{"bn":"DW186B:uwb:3ddb0371","bt":167908,"n":"d_cm","v":293,"u":"cm"}]
[{"bn":"DWF8EB:uwb:2f9c24de","bt":167857,"n":"d_cm","v":455,"u":"cm"}]
[{"bn":"DW225F:uwb:36a955cf","bt":168112,"n":"d_cm","v":607,"u":"cm"}]
[{"bn":"DW78EB:uwb:36a955cf","bt":168143,"n":"d_cm","v":489,"u":"cm"}]
[{"bn":"DW27DD:uwb:2f9c24de","bt":168184,"n":"d_cm","v":557,"u":"cm"}]
{"tag":"DW1EAF","epoch":129,"pets":[{"pet":{"etl":"EEF409702C1F7ACA695F3016E49C4907C0AE0AB9F6FEFE90037097340900F5C5","rtl":"088C3E7FB261CD1D677D77C6E92AD1EDA104B5A6255661998B3182C00FDA0855","uwb":{"exposure":51,"req_count":15,"avg_d_cm":139}}},{"pet":{"etl":"7E728DC7BC95C0997ADFC0114C886CCC07F2A437C96BCE0695D88939CA904E8D","rtl":"3C7E137070B9C7D972AFA9BAA51DBA4FA9F587FB50D89A11BB887B68ABD2603C","uwb":{"exposure":36,"req_count":10,"avg_d_cm":532}}},{"pet":{"etl":"E5D0B0C648B2F5BA6FE112B7C30601D59CFFD3656AB72BB94139574F36189305","rtl":"956131303D4576B9435BBACD4F4F827B0EB914A36257E0B101BD0D6E9689789A","uwb":{"exposure":46,"req_count":8,"avg_d_cm":469}}},{"pet":{"etl":"6287F0A674B7410A316EE82DA02C23767CEB24DD88BDBD6549890FE961AA044A","rtl":"BB7290A690279D07CEEB0112D252A3418F74465CD6B9F2EB7CEF3AAFFB5EC647","uwb":{"exposure":52,"req_count":14,"avg_d_cm":455}}},]}
```

Optionally lunch [serial aggregator](https://iot-lab.github.io/docs/tools/serial-aggregator/) on IoT-LAB:

```bash
$ ssh <login>@gsaclay.iot-lab.info
<login>@saclay:~$ serial_aggregator
```

To modify more parameters take a look at the `help` command:

```bash
$ help
```
