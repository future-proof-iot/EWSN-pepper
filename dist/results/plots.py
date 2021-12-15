#!/usr/bin/env python3

"""
This script parses end of epoch data from pepper applications serialized in JSON
format

Usage
-----

$ python plots.py static/dw5fc2-ordered.txt static/dwa5fde-ordered.txt
$ python dist/results/plots.py dist/results/static/dw5fc2-ordered.txt

usage: plots.py [-h] [--loglevel {debug,info,warning,error,fatal,critical}] files [files ...]

positional arguments:
  files                 Range measurements files

optional arguments:
  -h, --help            show this help message and exit
  --loglevel {debug,info,warning,error,fatal,critical}
                        Python logger log level (default: info)
"""
import argparse
import logging
from typing import Dict
from pepper.epoch_data import *

import matplotlib.pyplot as plt

LOG_HANDLER = logging.StreamHandler()
LOG_HANDLER.setFormatter(logging.Formatter(logging.BASIC_FORMAT))
LOG_LEVELS = ("debug", "info", "warning", "error", "fatal", "critical")
LOGGER = logging.getLogger("parser")

PARSER = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
PARSER.add_argument("files", nargs="+", help="Range measurements files")
PARSER.add_argument(
    "--loglevel", choices=LOG_LEVELS, default="info", help="Python logger log level"
)

def plot_range_histograms(data: Dict[str, int]):
    n = len(data)
    fig = plt.subplots(n, 1, sharey=True, sharex=True, tight_layout=True)
    for ax, node in zip(fig.axes, data):
        ax.hist(data[node]["raz"], label=node)
        ax.legend()
        ax.set_ylabel("frequency")
        ax.set_xlabel("Range (m)")
    fig.suptitle("Range distributions")


def plot_scatter(captures, title=None, ylabel=None, scale=1, ax=None):
    fig, ax = plt.subplots()
    for i, key in enumerate(captures.keys()):
        if i == 1:
            ax.legend()
        capture = captures[key]
        d_cm = int(key.split("-")[1])
        ax.scatter(
            [i] * len(capture["uwb_d"]),
            capture["uwb_d"],
            color="red",
            marker="o",
            label="uwb",
        )
        ax.scatter(
            [i] * len(capture["ble_d"]),
            capture["ble_d"],
            color="blue",
            marker="x",
            label="ble",
        )
        ax.scatter([i], [d_cm], color="black", label="truth")
    labels = captures.keys()
    ax.set_xticks(range(len(captures.keys())))
    ax.set_xticklabels(labels)
    if title:
        ax.set_title(title)
    if ylabel:
        ax.set_ylabel(ylabel)
    ax.grid(True)


def parse_logs(logfile):
    captures = dict()

    def append_epoch_data(data: EpochData):
        tag = data.tag
        if tag not in captures:
            captures[tag] = {
                "ble_d": [data.pets[0].pet.ble.avg_d_cm],
                "uwb_d": [data.pets[0].pet.uwb.avg_d_cm],
                "ble_rssi": [data.pets[0].pet.ble.avg_rssi],
            }
        captures[tag]["ble_d"].append(data.pets[0].pet.ble.avg_d_cm)
        captures[tag]["uwb_d"].append(data.pets[0].pet.uwb.avg_d_cm)
        captures[tag]["ble_rssi"].append(data.pets[0].pet.ble.avg_rssi)

    with open(logfile, "r") as file:
        for line in file:
            try:
                data = EpochData.from_json_str(line)
                append_epoch_data(data)
            except ValueError:
                LOGGER.error("JSONDecodeError on: '{}'".format(line))
    return captures


def main(args=None):
    args = PARSER.parse_args()

    # Setup logger
    if args.loglevel:
        loglevel = logging.getLevelName(args.loglevel.upper())
        LOGGER.setLevel(loglevel)

    LOGGER.addHandler(LOG_HANDLER)
    LOGGER.propagate = False

    logfiles = args.files
    for logfile in logfiles:
        captures = parse_logs(logfile)
        plot_scatter(captures, ylabel="d[cm]", title="BLE vs UWB distances")
    plt.show(block=False)
    input("Press enter to exit")


if __name__ == "__main__":
    main()
