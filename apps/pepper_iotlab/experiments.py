import argparse
import logging
import os
import sys
import re
import shutil
import asyncio
import time

from contextlib import ExitStack

from riotctrl_shell.sys import Reboot

from pepper_data.experiment_data import ExperimentData, ExperimentNode
from pepperctrl_shell.pepper_shell import PepperCmd, PepperStartParser, PepperParams
from pepperctrl_shell.current_time_shell import CurrentTimeCmd
from factories import FileCtrlEnvFactory, RIOTCtrlAppFactory, IoTLABCtrlEnvFactory

DEFAULT_DIRECTORY = "exp"
DEFAULT_LOGDUMP_FILE = "dump.log"
DEFAULT_DATA_FILE = "exp_data.log"
DEFAULT_BOARD_FILE = ".boards.yaml"
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
PARSER.add_argument(
    "--data-file", "-df", default=DEFAULT_DATA_FILE, help="Datafile name"
)
PARSER.add_argument(
    "--boards-file", "-bf", default=DEFAULT_BOARD_FILE, help="Datafile name"
)
PARSER.add_argument(
    "--iotlab-nodes-num", "-n", type=int, default=0, help="dwm1001 devices to use"
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


def cleanup_termlog(node: ExperimentNode, dir):
    """Removes non utf-8 characters that show up when connecting to node
    over IoT-LAB"""
    filename = os.path.join(dir, f"{node.node_id}.log")
    with open(filename, "r+", encoding="utf-8", errors="ignore") as fp:
        lines = fp.readlines()
        m = re.search(r".*(>.*$)", lines[0])
        if m:
            fp.seek(0)
            fp.truncate()
            fp.write(m.group(1) + "\n")
            fp.writelines(lines[1:])


async def finish_epoch(node: ExperimentNode, future):
    res = await future
    parser = PepperStartParser()
    epoch_data, debug_data = parser.parse(res)
    node.debug_data = debug_data
    node.epoch_data = epoch_data


def get_env_factory(**config):
    if "boards" in config:
        return IoTLABCtrlEnvFactory()
    elif "boards_file" in config:
        return FileCtrlEnvFactory()
    else:
        raise ValueError("Wrong Environment Factory")


def run(config, app_dir, log_dir, params: PepperParams):
    """ """
    exp_data = ExperimentData([])
    # start iotlab experiment and recover list of nodes
    with ExitStack() as es:
        LOGGER.info("Set up environment factory")
        env_factory = es.enter_context(get_env_factory(**config))
        factory = es.enter_context(RIOTCtrlAppFactory())
        # get environments and positions dicts
        envs_poss = env_factory.get_envs(**config)
        if len(envs_poss) <= 1:
            LOGGER.error("Not enough devices")
            return exp_data
        # get nodes positions and network addresses
        for i, env_pos in enumerate(envs_poss):
            # Set "TERMLOG"
            env = env_pos["env"]
            position = env_pos["position"]
            if "IOTLAB_NODE" in env:
                node_id = env["IOTLAB_NODE"]
            elif "BOARD_INDEX" in env:
                node_id = f"{env['BOARD']}_{env['BOARD_INDEX']}"
            elif "DEBUG_ADAPTER_ID" in env:
                node_id = f"{env['BOARD']}_{env['DEBUG_ADAPTER_ID']}"
            else:
                raise ValueError("Cant construct node_id")

            env["TERMLOG"] = os.path.join(log_dir, f"{node_id}.log")
            LOGGER.info(f"Flash device {i + 1}/{len(envs_poss)} : {node_id} ...")
            ctrl = factory.get_ctrl(app_dir=app_dir, env=env, cflags=CFLAGS_DEFAULT)
            shell = PepperShell(ctrl)
            # add ExperimentNode
            exp_data.nodes.append(
                ExperimentNode(
                    node_id=node_id,
                    position=position,
                    uid="",
                    debug_data=[],
                    epoch_data=[],
                    shell=shell,
                )
            )

        # give some time for all terminals to start
        time.sleep(3)

        for node in exp_data.nodes:
            node.shell.parse_uid()
            node.uid = node.shell.uid()
            node.shell.current_time_set_now()
            LOGGER.info(f"{node.node_id} [{node.uid}] OK")

        futures = list()
        for node in exp_data.nodes:
            LOGGER.info(f"start pepper for {node.node_id} [{node.uid}]")
            out = node.shell.pepper_start(params=params, async_=True)
            futures.append(finish_epoch(node, out))

        LOGGER.info("wait for pepper to finish...")
        loop = asyncio.get_event_loop()
        loop.run_until_complete(asyncio.gather(*futures))

    for node in exp_data.nodes:
        cleanup_termlog(node, log_dir)

    return exp_data


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
    log_directory = f"logs/{args.log_directory}"
    data_file = args.data_file

    iotlab_nodes_num = args.iotlab_nodes_num
    iotlab_nodes = args.iotlab_nodes
    # parse pepper params
    params = PepperParams(
        iterations=args.counts,
        duration=args.duration,
        adv_itvl=args.adv_interval,
        advs_slice=args.adv_rotation,
        scan_itvl=args.scan_interval,
        scan_win=args.scan_window,
        align_end=True,
        align_start=False,
    )
    # create directory if non existant
    create_directory(log_directory)

    if iotlab_nodes:
        config = {"boards": iotlab_nodes}
    elif iotlab_nodes_num:
        config = {"boards": ["dwm1001"] * iotlab_nodes_num}
    else:
        config = {"boards_file": args.boards_file}

    # setup experiments and run
    exp_data = run(config, app_dir, log_directory, params)
    for node in exp_data.nodes:
        node.shell = None  # not JSON serializable

    # dump json
    create_and_dump([exp_data.to_json_str()], log_directory, data_file)


if __name__ == "__main__":
    main()
