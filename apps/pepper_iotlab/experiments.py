import argparse
import logging
import os
import sys
import re
import shutil
import asyncio

from contextlib import ExitStack
from typing import List
from riotctrl_shell.sys import Reboot

from pepper_data.epoch import NamedEpochData
from pepper_data.experiment_data import ExperimentData
from pepperctrl_shell.pepper_shell import PepperCmd, PepperStartParser, PepperParams
from pepperctrl_shell.current_time_shell import CurrentTimeCmd
from factories import RIOTCtrlAppFactory, IoTLABExperimentsFactory

DEFAULT_DIRECTORY = "testlogs"
DEFAULT_LOGFILE = "logs"
DEFAULT_DATAFILE = "out.log"
DEVNULL = open(os.devnull, "w")

LOG_HANDLER = logging.StreamHandler()
LOG_HANDLER.setFormatter(logging.Formatter(logging.BASIC_FORMAT))
LOG_LEVELS = ("debug", "info", "warning", "error", "fatal", "critical")
LOGGER = logging.getLogger()

DEFAULT_EPOCH_DURATION_SEC = 300
DEFAULT_BLE_ADV_ITVL_MS = 1000
DEFAULT_BLE_SCAN_ITVL_MS = 1024
DEFAULT_BLE_SCAN_WIN_MS = 1024
DEFAULT_ADV_PER_SLICE = 10
DEFAULT_EPOCHS_COUNT = 5

USAGE_EXAMPLE = """example:
"""

PARSER = argparse.ArgumentParser(
    formatter_class=argparse.RawDescriptionHelpFormatter, epilog=USAGE_EXAMPLE
)
PARSER.add_argument("app_dir", help="applications directory")
PARSER.add_argument(
    "--log-directory", "-ld", default=DEFAULT_DIRECTORY, help="Logs directory"
)
PARSER.add_argument("--logall", "-la", action="store_true", help="Log all data")
PARSER.add_argument("--datafile", "-df", default=DEFAULT_DATAFILE, help="Datafile name")
PARSER.add_argument(
    "--num-devices", "-n", type=int, default=5, help="dwm1001 devices to use"
)
PARSER.add_argument(
    "--iotlab-nodes",
    nargs="+",
    default=None,
    help="List of iotlab-nodes network-addresses",
)
PARSER.add_argument(
    "--loglevel",
    "-l",
    choices=LOG_LEVELS,
    default="info",
    help="Python logger log level",
)
PARSER.add_argument(
    "--counts",
    "-c",
    type=int,
    default=DEFAULT_EPOCHS_COUNT,
    help="Epoch iterations, 0 to run until stopped",
)
PARSER.add_argument(
    "--adv-interval",
    "-i",
    type=int,
    default=DEFAULT_BLE_ADV_ITVL_MS,
    help="Advertisement interval in milliseconds",
)
PARSER.add_argument(
    "--adv-rotation",
    "-r",
    type=int,
    default=DEFAULT_ADV_PER_SLICE,
    help="Advertisement per slice",
)
PARSER.add_argument(
    "--duration",
    "-d",
    type=int,
    default=DEFAULT_EPOCH_DURATION_SEC,
    help="Epoch duration in seconds",
)
PARSER.add_argument(
    "--scan-interval",
    "-s",
    type=int,
    default=DEFAULT_BLE_SCAN_ITVL_MS,
    help="Epoch duration in seconds",
)
PARSER.add_argument(
    "--scan-window",
    "-w",
    type=int,
    default=DEFAULT_BLE_SCAN_WIN_MS,
    help="Epoch duration in seconds",
)


APPLICATION = "."
CFLAGS_DEFAULT = "-DCONFIG_PEPPER_SHELL_BLOCKING=1 "


class PepperShell(Reboot, PepperCmd, CurrentTimeCmd):
    """Convenience class inheriting from the Reboot and TwrCmd shell"""

    _uid: None

    uid_c = re.compile(r"\[pepper\]: uid (DW[0-9A-Za-z]+)\s")

    def parse_uid(self):
        out = self.pepper_get("uid")
        m = self.uid_c.search(out)
        if m is not None:
            self._uid = m.group(1)

    def uid(self):
        return self._uid


