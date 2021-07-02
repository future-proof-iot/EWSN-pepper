#!/usr/bin/env python3
import logging

from abc import ABC, abstractmethod

import asyncio

import aiocoap.resource as resource
import aiocoap

from typing import List


from desire_coap_resources import ErtlPayload, ErtlResource, EsrResource, InfectedResource
from desire_coap_resources import RqHandlerBase

class DummyRqHandler(RqHandlerBase):

    def update_ertl(self, uid, ertl:ErtlPayload):
        print(f'[{self.__class__}] update_ertl: uid={uid}, ertl = {ertl}')
    
    def get_ertl(self, uid) -> ErtlPayload:
        print(f'[{self.__class__}] update_ertl: uid={uid}, ertl = {None}')
        return None

    def is_infected(self, uid) -> bool :
        print(f'[{self.__class__}] is_infected: uid={uid}')
        return False

    def is_exposed(self, uid) -> bool:
        print(f'[{self.__class__}] is_exposed: uid={uid}')
        return False
    
    def set_infected(self, uid) -> None :
        print(f'[{self.__class__}] set_infected: uid={uid}')
        return None

    def set_exposed(self, uid) -> None:
        print(f'[{self.__class__}] set_exposed: uid={uid}')
        return None


# logging setup

logging.basicConfig(level=logging.INFO)
logging.getLogger("coap-server").setLevel(logging.DEBUG)

def main(nodes:List[str]):
    # Resource tree creation
    root = resource.Site()
    rq_handler = DummyRqHandler()

    root.add_resource(['.well-known', 'core'],
            resource.WKCResource(root.get_resources_as_linkheader))
    
    for node_id in nodes:
        root.add_resource([node_id, 'ertl'], ErtlResource(uid=node_id, handler=rq_handler))
        root.add_resource([node_id, 'infected'], InfectedResource(uid=node_id, handler=rq_handler))
        root.add_resource([node_id, 'esr'], EsrResource(uid=node_id, handler=rq_handler))

    asyncio.Task(aiocoap.Context.create_server_context(root,bind=("localhost",5683))) # bind arg required on Mac and windows

    asyncio.get_event_loop().run_forever()

if __name__ == "__main__":
    import sys
    assert len(sys.argv)>1, "Provide at least one node uid for enrollement"
    main(sys.argv[1:])