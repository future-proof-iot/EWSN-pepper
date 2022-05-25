# Copyright (C) 2022 Inria
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

import datetime
import pytest
import pepperctrl_shell.current_time_shell as current_time_shell

from riotctrl_shell.tests.common import init_ctrl


FAKE_TIME = datetime.datetime(2022, 2, 14, 17, 5, 55)
FAKE_EPOCH = 67021555

@pytest.fixture
def patch_datetime_today(monkeypatch):
    class mydatetime:
        @classmethod
        def today(cls):
            return FAKE_TIME

    monkeypatch.setattr(datetime, "datetime", mydatetime)


def test_current_time_get():
    rc = init_ctrl(
        output="""
[current_time] shell:
      Date: 2022-02-14 17:05:55
      Epoch: 67021555
"""
    )
    shell = current_time_shell.CurrentTimeCmd(rc)
    out = shell.current_time_get()
    parser = current_time_shell.CurrentTimeParser()
    current_time = parser.parse(out)
    assert current_time["epoch"] == FAKE_EPOCH
    assert current_time["date"] == FAKE_TIME


def test_current_time_set(patch_datetime_today):
    rc = init_ctrl(
        output="""
[current_time] shell:
      Date: 2022-02-14 17:05:55
      Epoch: 67021555
"""
    )
    shell = current_time_shell.CurrentTimeCmd(rc)
    out = shell.current_time_set_now()
    parser = current_time_shell.CurrentTimeParser()
    current_time = parser.parse(out)
    assert current_time["epoch"] == FAKE_EPOCH
    assert current_time["date"] == FAKE_TIME
