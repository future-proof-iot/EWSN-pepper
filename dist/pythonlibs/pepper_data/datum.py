from dataclasses import dataclass, asdict
from distutils.dir_util import copy_tree
from multiprocessing.sharedctypes import Value
from typing import List, Optional, Union
from dacite import from_dict
import json


@dataclass
class Datum:
    bn: Optional[str]  # data tag
    n: str  # node id (desire CID)
    v: Union[float, int]  # value
    u: str  # unit
    bt: Optional[int] = 0  # data tag
    t: Optional[int] = 0  # timestamp in milliseconds

    @property
    def annotation_tag(self) -> str:
        return self.bn

    @property
    def value(self) -> int:
        return self.v

    @property
    def unit(self) -> str:
        return self.u

    @property
    def cid(self) -> str:
        return self.n

    @property
    def timestamp(self) -> str:
        return self.t + self.bt

    def to_json_str(self, indent=None) -> str:
        json_dict = asdict(self)
        return json.dumps(json_dict, indent=indent)

    @classmethod
    def from_json_str(cls, json_string: str):
        json_dict = json.loads(json_string)
        return from_dict(data_class=cls, data=json_dict)


@dataclass
class Datums:
    base: Datum
    meas: List[Datum]

    @property
    def node_id(self) -> str:
        "'DWABCD:pepper:ble:2ede8fd6' => DWABCD"
        return self.base.bn.split(":")[0]

    @property
    def tag(self) -> str:
        "'DWABCD:pepper:ble:2ede8fd6' => pepper"
        return self.base.bn.split(":")[1] if len(self.base.bn.split(":")) == 4 else None

    @property
    def tag_type(self) -> str:
        "'DWABCD:pepper:ble:2ede8fd6' => ble"
        return self.base.bn.split(":")[-2]

    @property
    def cid(self) -> str:
        "'DWABCD:pepper:ble:2ede8fd6' => 2ede8fd6"
        return self.base.bn.split(":")[-1]

    @property
    def neigh_id(self) -> str:
        "'DWABCD:pepper:ble:8fd6' => DW8fd6"
        return f"DW{self.cid.upper()}" if len(self.cid) == 4 else None

    def name(self, idx) -> str:
        if idx > len(self.meas):
            raise IndexError
        else:
            return self.meas[idx].n

    def timestamp(self, idx) -> int:
        if idx > len(self.meas):
            raise IndexError
        else:
            return self.meas[idx].t + self.base.bt

    def value(self, idx) -> str:
        if idx > len(self.meas):
            raise IndexError
        else:
            return self.meas[idx].v

    @classmethod
    def from_json_str(cls, json_string: str):
        json_data = json.loads(json_string)
        base = from_dict(data_class=Datum, data=json_data[0])
        meas = [from_dict(data_class=Datum, data=json_data) for json_data in json_data]

        return cls(base=base, meas=meas)

    def to_json_str(self, indent=None) -> str:
        json_dict = asdict(self.base)
        if self.meas:
            json_dict.append(asdict(datum) for datum in self.meas)
        return json.dumps(json_dict, indent=indent)


@dataclass
class UWBDatum:
    cid: str
    time: int
    node_id: str
    d_cm: Optional[Union[float, int]] = None
    tag: Optional[str] = ""
    rssi: Optional[Union[float, int]] = None
    los: Optional[Union[float, int]] = None

    @property
    def neigh_id(self) -> str:
        "'8fd6' => DW8fd6"
        return f"DW{self.cid.upper()}" if len(self.cid) == 4 else None

    @classmethod
    def from_datum(cls, datum: Datums):
        if datum.tag_type == "uwb":
            uwb = UWBDatum(
                cid=datum.cid, time=datum.base.timestamp, node_id=datum.node_id
            )
            for idx, _ in enumerate(datum.meas):
                if datum.name(idx) == "rssi":
                    uwb.rssi = datum.value(idx)
                if datum.name(idx) == "d_cm":
                    uwb.d_cm = datum.value(idx)
                if datum.name(idx) == "los":
                    uwb.los = datum.value(idx)
            return uwb
        else:
            raise ValueError("not UWBDatum")

    @classmethod
    def from_json_str(cls, json_string: str):
        datum = Datums.from_json_str(json_string)
        return UWBDatum.from_datum(datum)

    @classmethod
    def from_file(cls, filename: str):
        datums = []
        with open(filename, "r") as f:
            for line in f:
                try:
                    datums.append(UWBDatum.from_json_str(line))
                except ValueError:
                    continue
        return datums


@dataclass
class BLEDatum:
    cid: str
    time: int
    node_id: str
    rssi: Optional[Union[float, int]] = None

    @property
    def neigh_id(self) -> str:
        "'8fd6' => DW8fd6"
        return f"DW{self.cid.upper()}" if len(self.cid) == 4 else None

    @classmethod
    def from_datum(cls, datum: Datums):
        if datum.tag_type == "ble":
            ble = BLEDatum(
                cid=datum.cid, time=datum.base.timestamp, node_id=datum.node_id
            )
            for idx, _ in enumerate(datum.meas):
                ble.rssi = datum.value(idx) if datum.name(idx) == "rssi" else None
            return ble
        else:
            raise ValueError("not BLEDatum")

    @classmethod
    def from_json_str(cls, json_string: str):
        datum = Datums.from_json_str(json_string)
        return BLEDatum.from_datum(datum)

    @classmethod
    def from_file(cls, filename: str):
        datums = []
        with open(filename, "r") as f:
            for line in f:
                try:
                    datums.append(BLEDatum.from_json_str(line))
                except ValueError:
                    continue
        return datums


@dataclass
class DebugDatums:
    uwb: List[UWBDatum]
    ble: List[BLEDatum]

    @classmethod
    def from_datums(cls, datums: List[Datums]):
        uwb_list = list()
        ble_list = list()
        for datum in datums:
            try:
                uwb_list.append(UWBDatum.from_datum(datum))
                continue
            except ValueError:
                pass
            try:
                ble_list.append(BLEDatum.from_datum(datum))
                continue
            except ValueError:
                pass
        return DebugDatums(uwb=uwb_list, ble=ble_list)

    @staticmethod
    def from_file(filename: str):
        datums = []
        with open(filename, "r") as f:
            for line in f:
                try:
                    datums.append(Datums.from_json_str(line))
                except ValueError:
                    continue
        return DebugDatums.from_datums(datums)
