# BLE time server

This application sets up a current-time BLE advertiser.

## Notes

Compare to the [time_advertiser](../time_advertiser) application this has been
re-written to use extended advertisements, this allows for the time advertisement
logic to run in parallel with e.g.: `stdio_nimble` or `nimble_netif`.

## TODO:

- The application was written with re-usability in mind so the first half of the
code could and should be split into its own module that allows for extended auto-adv
where a user callback can be called after every advertisement event completion. In
this case such callback changes the advertisement payload to the current system time.
- Improve this application and turn it into a wall-clock.

## Usage

0. get/set the time

```
> time
time
Current Time =  2020-01-01 00:00:17, epoch: 17
> time 2021 08 16 14 08 00
time 2021 08 16 14 08 00
```

0. start/stop advertising

```
> start
start
[Tick]
Current Time =  2021-08-16 14:20:07, epoch: 51286807
[Tick]
Current Time =  2021-08-16 14:20:08, epoch: 51286808
[Tick]
Current Time =  2021-08-16 14:20:09, epoch: 51286809
[Tick]
Current Time =  2021-08-16 14:20:10, epoch: 51286810
> stop
stop
Stopped ongoing advertisements (if any)
```

start with a given advertisement period in seconds

```
> start 10
start 10
> [Tick]
Current Time =  2021-08-16 14:22:00, epoch: 51286920
```
