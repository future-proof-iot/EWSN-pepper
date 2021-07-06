#!/usr/bin/env python3
import logging
from tkinter.constants import NO
from typing import List


from desire_coap_resources import ErtlPayload
from desire_coap_resources import DesireCoapServer, RqHandlerBase


class DummyRqHandler(RqHandlerBase):

    def update_ertl(self, uid, ertl:ErtlPayload):
        print(f'[{self.__class__.__name__}] update_ertl: uid={uid}, ertl = {ertl}, json = \n{ertl.to_json_str()}')
    
    def get_ertl(self, uid) -> ErtlPayload:
        # load from statics
        ertl = None
        with open('static/ertl.json') as json_file:
            ertl = ErtlPayload.from_json_str(''.join(json_file.readlines()))

        print(f'[{self.__class__.__name__}] update_ertl: uid={uid}, ertl = {ertl}')
        return ertl

    def is_infected(self, uid) -> bool :
        print(f'[{self.__class__.__name__}] is_infected: uid={uid}')
        return False

    def is_exposed(self, uid) -> bool:
        print(f'[{self.__class__.__name__}] is_exposed: uid={uid}')
        return False
    
    def set_infected(self, uid) -> None :
        print(f'[{self.__class__.__name__}] set_infected: uid={uid}')
        return None

    def set_exposed(self, uid) -> None:
        print(f'[{self.__class__.__name__}] set_exposed: uid={uid}')
        return None


# logging setup

logging.basicConfig(level=logging.INFO)
logging.getLogger("coap-server").setLevel(logging.DEBUG)

def main(nodes:List[str],bind:bool):
    # Desire coap server instance , the rq_handler is the engine for handling post/get requests
    coap_server = DesireCoapServer(host="localhost" if bind else None, port=5683 if bind else None, rq_handler=DummyRqHandler(), nodes=nodes)
    # blocking run in this thread
    coap_server.run()


if __name__ == "__main__":
    import sys
    import platform

    assert len(sys.argv)>1, "Provide at least one node uid for enrollement"
    
    main(sys.argv[1:], bind=platform.system() != 'Linux)')

