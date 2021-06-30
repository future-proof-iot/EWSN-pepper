# RIOT-DESIRE PoC

RIOT-based desire implementation. This simple PoC wil start a DESIRE
advertiser and scanner. The EBID slice is rotated according to
`CONFIG_SLICE_ROTATION_T_S` (default 20s). Every `CONFIG_EBID_ROTATION_T_S`
(default `15*60s`) the EBID is reset and an epoch ends. At the end of an
epoch all EBID seen for more than `MIN_EXPOSURE_TIME` (default `10*60s`)
trigger PET calculation. These as well as faded windows are printed as
a JSON over serial.

Currently devices are not time synchronized so if the `MIN_EXPOSE_TIME_S`
is too large with respect to `CONFIG_EBID_ROTATION` they might miss each
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
$ BUILD_IN_DOCKER=1 IOTLAB_NODE=dwm1001-5.saclay.iot-lab.info make term
main(): This is RIOT! (Version: 2021.07-devel-184-g072311-pepper/nimble-uwb-core)
[desire]: new epoch t=0
[desire]: starting new epoch
[desire]: local ebid: [232, 103, 92, 134, 124, 213, 112, 239, 103, 25, 62, 236, 61, 4, 97, 199, 54, 214, 103, 2, 108, 154, 225, 66, 21, 111, 136, 18, 101, 19, 151, 99, ]
[desire]: start adv
[desire]: start scanning
[desire]: schedule end of epoch
> [desire]: adv_data 2a31f181: t=2264, RSSI=-55, sid=0
[desire]: adv_data 2a31f181: t=3266, RSSI=-59, sid=0
[desire]: adv_data 2a31f181: t=4269, RSSI=-53, sid=0
[desire]: adv_data 23d1c64c: t=4472, RSSI=-51, sid=0
[desire]: adv_data 2a31f181: t=5266, RSSI=-52, sid=0
[desire]: adv_data 23d1c64c: t=5480, RSSI=-51, sid=0
[desire]: adv_data 23d1c64c: t=6477, RSSI=-53, sid=0
[desire]: adv_data 2a31f181: t=7266, RSSI=-53, sid=0
[desire]: adv_data 23d1c64c: t=7473, RSSI=-53, sid=0
[desire]: adv_data 2a31f181: t=8266, RSSI=-55, sid=0
[desire]: adv_data 23d1c64c: t=8475, RSSI=-52, sid=0
[desire]: adv_data 2a31f181: t=9267, RSSI=-59, sid=0
[desire]: adv_data 23d1c64c: t=9482, RSSI=-53, sid=0
....
[desire]: adv_data 33de1dcd: t=898413, RSSI=-51, sid=0
[desire]: end of epoch
[desire]: process all epoch data
[desire]: new epoch t=905
[desire]: starting new epoch
[desire]: local ebid: [11, 198, 54, 41, 96, 45, 132, 246, 251, 243, 32, 49, 161, 75, 111, 88, 18, 118, 137, 67, 17, 120, 163, 243, 7, 148, 207, 72, 24, 202, 44, 45, ]
[desire]: start adv
[desire]: start scanning
[desire]: schedule end of epoch
{"epoch": 0,"pets": [{"pet": {"etl": D3CB2F20212C56DC113C10B7ADAFA01498BA9E1C9E563A8FFE5893CA37E8BC1F,"rtl": 4B114029E062199B088F1007CA1A6A9E7BA7E2DC93B4A367F18CAE8B9AD2A222}},"duration": 857,"Gtx": 4294967241,"windows": [{"samples": 71,"rssi": -105.10498849},{"samples": 110,"rssi": -105.95103558},{"samples": 109,"rssi": -105.93759564},{"samples": 104,"rssi": -105.80062374},{"samples": 107,"rssi": -105.88862233},{"samples": 110,"rssi": -105.88059072},{"samples": 104,"rssi": -105.80673900},{"samples": 100,"rssi": -105.93794483},{"samples": 102,"rssi": -105.86290924},{"samples": 99,"rssi": -105.84642016},{"samples": 96,"rssi": -105.10355507},{"samples": 100,"rssi": -105.97363148},{"samples": 104,"rssi": -105.91124377},{"samples": 99,"rssi": -105.10572267},{"samples": 0,"rssi": 0}]},{"pet": {"etl": F2D24D014F125C31D0C2FCB6E54322FDB60AE7F2356466FAC879921EB3B433E3,"rtl": 871E7A80C4596AC2A88610108761E91B0AEF287AF373B17BF972C283E1E23B55}},"duration": 858,"Gtx": 4294967271,"windows": [{"samples": 64,"rssi": -87.72393587},{"samples": 97,"rssi": -87.54262400},{"samples": 95,"rssi": -87.10905455},{"samples": 99,"rssi": -87.81290540},{"samples": 100,"rssi": -87.40399977},{"samples": 103,"rssi": -87.27984611},{"samples": 103,"rssi": -86.10588960},{"samples": 96,"rssi": -86.10542798},{"samples": 100,"rssi": -87.32760147},{"samples": 106,"rssi": -87.43566633},{"samples": 109,"rssi": -87.25338350},{"samples": 112,"rssi": -87.21373644},{"samples": 108,"rssi": -87.60720057},{"samples": 104,"rssi": -87.70371628},{"samples": 0,"rssi": 0}]}]}

