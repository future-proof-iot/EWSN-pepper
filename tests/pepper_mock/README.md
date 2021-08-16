# Native Networking SetUp

$ sudo ip tuntap add tap0 mode tap user $USER
$ sudo ip link set tap0 up
$ sudo ip address add 2001:db8::1/64 dev tap0

# Hardware Networking SetUp

On one terminal (keep open)
$ sudo ${RIOTBASE}/dist/tools/ethos/setup_network.sh riot0 2002:db8::/64
