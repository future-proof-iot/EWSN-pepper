#!/usr/bin/env python3

# Copyright (C) 2019 Inria
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

import os
import subprocess
import sys
import tempfile
import time

from testrunner import run
from testrunner import utils

# Default test over loopback interface
BOARD = os.getenv('BOARD', 'native')
COAP_HOST = "[2001:db8::1]" if BOARD == 'native' else "[fd00:dead:beef::1]"

DEFAULT_TAP = "tap0" if BOARD == 'native' else "riot0"
TAP = os.getenv("TAP", DEFAULT_TAP)
TMPDIR = tempfile.TemporaryDirectory()


def start_aiocoap_fileserver():
    aiocoap_process = subprocess.Popen(
        "exec aiocoap-fileserver %s" % TMPDIR.name, shell=True
    )

    return aiocoap_process


def cleanup(aiocoap_process):
    aiocoap_process.kill()
    TMPDIR.cleanup()


def notify(coap_server, client_url, version=None):
    cmd = [
        "make",
        "suit/notify",
        "SUIT_COAP_SERVER={}".format(coap_server),
        "SUIT_CLIENT={}".format(client_url),
    ]
    if version is not None:
        cmd.append("SUIT_NOTIFY_VERSION={}".format(version))
    assert not subprocess.call(cmd)


def publish(server_dir, server_url, distance=200, keys='default'):
    cmd = [
        "make",
        "suit/publish",
        "BPF_CFLAGS=-DMAX_DISTANCE_CM={}".format(distance),
        "SUIT_COAP_FSROOT={}".format(server_dir),
        "SUIT_COAP_SERVER={}".format(server_url),
        "RIOTBOOT_SKIP_COMPILE=1",
        "SUIT_KEY={}".format(keys),
    ]

    assert not subprocess.call(cmd)


def get_ipv6_addr(child):
    child.expect_exact('>')
    child.sendline('ifconfig')
    if BOARD == "native":
        # Get device global address
        child.expect(
            r"inet6 addr: (?P<gladdr>[0-9a-fA-F:]+:[A-Fa-f:0-9]+)"
            "  scope: global  VAL"
        )
        addr = child.match.group("gladdr").lower()
    else:
        # Get device local address
        child.expect_exact("Link type: wired")
        child.expect(
            r"inet6 addr: (?P<lladdr>[0-9a-fA-F:]+:[A-Fa-f:0-9]+)"
            "  scope: link  VAL"
        )
        addr = "{}%{}".format(child.match.group("lladdr").lower(), TAP)
    return addr


def ping6(client):
    print("pinging node...")
    ping_ok = False
    for _i in range(10):
        try:
            subprocess.check_call(["ping", "-q", "-c1", "-w1", client])
            ping_ok = True
            break
        except subprocess.CalledProcessError:
            pass

    if not ping_ok:
        print("pinging node failed. aborting test.")
        sys.exit(1)
    else:
        print("pinging node succeeded.")
    return ping_ok


def get_reachable_addr(child):
    # Give some time for the network interface to be configured
    time.sleep(1)
    # Get address
    client_addr = get_ipv6_addr(child)
    # Verify address is reachable
    ping6(client_addr)
    return "[{}]".format(client_addr)


def _test_update_distance_threshold(child, client):
    child.sendline("run")
    child.expect_exact("uwb_ed_finish_bpf: d=(200), exposure=(601), valid=(1)")
    publish(TMPDIR.name, COAP_HOST, distance=100)
    notify(COAP_HOST, client)
    child.expect_exact("[uwb_ed_bpf]: lock region")
    child.expect_exact("[uwb_ed_bpf]: unlock region")
    child.sendline("run")
    child.expect_exact("uwb_ed_finish_bpf: d=(200), exposure=(601), valid=(0)")


def testfunc(child):
    # Verify client is reachable and get address
    client = get_reachable_addr(child)

    def run(func):
        if child.logfile == sys.stdout:
            func(child, client)
        else:
            try:
                func(child, client)
                print(".", end="", flush=True)
            except Exception as e:
                print("FAILED")
                raise e

    run(_test_update_distance_threshold)
    print("TEST PASSED")


if __name__ == "__main__":
    try:
        res = 1
        aiocoap_process = start_aiocoap_fileserver()
        # TODO: wait for coap port to be available
        res = run(testfunc, echo=False)

    except Exception as e:
        print(e)
    finally:
        cleanup(aiocoap_process)

    sys.exit(res)
