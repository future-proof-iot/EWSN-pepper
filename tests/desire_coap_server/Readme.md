desire_coap_server
==
A python coap server for offloading RTL, ETL (for debug) and Exposure Status Request (ESR).

Enrollement is done by declaring the list of nodes uuids on sever start as args. 

Example of a server with nodes DW01E2 and DW0AB34 enrolled.
```shell
$ python desire_coap_srv.py DW01E2 DW0AB34
```
The node ids are built from their IEEE long mac address by using the last two bytes.

For each node, identified by a 16-bit uid in hex format, the following resources are exposes:

| Resource URI                      | Semantic                                                                                                                      |
|-----------------------------------|-------------------------------------------------------------------------------------------------------------------------------|
| `coap://localhost/<uid>/ertl`     | [POST] a json object representing the epoch's RTL and ETL data                                                                |
| `coap://localhost/<uid>/infected` | [POST] a json object representing node's infection (a boolean)                                                                |
| `coap://localhost/<uid>/esr`      | [GET] return an json object integer equal to true if the node `<uid>`<br>was exposed to an infected user, and false otherwise |

*Example of aiocoap-client session with the server:*
- POST ERTL data [static/ertl.json](static/ertl.json) for node `dw0456`
```shell
$ aiocoap-client -q coap://localhost/dw0456/ertl -m POST --payload @static/ertl.json 
```
- GET infected data for node `dw0456` - allows him to declare an infection
```shell
$ aiocoap-client -q coap://localhost/dw0456/infected
{"infected": false}
```

- GET exposure data for node `dw0456` - allows him to check if he was in contact with another infected user
```shell
$ aiocoap-client -q coap://localhost/dw0456/esr
{"contact": false}
```

