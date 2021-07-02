#!/usr/bin/env python3
import logging

from abc import ABC, abstractmethod

import asyncio
from tkinter.constants import NO

import aiocoap.resource as resource
import aiocoap

from typing import List
from dataclasses import dataclass, asdict
from dacite import from_dict

from typing import List
import json

# Coap payloads

@dataclass
class EncounterData:
    etl: str
    rtl: str
    exposure: int
    req_count: int
    avg_d_cm: float

    def to_json_str(self):
        json_dict = asdict(self)
        return json.dumps(json_dict)

    @staticmethod
    def from_json_str(json_string: str):
        json_dict = json.loads(json_string)
        return from_dict(data_class=EncounterData, data=json_dict)

@dataclass
class PetElement:
    pet: EncounterData

    def to_json_str(self):
        json_dict = asdict(self)
        return json.dumps(json_dict)

    @staticmethod
    def from_json_str(json_string: str):
        json_dict = json.loads(json_string)
        return from_dict(data_class=PetElement, data=json_dict)

@dataclass
class ErtlPayload:
    epoch: int
    pets: List[PetElement]

    def to_json_str(self):
        json_dict = asdict(self)
        return json.dumps(json_dict)

    @staticmethod
    def from_json_str(json_string: str):
        json_dict = json.loads(json_string)  
        return from_dict(data_class=ErtlPayload, data=json_dict)

@dataclass
class EsrPayload:
    contact: bool

    def to_json_str(self):
        json_dict = asdict(self)
        return json.dumps(json_dict)

    @staticmethod
    def from_json_str(json_string: str):
        json_dict = json.loads(json_string)  
        return from_dict(data_class=EsrPayload, data=json_dict)

@dataclass
class InfectedPayload:
    infected: bool

    def to_json_str(self):
        json_dict = asdict(self)
        return json.dumps(json_dict)

    @staticmethod
    def from_json_str(json_string: str):
        json_dict = json.loads(json_string)  
        return from_dict(data_class=InfectedPayload, data=json_dict)

class RqHandlerBase(ABC):
    @abstractmethod
    def update_ertl(self, uid, ertl:ErtlPayload):
        pass

    @abstractmethod
    def get_ertl(self, uid) -> ErtlPayload:
        pass

    @abstractmethod
    def is_infected(self, uid) -> bool :
        pass

    @abstractmethod
    def is_exposed(self, uid) -> bool:
        pass
    
    @abstractmethod
    def set_infected(self, uid) -> None :
        pass

    @abstractmethod
    def set_exposed(self, uid) -> None:
        pass


### Coap resources

class NodeResource(resource.Resource):
    def __init__(self, uid:str, handler:RqHandlerBase):
        super().__init__()
        self.handler = handler
        self.uid = uid

class ErtlResource(NodeResource):
    async def render_post(self, request:aiocoap.message.Message):
        # get uid, get json payload,  
        print(f'uri = {request.get_request_uri()}, payload = {request.payload}')

        ertl = ErtlPayload.from_json_str(request.payload)
        self.handler.update_ertl(self.uid, ertl)

        return aiocoap.Message(payload=f"{ertl.to_json_str()}".encode())


    async def render_get(self, request:aiocoap.message.Message): 
        print(f'uid = {self.uid}')
        print(f"{self.uid} -> ertl = {self.handler.get_ertl(self.uid)}")
        return aiocoap.Message(payload=b"{}")

class InfectedResource(NodeResource):

    async def render_get(self, request): 
        infected_payload =  InfectedPayload(self.handler.is_infected(self.uid))
        return aiocoap.Message(payload=infected_payload.to_json_str().encode())

class EsrResource(NodeResource):

    async def render_get(self, request): 
        exposed_payload =  EsrPayload(self.handler.is_exposed(self.uid))
        return aiocoap.Message(payload=exposed_payload.to_json_str().encode())


if __name__ == "__main__":
    with open('static/ertl.json') as json_file:
        ertl = ErtlPayload.from_json_str(''.join(json_file.readlines()))
        print(ertl)