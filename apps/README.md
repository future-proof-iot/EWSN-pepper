# PEPPER Applications

This directory holds a collection of applications:

- [pepper_desire](./pepper_desire/README.md): an implementation of the DESIRE contact tracing protocol for RIOT devices
- [pepper_simple](./pepper_simple/README.md): minimal PEPPER PoC application (UWB based proximity tracing), contact information is only offloaded over serial.
- [pepper_iotlab](./pepper_iotlab/README.md): compared to 'pepper_simple' it only logs contact information which include UWB exchanges stats. It's also tuned to work with many nodes (buffer size and others).
- [pepper_experience](./pepper_experience/README.md): compared to 'pepper_simple' it logs BLE and UWB data, UWB exchanges stats and it also includes features that are use-full for scripted measurement collection. Default parameters are toggled to generate data rapidly.
- [pepper_pm](./pepper_pm/README.md): similar to 'pepper_simple' but it activate modules that but the DW1000 module to sleep when unused. It also increases buffer size for encounters to handle many neighbors.
- [pepper_field](./pepper_field/README.md): similar to 'pepper_pm' but will log all contact information as well as UWB and BLE data to an SDCARD if present. It also includes a GATT end-point for configuration.
- [pepper_demo](./pepper_demo/README.md): a full pepper demo including contact information offloading to a CoAP server


Others:
- [blebr](./blebr/README.md): a IPv6 over BLE border router application.
- [pepper_riotfp](./pepper_riotfp/README.md): deprecated, some of the used futures are no longer supported.
