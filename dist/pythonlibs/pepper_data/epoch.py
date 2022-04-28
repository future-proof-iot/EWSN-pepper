#!/usr/bin/env python3

"""
Dataclass definitions matchings pepper JSON formatted end of epoch logs
"""

from calendar import EPOCH
import json
from dataclasses import dataclass, asdict
from typing import List, Optional, Union
from dacite import from_dict


@dataclass
class Ble:
    exposure: int
    scan_count: int
    avg_rssi: float
    avg_d_cm: int


@dataclass
class Uwb:
    exposure: int
    lst_scheduled: Optional[int]
    lst_aborted: Optional[int]
    lst_timeout: Optional[int]
    req_scheduled: Optional[int]
    lst_scheduled: Optional[int]
    lst_aborted: Optional[int]
    lst_timeout: Optional[int]
    req_scheduled: Optional[int]
    req_aborted: Optional[int]
    req_timeout: Optional[int]
    req_count: int
    avg_d_cm: int
    avg_los: Optional[int]
    avg_rssi: Optional[Union[int, float]]


@dataclass
class PetPet:
    etl: str
    rtl: str
    uwb: Uwb
    ble: Ble


@dataclass
class PetElement:
    pet: PetPet


@dataclass
class EpochData:
    tag: str
    epoch: int
    pets: List[PetElement]

    @property
    def annotation_tag(self) -> str:
        "'DWABCD:pepper' => pepper"
        return self.tag.split(":")[-1]

    @property
    def node_id(self) -> str:
        "'DWABCD:pepper' => DWABCD"
        return self.tag.split(":")[0]

    @staticmethod
    def from_json_str(json_string: str):
        json_dict = json.loads(json_string)
        return from_dict(data_class=EpochData, data=json_dict)

    def clone(self):
        return from_dict(data_class=EpochData, data=asdict(self))

    def to_json_str(self, indent=None) -> str:
        json_dict = asdict(self)
        return json.dumps(json_dict, indent=indent)

    @staticmethod
    def from_file(filename: str):
        data = []
        with open(filename, "r") as f:
            for line in f:
                try:
                    data.append(EpochData.from_json_str(line))
                except ValueError:
                    continue
        return data