def create_directory(directory, clean=False, mode=0o755):
    """Directory creation helper with `clean` option.

    :param clean: tries deleting the directory before re-creating it
    """
    if clean:
        try:
            shutil.rmtree(directory)
        except OSError:
            pass
    os.makedirs(directory, mode=mode, exist_ok=True)


def create_and_dump(data, directory, file_name):
    """Create file with name in 'directory' and dumps
    data do file
    """
    LOGGER.info("logging to file {}".format(file_name))
    file_path = os.path.join(directory, file_name)
    if os.path.exists(file_path):
        LOGGER.warning(f"File {file_name} already exists, overwriting")
    try:
        with open(file_path, "w") as f:
            for line in data:
                f.write("{}\n".format(line))
    except OSError as err:
        sys.exit("Failed to create a log file: {}".format(err))
    return file_path


async def finish_epoch(uid, exp_data: ExperimentData, logs: List, future):
    res = await future
    parser = PepperStartParser()
    epoch_data, _ = parser.parse(res)
    named_epoch_data = NamedEpochData(epoch_data, uid)
    exp_data.epoch_data.append(named_epoch_data)
    for line in res.splitlines():
        logs.append(line)


def run(devices, app_dir, params: PepperParams):
    """ """
    exp_data = ExperimentData(list(), list())
    print(exp_data.epoch_data)
    logs = list()
    # start iotlab experiment and recover list of nodes
    with ExitStack() as es:
        iotlab_factory = es.enter_context(IoTLABExperimentsFactory())
        LOGGER.info("SetUp IotLab Experiment")
        _, exp_envs = iotlab_factory.get_iotlab_experiment_nodes(devices)
        factory = es.enter_context(RIOTCtrlAppFactory())
        peppers = list()
        for i, _ in enumerate(devices):
            LOGGER.info(f"SetUp device {i + 1}/{len(devices)}")
            ctrl = factory.get_ctrl(
                application_directory=app_dir, env=exp_envs[i], cflags=CFLAGS_DEFAULT
            )
            shell = PepperShell(ctrl)
            shell.parse_uid()
            peppers.append(shell)
            LOGGER.info(f"Device: {shell.uid()} OK")

        futures = list()
        for pepper in peppers:
            LOGGER.info(f"Start pepper for {pepper.uid()}")
            out = pepper.pepper_start(params=params, async_=True)
            futures.append(finish_epoch(pepper.uid(), exp_data, logs, out))

        LOGGER.info("Wait for pepper to finish...")
        loop = asyncio.get_event_loop()
        loop.run_until_complete(asyncio.gather(*futures))

    return exp_data, logs


def main(args=None):
    args = PARSER.parse_args()

    # setup logger
    if args.loglevel:
        loglevel = logging.getLevelName(args.loglevel.upper())
        LOGGER.setLevel(loglevel)

    LOGGER.addHandler(LOG_HANDLER)
    LOGGER.propagate = False

    # parse args
    app_dir = args.app_dir
    log_directory = args.log_directory
    num_devices = args.num_devices
    iotlab_nodes = args.iotlab_nodes
    datafile = args.datafile
    logall = args.logall
    # parse pepper params
    params = PepperParams(
        iterations=args.counts,
        duration=args.duration,
        adv_itvl=args.adv_interval,
        advs_slice=args.adv_rotation,
        scan_itvl=args.scan_interval,
        scan_win=args.scan_window,
    )
    # create directory if non existant
    create_directory(log_directory)

    # setup experiments and run
    if iotlab_nodes:
        devices = iotlab_nodes
    else:
        devices = ["dwm1001"] * num_devices
    exp_data, logs = run(devices, app_dir, params)

    # dump json
    create_and_dump([exp_data.to_json_str()], log_directory, datafile)
    if logall:
        logfile = f"{datafile.split('.')[0]}_{DEFAULT_LOGFILE}.{datafile.split('.')[1]}"
        create_and_dump(logs, log_directory, logfile)


if __name__ == "__main__":
    main()
