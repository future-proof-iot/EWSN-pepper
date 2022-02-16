#!/usr/bin/env python3

"""
Dataclass definitions matchings pepper JSON formatted end of epoch logs
"""

import json
from dataclasses import dataclass, asdict
from typing import List, Optional
from pepper_data.datum import Datum
from pepper_data.epoch import EpochData, NamedEpochData
from dacite import from_dict


@dataclass
class ExperimentData:
    epoch_data: List[NamedEpochData]
    uwb_ble_data: Optional[List[Datum]]

    def to_json_str(self, indent=None) -> str:
        json_dict = asdict(self)
        return json.dumps(json_dict, indent=indent)

    @staticmethod
    def from_json_str(json_string: str):
        json_dict = json.loads(json_string)
        return from_dict(data_class=ExperimentData, data=json_dict)
