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
from pepper_data.datum import Datum
from pepper_data.epoch import EpochData, NamedEpochData

import matplotlib.pyplot as plt

import numpy as np
import pandas as pd
import seaborn as sns
import re

LOG_HANDLER = logging.StreamHandler()
LOG_HANDLER.setFormatter(logging.Formatter(logging.BASIC_FORMAT))
LOG_LEVELS = ("debug", "info", "warning", "error", "fatal", "critical")
LOGGER = logging.getLogger("parser")

PARSER = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
PARSER.add_argument("files", nargs="+", help="Range measurements files")
PARSER.add_argument(
    "--loglevel", choices=LOG_LEVELS, default="info", help="Python logger log level"
)


def parse_logs(logfile:str) -> pd.DataFrame:
    df_columns=["rf", "env", "d_cm", "d_est", "rssi"]
    df_data = []
    with open(logfile, "r") as file:
        for line in file:
            try:
                data = EpochData.from_json_str(line)
                assert re.match(r'.*\-\d+', data.tag), 'Tag not compliant must be <env>-<distance_cm>'
                (env,d_cm) = data.tag.split('-')
                d_cm = int(d_cm) 
                d_ble = data.pets[0].pet.ble.avg_d_cm
                rssi_ble = data.pets[0].pet.ble.avg_rssi
                d_uwb =  data.pets[0].pet.uwb.avg_d_cm
                df_data.append(["ble", env, d_cm, d_ble, rssi_ble])
                df_data.append(["uwb", env, d_cm, d_uwb, np.NaN])
            except (ValueError, AssertionError):
                LOGGER.error(f"JSONDecodeError on: '{line}'")
            except AssertionError as e:
                LOGGER.error(f"{e}:Malformed tag on: '{line}'")

    df = pd.DataFrame(df_data, columns=df_columns)
    
    return df




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
        df = parse_logs(logfile)
        plt.figure()
        sns.barplot(x="d_cm", y="d_est", hue="env", data=df[df.rf=="ble"], linewidth=1, capsize=.1).set_title("BLE ranging")
        
        plt.figure()
        sns.barplot(x="d_cm", y="d_est", hue="env", data=df[df.rf=="uwb"], linewidth=1, capsize=.1).set_title("UWB ranging")

        g = sns.catplot(x="d_cm", y="d_est",
                hue="env", col="rf",
                data=df, kind="bar",
                margin_titles=True,
                linewidth=1, capsize=.1)
        g.fig.subplots_adjust(top=0.9)
        g.fig.suptitle("BLE vs UWB ranging")
    
    plt.show(block=True)
    input("Press enter to exit")


if __name__ == "__main__":
    main()
