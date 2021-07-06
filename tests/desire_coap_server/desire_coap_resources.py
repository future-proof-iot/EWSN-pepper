import logging

from abc import ABC, abstractmethod

import asyncio

import aiocoap.resource as resource
import aiocoap

from typing import List
from dataclasses import dataclass, field

from desire_coap_payloads import ErtlPayload, InfectedPayload, EsrPayload

# Coap Request handler to whom we formward the requests
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
        #print(f'uri = {request.get_request_uri()}, payload = {request.payload}')
        #print(f'\n\n location_path = {request.opt.location_path}, location_query = {request.opt.location_query}, content_format={request.opt.content_format}\n\n')
        #print(f'header = {request.opt}')

        rsp = aiocoap.Message(mtype=request.mtype)
        content_format = request.opt.content_format 
        
        try:
            if content_format == aiocoap.numbers.media_types_rev['application/json']:
                ertl = ErtlPayload.from_json_str(request.payload)
                self.handler.update_ertl(self.uid, ertl)
                # TODO handle eventual return code of ertl update (?) and report in the coap response (?)                
                rsp.opt.content_format = content_format
            elif content_format == aiocoap.numbers.media_types_rev['application/cbor']:
                ertl = ErtlPayload.from_cbor_bytes(request.payload)
                self.handler.update_ertl(self.uid, ertl)
                # TODO handle eventual return code of ertl update (?) and report in the coap response (?)                
                rsp.opt.content_format = content_format
            else:
                # unsupported payload format
                rsp = aiocoap.Message(mtype=request.mtype, code=aiocoap.numbers.codes.Code.UNSUPPORTED_CONTENT_FORMAT)
        except Exception as e:
                rsp = aiocoap.Message(mtype=request.mtype, code=aiocoap.numbers.codes.Code.INTERNAL_SERVER_ERROR)

        return rsp


    async def render_get(self, request:aiocoap.message.Message): 
        rsp = aiocoap.Message(mtype=request.mtype)
        content_format = request.opt.content_format 
        try:
            if content_format == aiocoap.numbers.media_types_rev['application/json']:
                ertl = self.handler.get_ertl(self.uid)
                print(f"{self.uid} -> ertl = {ertl}")               
                rsp = aiocoap.Message(mtype=request.mtype, payload=ertl.to_json_str().encode())
                rsp.opt.content_format = content_format
            elif content_format == aiocoap.numbers.media_types_rev['application/cbor']:
                ertl = self.handler.get_ertl(self.uid)
                print(f"{self.uid} -> ertl = {ertl}")               
                rsp = aiocoap.Message(mtype=request.mtype, payload=ertl.to_cbor_bytes())
                rsp.opt.content_format = content_format
            else:
                # unsupported payload format
                rsp = aiocoap.Message(mtype=request.mtype, code=aiocoap.numbers.codes.Code.UNSUPPORTED_CONTENT_FORMAT)
        except Exception as e:
                print(e)
                rsp = aiocoap.Message(mtype=request.mtype, code=aiocoap.numbers.codes.Code.INTERNAL_SERVER_ERROR)

        return rsp


class InfectedResource(NodeResource):

    async def render_get(self, request): 
        rsp = aiocoap.Message(mtype=request.mtype)
        content_format = request.opt.content_format 
        try:
            infected_payload =  InfectedPayload(self.handler.is_infected(self.uid))
            if content_format == aiocoap.numbers.media_types_rev['application/json']:                
                rsp = aiocoap.Message(payload=infected_payload.to_json_str().encode())
                rsp.opt.content_format = content_format
            elif content_format == aiocoap.numbers.media_types_rev['application/cbor']:
                rsp = aiocoap.Message(payload=infected_payload.to_cbor_bytes())
                rsp.opt.content_format = content_format
            elif content_format == aiocoap.numbers.media_types_rev['application/octet-stream']:
                rsp = aiocoap.Message(payload=infected_payload.infected.to_bytes(1,byteorder='little'))
                rsp.opt.content_format = content_format
            else:
                # unsupported payload format
                rsp = aiocoap.Message(mtype=request.mtype, code=aiocoap.numbers.codes.Code.UNSUPPORTED_CONTENT_FORMAT)
        except Exception as e:
                rsp = aiocoap.Message(mtype=request.mtype, code=aiocoap.numbers.codes.Code.INTERNAL_SERVER_ERROR)

        return rsp

class EsrResource(NodeResource):

    async def render_get(self, request): 
        exposed_payload = EsrPayload(self.handler.is_exposed(self.uid))
        rsp = aiocoap.Message(mtype=request.mtype)
        content_format = request.opt.content_format 
        try:
            exposed_payload =  EsrPayload(self.handler.is_exposed(self.uid))
            if content_format == aiocoap.numbers.media_types_rev['application/json']:                
                rsp = aiocoap.Message(payload=exposed_payload.to_json_str().encode())
                rsp.opt.content_format = content_format
            elif content_format == aiocoap.numbers.media_types_rev['application/cbor']:
                rsp = aiocoap.Message(payload=exposed_payload.to_cbor_bytes())
                rsp.opt.content_format = content_format
            elif content_format == aiocoap.numbers.media_types_rev['application/octet-stream']:
                rsp = aiocoap.Message(payload=exposed_payload.contact.to_bytes(1,byteorder='little'))
                rsp.opt.content_format = content_format
            else:
                # unsupported payload format
                rsp = aiocoap.Message(mtype=request.mtype, code=aiocoap.numbers.codes.Code.UNSUPPORTED_CONTENT_FORMAT)
        except Exception as e:
                rsp = aiocoap.Message(mtype=request.mtype, code=aiocoap.numbers.codes.Code.INTERNAL_SERVER_ERROR)

        return rsp


# Coap Server
@dataclass
class DesireCoapServer:
    host:str
    port:int
    rq_handler: RqHandlerBase
    nodes:List[str]

    __coap_root:resource.Site = field(init=False, repr=False)

    def __post_init__(self):
        self.__coap_root = resource.Site()
        self.__coap_root.add_resource(['.well-known', 'core'],
            resource.WKCResource( self.__coap_root .get_resources_as_linkheader))
        for node_id in self.nodes:
            self.__coap_root.add_resource([node_id, 'ertl'], ErtlResource(uid=node_id, handler=self.rq_handler))
            self.__coap_root.add_resource([node_id, 'infected'], InfectedResource(uid=node_id, handler=self.rq_handler))
            self.__coap_root.add_resource([node_id, 'esr'], EsrResource(uid=node_id, handler=self.rq_handler))


    def run(self):
        if not self.host or not self.port:
            asyncio.Task(aiocoap.Context.create_server_context(self.__coap_root))
        else:
            asyncio.Task(aiocoap.Context.create_server_context(self.__coap_root,bind=(self.host,self.port)))
        
        asyncio.get_event_loop().run_forever()

