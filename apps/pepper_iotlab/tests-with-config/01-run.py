#! /usr/bin/env python3

# Copyright (C) 2021 Inria
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

from datetime import timedelta
import logging
import sys
import unittest
import re

from riotctrl.ctrl import RIOTCtrl
from riotctrl_shell.sys import Reboot
from pepperctrl_shell.pepper_shell import PepperCmd, PepperStatusParser, PepperParams
from pepperctrl_shell.current_time_shell import CurrentTimeCmd, CurrentTimeParser


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


class TestPepperBase(unittest.TestCase):
    DEBUG = False

    @classmethod
    def setUpClass(cls):
        cls.ctrl = RIOTCtrl()
        cls.ctrl.reset()
        cls.ctrl.start_term()
        if cls.DEBUG:
            cls.ctrl.term.logfile = sys.stdout
        cls.shell = PepperShell(cls.ctrl)
        cls.logger = logging.getLogger(cls.__name__)
        if cls.DEBUG:
            cls.logger.setLevel(logging.DEBUG)

    @classmethod
    def tearDownClass(cls):
        cls.ctrl.stop_term()


class TestPepper(TestPepperBase):
    def test_get_uid(self):
        self.shell.parse_uid()
        assert self.shell.uid() is not None

    def test_get_status(self):
        parser = PepperStatusParser()
        status = parser.parse(self.shell.pepper_status())
        assert status["active"] is False

    def test_start_stop(self):
        params = PepperParams(
            duration=5, adv_itvl=100, advs_slice=1, scan_itvl=1024, scan_win=1024
        )
        out = self.shell.pepper_start(params)
        assert "[pepper] shell: start proximity tracing" in out
        out = self.shell.pepper_stop()
        assert "[pepper] shell: stop proximity tracing" in out


class TestCurrentTime(TestPepperBase):
    def test_set_get_time(self):
        parser = CurrentTimeParser()
        out = self.shell.current_time_get()
        old_time = parser.parse(out)
        datetime_new = old_time["date"] + timedelta(days=10)
        out = self.shell.current_time_set(datetime=datetime_new)
        new_time = parser.parse(out)
        assert new_time["date"] == datetime_new
        assert new_time["date"] != old_time["date"]


if __name__ == "__main__":
    unittest.main()