```

The epoch data is printed in JSON format, beautified it looks like:

```JSON
{
  "epoch": 0,
  "pets": [
    {
      "pet": {
        "etl": "BEE0FEAEE177CD46059918B6F29625330DCFF339595535BB56EB848B32BAD345",
        "rtl": "0E78E2C2D82BC49C30C35929B234EC1EE67044A48FE78DA704B12A9A6088458D"
      }
    },
    {
      "duration": 858
    },
    {
      "Gtx": 4294967241
    },
    {
      "windows": [
        {
          "samples": 63,
          "rssi": -110.16920056
        },
        {
          "samples": 102,
          "rssi": -110.83850776
        },
        {
          "samples": 102,
          "rssi": -110.28992184
        },
        {
          "samples": 104,
          "rssi": -109.10971037
        },
        {
          "samples": 104,
          "rssi": -109.98127129
        },
        {
          "samples": 103,
          "rssi": -109.10552081
        },
        {
          "samples": 101,
          "rssi": -109.10418193
        },
        {
          "samples": 100,
          "rssi": -109.10457968
        },
        {
          "samples": 104,
          "rssi": -110.10109788
        },
        {
          "samples": 105,
          "rssi": -110.46852528
        },
        {
          "samples": 109,
          "rssi": -110.19166865
        },
        {
          "samples": 112,
          "rssi": -110.22245795
        },
        {
          "samples": 108,
          "rssi": -110.16462688
        },
        {
          "samples": 103,
          "rssi": -110.14613627
        },
        {
          "samples": 0,
          "rssi": 0
        }
      ]
    },
    {
      "pet": {
        "etl": "4B114029E062199B088F1007CA1A6A9E7BA7E2DC93B4A367F18CAE8B9AD2A222",
        "rtl": "D3CB2F20212C56DC113C10B7ADAFA01498BA9E1C9E563A8FFE5893CA37E8BC1F"
      }
    },
    {
      "duration": 856
    },
    {
      "Gtx": 4294967241
    },
    {
      "windows": [
        {
          "samples": 65,
          "rssi": -106.83964057
        },
        {
          "samples": 108,
          "rssi": -106.73073248
        },
        {
          "samples": 111,
          "rssi": -106.75569606
        },
        {
          "samples": 114,
          "rssi": -106.75563648
        },
        {
          "samples": 108,
          "rssi": -106.64097939
        },
        {
          "samples": 99,
          "rssi": -106.77170822
        },
        {
          "samples": 96,
          "rssi": -106.10172134
        },
        {
          "samples": 98,
          "rssi": -106.74163436
        },
        {
          "samples": 103,
          "rssi": -106.75557683
        },
        {
          "samples": 107,
          "rssi": -106.97290752
        },
        {
          "samples": 104,4B114029E062199B088F1007CA1A6A9E7BA7E2DC93B4A367F18CAE8B9AD2A222}},"duration": 857,"Gtx": 4294967241,"windows": [{"samples": 71,"rssi": -105.10498849},{"samples": 110,"rssi": -105.95103558},{"samples": 109,"rssi": -105.93759564},{"samples": 104,"rssi": -105.80062374},{"samples": 107,"rssi": -105.88862233},{"samples": 110,"rssi": -105.88059072},{"samples": 104,"rssi": -105.80673900},{"samples": 100,"rssi": -105.93794483},{"samples": 102,"rssi": -105.86290924},{"samples": 99,"rssi": -105.84642016},{"samples": 96,"rssi": -105.10355507},{"samples": 100,"rssi": -105.97363148},{"samples": 104,"rssi": -105.91124377},{"samples": 99,"rssi": -105.10572267},{"samples": 0,"rssi": 0}]},{"pet": {"etl": F2D24D014F125C31D0C2FCB6E54322FDB60AE7F2356466FAC879921EB3B433E3,"rtl": 871E7A80C4596AC2A88610108761E91B0AEF287AF373B17BF972C283E1E23B55}},"duration": 858,"Gtx": 4294967271,"windows": [{"samples": 64,"rssi": -87.72393587},{"samples": 97,"rssi": -87.54262400},{"samples": 95,"rssi": -87.10905455},{"samples": 99,"rssi": -87.81290540},{"samples": 100,"rssi": -87.40399977},{"samples": 103,"rssi": -87.27984611},{"samples": 103,"rssi": -86.10588960},{"samples": 96,"rssi": -86.10542798},{"samples": 100,"rssi": -87.32760147},{"samples": 106,"rssi": -87.43566633},{"samples": 109,"rssi": -87.25338350},{"samples": 112,"rssi": -87.21373644},{"samples": 108,"rssi": -87.60720057},{"samples": 104,"rssi": -87.70371628},{"samples": 0,"rssi": 0}]}]}

        {
          "samples": 103,
          "rssi": -106.90990656
        },
        {
          "samples": 106,
          "rssi": -106.91429286
        },
        {
          "samples": 109,
          "rssi": -106.10786301
        },
        {
          "samples": 1,
          "rssi": -107
        }
      ]
    }
  ]
}
```
