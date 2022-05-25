# Copyright (C) 2022 Inria
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

"""
Current Time shell interaction
"""
import re
from datetime import datetime
from riotctrl.shell import ShellInteraction, ShellInteractionParser


class CurrentTimeParser(ShellInteractionParser):
    date_c = re.compile(r"Date:\s+(?P<date>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$)")
    epoch_c = re.compile(r"Epoch:\s+(?P<epoch>\d+)$")

    DATE_TIME_STR = "%Y-%m-%d %H:%M:%S"

    def parse(self, cmd_output):
        current_time = {"epoch": None, "date": None}
        for line in cmd_output.splitlines():
            m = self.epoch_c.search(line)
            if m is not None:
                current_time["epoch"] = int(m.group("epoch"))
            m = self.date_c.search(line)
            if m is not None:
                current_time["date"] = datetime.strptime(
                    m.group("date"), self.DATE_TIME_STR
                )
        return current_time


class CurrentTimeCmd(ShellInteraction):
    @ShellInteraction.check_term
    def current_time_get(self, timeout=-1, async_=False):
        return self.cmd("time", timeout=timeout, async_=async_)

    @ShellInteraction.check_term
    def current_time_set(self, datetime: datetime, timeout=-1, async_=False):
        now = f"time {datetime.strftime('%Y %m %d %H %M %S')}"
        return self.cmd(now, timeout=timeout, async_=async_)

    def current_time_set_now(self, timeout=-1, async_=False):
        return self.current_time_set(
            datetime=datetime.today(), timeout=timeout, async_=async_
        )


