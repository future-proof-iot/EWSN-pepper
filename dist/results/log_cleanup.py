#!/usr/bin/env python3

"""
This script parses dirty logfiles with some JSON data. Its used to parse
data generate by capturin RIOT devices logs, e.g.:

Usage
-----

$ TERMLOG=$(pwd)/scan_raw.log make term
$ python log_cleanup.py $(pwd)/scan_raw.log $(pwd)/scan_clean.log

usage: log_cleanup.py [-h] [--outfile OUTFILE]
                      [--loglevel {debug,info,warning,error,fatal,critical}]
                      infile

positional arguments:
  infile                Input file to clean

optional arguments:
  -h, --help            show this help message and exit
  --outfile OUTFILE     Output file name (default: cleanlog.json)
  --loglevel {debug,info,warning,error,fatal,critical}
                        Python logger log level (default: info)
"""
import argparse
import json
import logging

DEFAULT_OUTFILE = "cleanlog.json"

LOG_HANDLER = logging.StreamHandler()
LOG_HANDLER.setFormatter(logging.Formatter(logging.BASIC_FORMAT))
LOG_LEVELS = ("debug", "info", "warning", "error", "fatal", "critical")
LOGGER = logging.getLogger("parser")

PARSER = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
PARSER.add_argument("infile", help="Input file to clean")
PARSER.add_argument("--outfile", default=DEFAULT_OUTFILE, help="Output file name")
PARSER.add_argument(
    "--loglevel", choices=LOG_LEVELS, default="info", help="Python logger log level"
)


def parse_logs(infile, outfile):
    # Clean non unicode characters
    content = ""
    with open(infile, "rb") as f_raw:
        while 1:
            byte = f_raw.read(1)
            try:
                content += byte.decode()
            except UnicodeDecodeError:
                continue
            if not byte:
                break

    # Remove non json content
    output = []
    for line in content.split("\n"):
        try:
            json.loads(line)
        except json.JSONDecodeError:
            continue
        else:
            output.append(line + "\n")

    # Write to new log file
    with open(outfile, "w") as f_clean:
        f_clean.write("".join(output))


def main(args=None):
    args = PARSER.parse_args()

    # setup logger
    if args.loglevel:
        loglevel = logging.getLevelName(args.loglevel.upper())
        LOGGER.setLevel(loglevel)
    LOGGER.addHandler(LOG_HANDLER)
    LOGGER.propagate = False

    parse_logs(args.infile, args.outfile)


if __name__ == "__main__":
    main()
