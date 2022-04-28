# Copyright (C) 2021 Inria
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

"""
PEPPER shell interactions
"""

from dataclasses import dataclass
import logging
import re

from pepper_data.datum import Datums, DebugDatums
from pepper_data.epoch import EpochData

from riotctrl.shell import ShellInteraction, ShellInteractionParser


LOG_HANDLER = logging.StreamHandler()
LOG_HANDLER.setFormatter(logging.Formatter(logging.BASIC_FORMAT))
LOG_LEVELS = ("debug", "info", "warning", "error", "fatal", "critical")
LOGGER = logging.getLogger()


@dataclass
class PepperParams:
    iterations: int = 0
    duration: int = 900
    adv_itvl: int = 1000
    advs_slice: int = 20
    scan_itvl: int = 4096
    scan_win: int = 1024
    align_start: bool = False
    align_end: bool = False


class PepperStatusParser(ShellInteractionParser):
    status_c = re.compile(r"status:\s+(?P<status>[A-Za-z]+)\s")

    def parse(self, cmd_output):
        status = {
            "active": False,
        }
        for line in cmd_output.splitlines():
            m = self.status_c.search(line)
            if m is not None:
                status["active"] = True if m.group("status") == "active" else False
        return status


class PepperStartParser(ShellInteractionParser):
    def parse(self, cmd_output):
        datums = list()
        epoch_data = list()
        for line in cmd_output.splitlines():
            LOGGER.debug(f"Parsing line: {line}")
            try:
                data = Datums.from_json_str(line)
                datums.append(data)
            except:
                LOGGER.debug(f"JSONDecodeError on: '{line}'")
            try:
                data = EpochData.from_json_str(line)
                epoch_data.append(data)
            except:
                LOGGER.debug(f"JSONDecodeError on: '{line}'")
        return epoch_data, DebugDatums.from_datums(datums)


class PepperCmd(ShellInteraction):
    @ShellInteraction.check_term
    def pepper_cmd(self, args=None, timeout=-1, async_=False):
        cmd = "pepper"
        if args is not None:
            cmd += " {args}".format(args=" ".join(str(a) for a in args))
        return self.cmd(cmd, timeout=timeout, async_=async_)

    def pepper_stop(self, on=True, timeout=-1, async_=False):
        return self.pepper_cmd(args=("stop",), timeout=timeout, async_=async_)

    def pepper_start(
        self,
        params: PepperParams,
        timeout=None,
        async_=False,
    ):
        if timeout is None:
            if params.iterations != 0:
                timeout = params.duration * params.iterations * 1.1 + 3
            else:
                timeout = -1

        return self.pepper_cmd(
            args=(
                "start",
                f"-d {params.duration}",
                f"-i {params.adv_itvl}",
                f"-r {params.advs_slice}",
                f"-c {params.iterations}",
                f"-s {params.scan_win},{params.scan_itvl}",
                f"-e" if params.align_end else "",
                f"-a" if params.align_start else "",
            ),
            timeout=timeout,
            async_=async_,
        )

    def pepper_set(self, key, value, timeout=-1, async_=False):
        return self.pepper_cmd(args=("set", key, value), timeout=timeout, async_=async_)

    def pepper_get(self, key, timeout=-1, async_=False):
        return self.pepper_cmd(args=("get", key), timeout=timeout, async_=async_)

    def pepper_status(self, timeout=-1, async_=False):
        return self.pepper_cmd(args=("status",), timeout=timeout, async_=async_)
