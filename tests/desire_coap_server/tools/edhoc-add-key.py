#!/usr/bin/env python3

import argparse
import base64
from security.edhoc_keys import (add_peer_cred)


parser = argparse.ArgumentParser()
parser.add_argument("key", type=str, help="the base64 encoded RPK")
parser.add_argument("kid", type=str, help="the base64 kid for the RPK")


def main(key, kid):
    """Main function."""
    key = base64.b64decode(key.encode())
    kid = base64.b64decode(kid.encode())
    add_peer_cred(key, kid)

    message = ("EDHOC added key:\n\n"
               "   - RPK: \t\n{} kid:{}\n\n")

    print(message.format(key, kid))


if __name__ == "__main__":
    args = parser.parse_args()
    main(args.key, args.kid)
