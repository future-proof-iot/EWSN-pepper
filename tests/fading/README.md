## Fading Test

This application want's to best the cost of fading on the target MCU. The
fading is done according to https://hal.inria.fr/hal-02641630/document.

It will bench the worst case scenario for each window, so:

    - 120 messages * 15 windows

Arbitrarily 100 encounters are used to aggregate.

### Expected Output

```
# main(): This is RIOT! (Version: 2021.01-devel-2355-g9e91e9-HEAD)
# Fading test
# 120 messages:   00019 [ms]
#  15 windows:    00272 [ms]
# 100 encounters: 27172 [ms]
```
