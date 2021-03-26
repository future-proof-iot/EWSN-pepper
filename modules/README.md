# riot-desire Modules

This directory holds the different firmware modules.

## [`crypto_manager`](sys/crypto_manager/include/crypto_manager.h)

This modules allows generating elliptic curve Curve25519 based public/secret key pair. It can also generate Private Encounter Tokens from a
give public/secret key pair and an encountered EBID (which is nothing
else than a public key). For more refer to (1).

### usage

See [doc](sys/crypto_manager/include/crypto_manager.h)

## [`ebid`](sys/crypto_manager/include/ebid.h)

This modules allows generating an Ephemeral Bluetooth Id from a given
public/secret key pair. Elliptic Curve Cryptography and more specifically the elliptic curve Curve25519 is supposed to be used for
the key generation. This is handled by the [`crypto_manager`]() module.
The module also holds functions to reconstruct a partially received EBID
(in case of carrousel advertisement of EBID slices). For more refer to (1).

### usage

See [doc](sys/crypto_manager/include/ebid.h)

# References

(1) [DESIRE : Leveraging the best of centralized and decentralized contact tracing systems]()
