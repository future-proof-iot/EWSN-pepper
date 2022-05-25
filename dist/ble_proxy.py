#!/usr/bin/env python3

# Copyright (C) 2021 Inria
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

"""
- author:      Roudy DAGHER <roudy.dagher@inria.fr>
"""

import asyncio
import sys
import traceback

from cmd import Cmd
import argparse

from ble_iface import BLEScanner, PEPPERConfig, PEPPERNode
from ble_iface import PEPPERStart

CONFIG_EPOCH_DURATION_SEC = 2.5 * 60
CONFIG_BLE_ADV_ITVL_MS = 1000
CONFIG_BLE_SCAN_ITVL_MS = 1024
CONFIG_BLE_SCAN_WIN_MS = 1024
CONFIG_ADV_PER_SLICE = 5

# decorator to ease documenting CLI commands with f-strings
def add_doc(value):
    def _doc(func):
        func.__doc__ = value
        return func

    return _doc


class WrapperCmdLineArgParser:
    def __init__(self, parser):
        """Init decorator with an argparse parser to be used in parsing cmd-line options"""
        self.parser = parser
        self.help_msg = ""

    def __call__(self, f):
        """Decorate 'f' to parse 'line' and pass options to decorated function"""
        if not self.parser:  # If no parser was passed to the decorator, get it from 'f'
            self.parser = f(None, None, None, True)

        def wrapped_f(*args):
            line = args[1].split()
            try:
                parsed = self.parser.parse_args(line)
            except SystemExit:
                return
            f(*args, parsed=parsed)

        wrapped_f.__doc__ = self.__get_help(self.parser)
        return wrapped_f

    @staticmethod
    def __get_help(parser):
        """Get and return help message from 'parser.print_help()'"""
        return parser.format_help()


