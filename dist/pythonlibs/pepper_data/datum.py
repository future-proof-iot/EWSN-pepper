from dataclasses import dataclass, asdict
from typing import Optional
from dacite import from_dict
import json


@dataclass
class Datum:
    bn: Optional[str]  # data tag
    t: int  # timestamp in milliseconds
    n: str  # node id (desire CID)
    v: float  # value
    u: str  # unit

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
        return self.t

    def to_json_str(self, indent=None) -> str:
        json_dict = asdict(self)
        return json.dumps(json_dict, indent=indent)

    @classmethod
    def from_json_str(cls, json_string: str):
        json_dict = json.loads(json_string)
        return from_dict(data_class=cls, data=json_dict)
