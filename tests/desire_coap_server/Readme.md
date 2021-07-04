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

| Resource URI                      | Semantic                                                                                                                                           |
|-----------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| `coap://localhost/<uid>/ertl`     | [POST, GET] a json/cbor(tag=51966) object representing the epoch's RTL and ETL data                                                                |
| `coap://localhost/<uid>/infected` | [POST] a json/cbor(tag=51962) object representing node's infection (a boolean)                                                                     |
| `coap://localhost/<uid>/esr`      | [GET] return an json/cbor(tag=51967) object integer equal to true if the node `<uid>`<br>was exposed to an infected user, and false otherwise      |

*Example of aiocoap-client session with the server:*
- POST ERTL data [static/ertl.json](static/ertl.json) for node `dw0456`
```shell
$ aiocoap-client -q coap://localhost/dw0456/ertl -m POST --content-format application/json --payload @static/ertl.json 
```
- POST ERTL data [static/ertl.cbor](static/ertl.cbor) for node `dw0456`
```shell
$ aiocoap-client -q coap://localhost/dw0456/ertl -m POST --content-format application/cbor --payload @static/ertl.cbor 
```
- GET ERTL data in `json` format for node `dw0456`
```shell
$ aiocoap-client -q coap://localhost/dw0456/ertl --content-format application/json
{
    "epoch": 332,
    "pets": [
        {
            "pet": {
                "etl": "56CE8ACF9E5D49D80DF41877983DB8752B82CB8152D79EF19C9B0D600C7EAE9E",
                "rtl": "0F070F9ACBB326E6A228CCA1D3FD393E6CA40901A8BD13E515BC9A00388C745E",
                "exposure": 22,
                "req_count": 21,
                "avg_d_cm": 51
            }
        },
        {
            "pet": {
                "etl": "56CE8ACF9E5D49D80DF41877983DB8752B82CB8152D79EF19C9B0D600C7EAE9E",
                "rtl": "0F070F9ACBB326E6A228CCA1D3FD393E6CA40901A8BD13E515BC9A00388C745E",
                "exposure": 22,
                "req_count": 21,
                "avg_d_cm": 51
            }
        }
    ]
}
```
- GET ERTL data in `cbor` format for node `dw0456`
```shell
$ aiocoap-client -q coap://localhost/dw0456/ertl --content-format application/cbor
CBORTag(51966, [332, [['56CE8ACF9E5D49D80DF41877983DB8752B82CB8152D79EF19C9B0D600C7EAE9E', '0F070F9ACBB326E6A228CCA1D3FD393E6CA40901A8BD13E515BC9A00388C745E', 22, 21, 51], ['56CE8ACF9E5D49D80DF41877983DB8752B82CB8152D79EF19C9B0D600C7EAE9E', '0F070F9ACBB326E6A228CCA1D3FD393E6CA40901A8BD13E515BC9A00388C745E', 22, 21, 51]]])
```

- GET infected data in `json` format for node `dw0456` - allows him to declare an infection
```shell
$ aiocoap-client -q coap://localhost/dw0456/infected --content-format application/json
{"infected": false}
```

- GET infected data in `cbor` format for node `dw0456` - allows him to declare an infection
```shell
$ aiocoap-client -q coap://localhost/dw0456/infected --content-format application/cbor
{"infected": false}
```

- GET exposure data in `json` for node `dw0456` - allows him to check if he was in contact with another infected user
```shell
$ aiocoap-client -q coap://localhost/dw0456/esr --content-format application/json
CBORTag(51962, [False]
```

- GET exposure data in `cbor` for node `dw0456` - allows him to check if he was in contact with another infected user
```shell
$ aiocoap-client -q coap://localhost/dw0456/esr --content-format application/cbor
CBORTag(51967, [False])
```

*Note on CBOR files in [./static](./static) folder*
The payloads have binary cbor files that can be dumped using this helper [tools/dump_cbor_file.py](tools/dump_cbor_file.py) as follows, where the hex bytes are printed then decoded using the system utility
```shell
$ python tools/dump_cbor_file.py static/ertl.cbor 
line [len = 282 bytes] = d9cafe8219014c8285784035364345384143463945354434394438304446343138373739383344423837353242383243423831353244373945463139433942304436303043374541453945784030463037304639414342423332364536413232384343413144334644333933453643413430393031413842443133453531354243394130303338384337343545161518338578403536434538414346394535443439443830444634313837373938334442383735324238324342383135324437394546313943394230443630304337454145394578403046303730463941434242333236453641323238434341314433464433393345364341343039303141384244313345353135424339413030333838433734354516151833
CBOR decoding:
{
    "CBORTag:51966": [
        332,
        [
            [
                "56CE8ACF9E5D49D80DF41877983DB8752B82CB8152D79EF19C9B0D600C7EAE9E",
                "0F070F9ACBB326E6A228CCA1D3FD393E6CA40901A8BD13E515BC9A00388C745E",
                22,
                21,
                51
            ],
            [
                "56CE8ACF9E5D49D80DF41877983DB8752B82CB8152D79EF19C9B0D600C7EAE9E",
                "0F070F9ACBB326E6A228CCA1D3FD393E6CA40901A8BD13E515BC9A00388C745E",
                22,
                21,
                51
            ]
        ]
    ]
}
```
Note that the hex string can also be deconded online on [http://cbor.me/](http://cbor.me/).

*Note on CBOR packet length*
In order to estimate the CBOR packet length for the ERTL payload (ErtlPayload class), one can generate a random object and serialize to cbor. Example running [tools/print_cbor_size.py](tools/print_cbor_size.py) with two random pets
```shell
$ PYTHONPATH=$PWD python tools/print_cbor_size.py 2
CBOR packet size for 2 pets
ErtlPayload(epoch=71, pets=[PetElement(pet=EncounterData(etl='B699803548DA82EA1D47E097A74CA08E4667595646CF427126686C420138D22B', rtl='4CD6710464607B50087A57AA082A8522A6D81886206354E0E6BA7A4A8980BA35', exposure=23, req_count=99, avg_d_cm=62.047)), PetElement(pet=EncounterData(etl='753E911B07E0AE17E0D17A5C4A9C7F3E8472AD4432D911065D67FFDED15C6D58', rtl='140F3F884C6FE25DAE07FC4A68A7A9135684D394BF57631809F5066A63CCD5C9', exposure=98, req_count=98, avg_d_cm=73.099))])
cbor packet length = 298
D9CAFE8218478285784042363939383033353438444138324541314434374530393741373443413038453436363735393536343643463432373132363638364334323031333844323242784034434436373130343634363037423530303837413537414130383241383532324136443831383836323036333534453045364241374134413839383042413335171863FB404F0604189374BC8578403735334539313142303745304145313745304431374135433441394337463345383437324144343433324439313130363544363746464445443135433644353878403134304633463838344336464532354441453037464334413638413741393133353638344433393442463537363331383039463530363641363343434435433918621862FB4052465604189375
```
