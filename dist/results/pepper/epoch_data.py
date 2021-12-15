#!/usr/bin/env python3

"""
Dataclass definitions matchings pepper JSON formatted end of epoch logs
"""

import json
from dataclasses import dataclass, asdict
from typing import List
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
    req_count: int
    avg_d_cm: int


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

    @staticmethod
    def from_json_str(json_string: str):
        json_dict = json.loads(json_string)
        return from_dict(data_class=EpochData, data=json_dict)

    def clone(self):
        return from_dict(data_class=EpochData, data=asdict(self))


@dataclass
class EpochData:
    tag: str
    epoch: int
    pets: List[PetElement]

    @staticmethod
    def from_json_str(json_string: str):
        json_dict = json.loads(json_string)
        return from_dict(data_class=EpochData, data=json_dict)

    def clone(self):
        return from_dict(data_class=EpochData, data=asdict(self))