# Application Shell (CLI)
class BLEPrompt(Cmd):
    def __init__(self, loop=None):
        Cmd.__init__(self)
        self.ble_scanner = BLEScanner()
        self.ble_nodes = []
        self.current_node = None
        if loop == None:
            self.loop = asyncio.get_event_loop()
        else:
            self.loop = loop
        self.intro = "Welcome to RIOT's PEPPER configuration over BLE shell! Type ? to list commands"
        self.prompt = "pepper-gatt> "
        self.do_EOF = self.do_exit

    def emptyline(self):
        # do noting on empty line
        pass

    def do_connect(self, arg: str):
        """Connects to all scanned nodes"""
        out = None
        try:
            for node in self.ble_nodes:
                if node.is_connected:
                    print(f"node {node.name} is already connected")
                    continue
                print(f"Connecting to node: {node.name}")
                self.loop.run_until_complete(node.connect())
                print(f"\tConnected to node: {node.name}")
        except Exception as e:
            print(f"[Error] BLE failure: {e}")
            print(traceback.format_exc())
            out = e

        return out

    def do_disconnect(self, arg: str):
        """Disconnects from all scanned nodes"""
        out = None
        try:
            for node in self.ble_nodes:
                if node.is_connected:
                    print(f"Disconnecting from node: {node.name}")
                    self.loop.run_until_complete(node.disconnect())
                    print(f"\tDisconnected from node: {node.name}")
        except Exception as e:
            print(f"[Error] BLE failure: {e}")
            print(traceback.format_exc())
            out = e

        return out

    @WrapperCmdLineArgParser(parser=None)
    def do_scan(self, line: str, parsed, get_parser=False):
        """Scans for BLE nodes."""
        if get_parser:
            parser = argparse.ArgumentParser(prog="scan")
            parser.add_argument(
                "--duration", "-d", default=1, type=int, help="Scan duration in seconds"
            )
            parser.add_argument(
                "--log",
                action="store_true",
                help="Flag to log detection events",
            )
            parser.add_argument(
                "--no-log",
                dest="log",
                default=False,
                action="store_false",
                help="Flag to silence detection events",
            )
            return parser
        try:
            # disconnect from nodes
            ex = self.do_disconnect("")
            if ex is not None:
                raise (ex)

            self.current_node = None
            self.ble_nodes = []
            nodes = self.loop.run_until_complete(
                self.ble_scanner.scan(
                    scan_duration=parsed.duration, log_detections=parsed.log
                )
            )
            self.ble_nodes = [PEPPERNode(n) for n in nodes]
            self.do_ls(line="")
        except Exception as e:
            print(f"[Error] BLE scan failure: {e}")
            traceback.format_exc()

    def do_ls(self, line: str):
        """Lists discovered BLE nodes."""
        print(f"Discovered {len(self.ble_nodes)} Nodes:")
        for i, node in enumerate(self.ble_nodes):
            sep = "\t* " if self.current_node == self.ble_nodes[i] else "\t  "
            print(f"[{i}]{sep}: {node.device}")

    def do_select(self, line: str):
        """Selects a BLE node from discovered nodes"""
        if line == "":
            print(
                "Select one of discovered nodes by index (currently selected node has a *)"
            )
            self.do_ls("")
        else:
            args = line.split(" ")
            if len(args) != 0:
                try:
                    idx = int(args[0])
                    self.current_node = self.ble_nodes[idx]
                except ValueError:
                    print(f"[Error]: provided index {args[0]} is not an integer")
                except IndexError:
                    err_msg = (
                        "No scanned nodes to select"
                        if len(self.ble_nodes) == 0
                        else f" Must be in [0-{len(self.ble_nodes)-1}]"
                    )
                    print(f"[Error]: provided index {args[0]} out of range. {err_msg}")
                if not self.current_node.is_connected:
                    try:
                        print(f"Connecting to node: {self.current_node.name}")
                        self.loop.run_until_complete(self.current_node.connect())
                        print(f"\tConnected to node: {self.current_node.name}")
                    except Exception as e:
                        print(f"[Error] BLE failure: {e}")
                        print(traceback.format_exc())
                        out = e

    def do_discover(self, line: str):
        """Discovers the selected BLE node by printing its services and characteristics"""
        if self.current_node is None:
            print("Select a node first")
            self.do_ls("")
            return

        try:
            self.loop.run_until_complete(
                self.ble_scanner.discover(self.current_node.client)
            )
        except Exception as e:
            print(f"[Error] BLE discovery failure: {e}")
            print(traceback.format_exc())

    def do_stop(self, line: str):
        """Performs an ifconfig over BLE on selected node"""
        if self.current_node is None:
            print("Select a node first")
            self.do_ls("")
            return

        node = self.current_node
        try:
            self.loop.run_until_complete(node.write_pepper_stop())
        except Exception as e:
            print(f"[Error] BLE scan failure: {e}")
            print(traceback.format_exc())

    @WrapperCmdLineArgParser(parser=None)
    def do_config(self, line: str, parsed, get_parser=False):
        """Configure PEPPER"""
        if get_parser:
            parser = argparse.ArgumentParser(prog="config")
            parser.add_argument(
                "--base-name",
                "-bn",
                default=None,
                help="Base name to configure",
            )
            return parser

        if self.current_node is None:
            print("Select a node first")
            self.do_ls("")
            return

        node = self.current_node

        # Write new value if needed
        if parsed.base_name:
            try:
                config = PEPPERConfig(base_name=parsed.base_name)
                self.loop.run_until_complete(node.write_pepper_config(config=config))
            except Exception as e:
                print(f"[Error] BLE failure: {e}")
                print(traceback.format_exc())
        # Read Current value
        try:
            base_name = self.loop.run_until_complete(node.read_pepper_config())
            print(f"Base name: {base_name.base_name}")
        except Exception as e:
            print(f"[Error] BLE read_rng_conf failure: {e}")
            print(traceback.format_exc())

    @WrapperCmdLineArgParser(parser=None)
    def do_start(self, line: str, parsed, get_parser=False):
        """Starts PEPPER"""
        if get_parser:
            parser = argparse.ArgumentParser(prog="start")
            parser.add_argument(
                "--counts",
                "-c",
                type=int,
                default=0,
                help="Epoch iterations, 0 to run until stopped",
            )
            parser.add_argument(
                "--adv-interval",
                "-i",
                type=int,
                default=CONFIG_BLE_ADV_ITVL_MS,
                help="Advertisement interval in milliseconds",
            )
            parser.add_argument(
                "--scan-window",
                "-sw",
                type=int,
                default=CONFIG_BLE_SCAN_WIN_MS,
                help="Scan Window in milliseconds",
            )
            parser.add_argument(
                "--scan-interval",
                "-si",
                type=int,
                default=CONFIG_BLE_SCAN_ITVL_MS,
                help="Scan interval in milliseconds",
            )
            parser.add_argument(
                "--adv-rotation",
                "-r",
                type=int,
                default=CONFIG_ADV_PER_SLICE,
                help="Advertisement per slice",
            )
            parser.add_argument(
                "--duration",
                "-d",
                type=int,
                default=CONFIG_EPOCH_DURATION_SEC,
                help="Epoch duration in seconds",
            )
            parser.add_argument(
                "--align", "-e", action="store_true", default=False, help="Align end of Epoch"
            )
            return parser

        if self.current_node is None:
            print("Select a node first")
            self.do_ls("")
            return

        node = self.current_node

        # Default configuration
        epoch_duration_s = parsed.duration
        epoch_iterations = parsed.counts
        adv_itvl_ms = parsed.adv_interval
        scan_itvl_ms = parsed.scan_interval
        scan_win_ms = parsed.scan_window
        advs_per_slice = parsed.adv_rotation
        align = parsed.align
        try:
            start = PEPPERStart(
                epoch_duration_s=epoch_duration_s,
                epoch_iterations=epoch_iterations,
                adv_itvl_ms=adv_itvl_ms,
                scan_itvl_ms=scan_itvl_ms,
                scan_win_ms=scan_win_ms,
                advs_per_slice=advs_per_slice,
                align=align,
            )
            print(start)
            self.loop.run_until_complete(node.write_pepper_start(start=start))
        except Exception as e:
            print(f"[Error] BLE failure: {e}")
            print(traceback.format_exc())

    def do_exit(self, arg):
        """exit the application."""
        print("Bye")
        return True


def main(args):
    # CLI loop
    cli = BLEPrompt()
    try:
        cli.cmdloop()
    finally:
        cli.do_disconnect("")


if __name__ == "__main__":
    main(sys.argv)
