from abc import ABC, abstractmethod
from contextlib import ContextDecorator
from turtle import position
from riotctrl.ctrl import RIOTCtrlBoardFactory
from iotlab import IoTLABExperiment

from typing import Tuple, List, Dict, Union

import yaml


Position = Union[List[float], Tuple[float, ...]]  # (x, y, z)


class RIOTCtrlAppFactory(RIOTCtrlBoardFactory, ContextDecorator):
    def __init__(self, board_cls=None):
        super().__init__(board_cls)
        self.ctrl_list = list()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        for ctrl in self.ctrl_list:
            ctrl.stop_term()
        return False

    def get_ctrl(
        self, app_dir=".", modules=None, cflags=None, env=None, flashfile=None
    ):
        """Returns a RIOTCtrl with its terminal started, if no envs are
        available ot if no env matches the specified 'BOARD' nothing is
        returned

        :param app_dir: the application directory
        :param modules: extra modules to add to 'USEMODULE'
        :param cflags: optional 'CFLAGS'
        :param flashfile: FLASHFILE to use, will use 'flash-only' to flash if set
        """
        the_env = dict()
        if env:
            the_env.update(env)
        # add extra MODULES and CFLAGS, extend as needed
        if cflags:
            the_env["CFLAGS"] = cflags
        if modules:
            the_env["USEMODULE"] = modules
        ctrl = super().get_ctrl(env=the_env, application_directory=app_dir)
        # if flashfile is set, flash without compiling
        if flashfile:
            ctrl.env["FLASHFILE"] = flashfile
            ctrl.FLASH_TARGETS = ["flash-only"]
        # append ctrl to list
        self.ctrl_list.append(ctrl)
        ctrl.flash()
        ctrl.reset()
        ctrl.start_term()
        # return ctrl with started terminal
        return ctrl


class RIOTCtrlEnvFactory(ABC, ContextDecorator):
    @abstractmethod
    def cleanup(self):
        pass

    @abstractmethod
    def get_envs(self, **kwargs) -> List[Dict]:
        pass


class FileCtrlEnvFactory(ContextDecorator):
    def get_envs(self, boards_file, **_ignored) -> List[Dict]:
        """
        :params boards_file: a yaml or json file
        """
        envs = []
        with open(boards_file, "r") as stream:
            envs = yaml.safe_load(stream)
        return envs

    def cleanup(self):
        pass


class IoTLABCtrlEnvFactory(ContextDecorator):
    def __init__(self) -> None:
        super().__init__()
        self.exps = list()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.cleanup()
        return False

    def cleanup(self):
        for exp in self.exps:
            exp.stop()

    def _map_envs_positions(self, envs, positions) -> List[Dict]:
        res = []
        for env in envs:
            for position in positions:
                if env["IOTLAB_NODE"] == position["network_address"]:
                    res.append({"env": env, "position": position["position"]})
        return res

    def get_envs(
        self,
        boards: List[str],
        name: str = "dwm1001-pepper",
        site: str = "lille",
        duration: int = 120,
        **_ignored,
    ) -> List[Dict]:
        """Start an experiment on IoT-LAB with boards

        :param boards: list of boards to book for experiment
            can be a list of RIOT 'BOARD', eg:
                ['samr21-xpro','samr21-xpro','iotlab-m3']
            or a list of IoT-LAB nodes 'network_address', eg:
                ['m3-1.grenoble.iot-lab.info', 'dwm1001-1.saclay.iot-lab.info']
        :param name: iotlab-experiment name
        :param site: iotlab-site, defaults 'lille'
        """
        # populate environments based on provided boards
        envs = list()
        for board in boards:
            if IoTLABExperiment.valid_board(board):
                env = {"BOARD": "{}".format(board)}
            else:
                env = {"IOTLAB_NODE": "{}".format(board)}
            envs.append(env)
        exp = IoTLABExperiment(name=name, envs=envs, site=site)
        exp.start(duration=duration)
        self.exps.append(exp)
        positions = exp.get_nodes_position()
        return self._map_envs_positions(envs, positions)
