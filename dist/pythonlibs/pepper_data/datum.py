from dataclasses import dataclass, asdict
from typing import List, Optional, Union
from dacite import from_dict
import json


@dataclass
class Datum:
    bn: Optional[str]  # data tag
    n: str  # node id (desire CID)
    v: Union[float, int]  # value
    u: str  # unit
    bt: Optional[int] = 0 # data tag
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
    def neighbor_id(self) -> str:
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
    def neighbor_id(self) -> str:
        "'pepper:ble:2ede8fd6' => 2ede8fd6"
        return self.base.bn.split(":")[-1]

    @property
    def tag(self) -> str:
        "'pepper:ble:2ede8fd6' => pepper"
        return self.base.bn.split(":")[0] if len(self.base.bn.split(":")) == 3 else None

    @property
    def tag_type(self) -> str:
        "'pepper:ble:2ede8fd6' => ble"
        return (
            self.base.bn.split(":")[1]
            if len(self.base.bn.split(":")) == 3
            else self.base.bn.split(":")[0]
        )

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
    d_cm: Optional[Union[float, int]] = None
    tag: Optional[str] = ""
    rssi: Optional[Union[float, int]] = None
    los: Optional[Union[float, int]] = None


@dataclass
class BLEDatum:
    cid: str
    time: int
    rssi: Optional[Union[float, int]] = None


@dataclass
class DebugDatums:
    uwb: List[UWBDatum]
    ble: List[BLEDatum]

    @classmethod
    def from_datums(cls, datums: List[Datums]):
        uwb_list = list()
        ble_list = list()
        for datum in datums:
            if datum.tag_type == "uwb":
                uwb = UWBDatum(cid=datum.neighbor_id, time=datum.base.timestamp)
                for idx, _ in enumerate(datum.meas):
                    uwb.rssi = datum.value(idx) if datum.name(idx) == "rssi" else None
                    uwb.d_cm = datum.value(idx) if datum.name(idx) == "d_cm" else None
                    uwb.los = datum.value(idx) if datum.name(idx) == "los" else None
                uwb_list.append(uwb)
            if datum.tag_type == "ble":
                ble = BLEDatum(cid=datum.neighbor_id, time=datum.base.timestamp)
                for idx, _ in enumerate(datum.meas):
                    ble.rssi = datum.value(idx) if datum.name(idx) == "rssi" else None
                ble_list.append(ble)
        return DebugDatums(uwb=uwb_list, ble=ble_list)


@dataclass
class NamedDebugDatums:
    data: DebugDatums
    uid: str

    @staticmethod
    def from_json_str(json_string: str):
        json_dict = json.loads(json_string)
        return from_dict(data_class=NamedDebugDatums, data=json_dict)

    def clone(self):
        return from_dict(data_class=NamedDebugDatums, data=asdict(self))

    def to_json_str(self, indent=None) -> str:
        json_dict = asdict(self)
        return json.dumps(json_dict, indent=indent)
