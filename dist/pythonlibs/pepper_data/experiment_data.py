#!/usr/bin/env python3

"""
Dataclass definitions matchings pepper JSON formatted end of epoch logs
"""

import json
from dataclasses import dataclass, asdict
from dacite import from_dict
from typing import List, Optional, Tuple, Union
from pepper_data.datum import DebugDatums
from pepper_data.epoch import EpochData


@dataclass
class ExperimentNode:
    uid: str                                          # DWCAFE
    network_address: str                              # iot-lab format, example: dwm1001-5.saclay.iot-lab.info
    position: Union[List[float], Tuple[float, ...]]   # (x, y, z)
    debug_data: Optional[DebugDatums]
    epoch_data: Optional[List[EpochData]]

    @property
    def node_id(self) -> str:
        # iot-lab node id, example: dwm1001-5
        return self.network_address.split('.')[0]

    @staticmethod
    def from_json_str(json_string: str):
        json_dict = json.loads(json_string)
        return from_dict(data_class=ExperimentNode, data=json_dict)

    def to_json_str(self, indent=None) -> str:
        json_dict = asdict(self)
        return json.dumps(json_dict, indent=indent)

@dataclass
class ExperimentData:
    nodes: List[ExperimentNode]

    def to_json_str(self, indent=None) -> str:
        json_dict = asdict(self)
        return json.dumps(json_dict, indent=indent)

    @staticmethod
    def from_json_str(json_string: str):
        json_dict = json.loads(json_string)
        return from_dict(data_class=ExperimentData, data=json_dict)
