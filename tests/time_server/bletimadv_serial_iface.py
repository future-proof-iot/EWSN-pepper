# Copyright (C) 2021 Inria
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

from datetime import datetime
import cffi
from riotctrl.ctrl import RIOTCtrl
import os

def main(adv_period:int, reboot:bool) :
    """
    Configures a bletimeadv_server firmware to current time of day and to advetise every <adv_period> seconds
    Assumes RIOT firmwree exposes  the following shell commands :
        - reboot : reset the node
        - time <year> <month> <day> <hour> <min> <sec> : sets current time 
        - start <period> : start time advertisements every <period> seconds
    """
    ctrl = RIOTCtrl()

    def repl(cmd:str):
        ctrl.term.sendline(cmd)  # send the command
        ctrl.term.expect(">")    # wait for the command result to finnish
        print(ctrl.term.before)  # print the command result

    with ctrl.run_term():
        ctrl.term.expect(">", timeout=5)  # wait for shell to start
        if reboot:
            print("Rebooting board prior config")
            repl("reboot")
        repl(f"time {datetime.today().strftime('%Y %m %d %H %M %S')}")
        repl(f"start {adv_period}")


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--adv_period", help="Advertisement period in seconds", type=int, default=10)
    parser.add_argument("-r", "--reboot", help="Reboot node prior config", action="store_true", default=False)
    args = parser.parse_args()

    main(adv_period=args.adv_period, reboot=True)#args.reboot)

