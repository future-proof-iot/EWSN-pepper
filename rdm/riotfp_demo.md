# RIOT-fp DEMO

WP output inclusion in the demo is shown in the figure below.

| ![](https://notes.inria.fr/uploads/upload_573594f8a2945e3d1a39166ae492904f.png) |
|:-------------------------------------------------------------------------------:|
|                          *Demo RIOT Modules Overview*                           |

A global overview of the demo architecture is shown below.

| ![](https://notes.inria.fr/uploads/upload_bfefe396078a30b026f2ade1816ef694.png) |
|:-------------------------------------------------------------------------------:|
|                          *Demo Architecture Overview*                           |

An nrf52840 dongle serves as an IPV6 over BLE access point between the tokens and the internet.
Here devices require to be able to communicate with STOPCOVID server as well as fetch potential
firmware updates.

Communication between the STOPCOVID server and the tokens is protected via COSE encryption. The
keys for encryption decryption are derived via an EDHOC handshake at initialization.

Tokens discover each other over BLE and bases on sniffed messages will initiate TWR (two way ranging)
to esimate their distance. If the distance is under a threshold Private Encounter Token are derived
and all encounter information reported to the server.

Not shown here, but there is also a BLE dongle advertising current EPOCH so that all tokens
are syncrhonizing the EUID(ephermeral unique identifiers) changes.

## Use Case: BLE + UWB Contact Tracing

| ![DESIRE-phases](https://notes.inria.fr/uploads/upload_37b9a4e8a6a9d29b972952d1ac4aadfd.png) |
|:--------------------------------------------------------------------------------------------:|
|                                    *Desire/Pepper phases*                                    |

## Demo Flow

### Initialization

|  ![](https://notes.inria.fr/uploads/upload_09e272767070bcb9e283e83644d5ac54.png)  |
|:---------------------------------------------------------------------------------:|
| *Devices are initialized, EDHOC key exchange takes places, COSE keys are derived* |

When first booted devices start scanning over BLE. If an AP is found then EDHOC key exchange
is attempted against the STOPCOVID (PEPPER) server. If it suceeds then keys are devided
for securing future communication with the server.

In the initial deployment the distance threshold is <2m. Initially a subset of nodes (2)
are deployed at a distance <2m and the test of the nodes are at a distance <3m of a
node that during the demo will be declared COVID positive.

### Proximity Discovery

| ![](https://notes.inria.fr/uploads/upload_120aef3375bfc8f3077ea2a6a860317b.png) |
|:-------------------------------------------------------------------------------:|
|        *Discovered Nodes Initiate TWR Activity, a distance is estimated*        |

Devices are almost allways (passively) scanning over BLE for EUID (ephemeral unique
identifiers) and advertising there indentifiers.

When a neighbors EUID is reconstructed (needs 3 slices) then the token had enough
information to initiate TWR (two way ranging) acativity against the other node.
For the duration of the epoch and as long as the neighbor is still seen over BLE
TWR exchanges will periodically happen.

| ![](https://notes.inria.fr/uploads/upload_f07f1b74e6312ca870871750b006241e.png) |
|:-------------------------------------------------------------------------------:|
|          *COSE protect Encounter Tables are sent to STOP COVID server*          |

At the end of an EPOCH (15 minutes window where EUID are the same) for all encounters
if the encounter data (distance, exposure time, etc) meets some criteria then PETS
(Private Encounter Tokens) are derived. These with as well as the average distance
and exposure are sent out to the server. These exchanges are protected with the
preivously derived COSE keys.

Because of the initial setup only two nodes should be generating PETS since they will
be the only ones under 2m.

### Virality change

| ![](https://notes.inria.fr/uploads/upload_7d8bb66d859b276c4129c66e7d076f9e.png) |
|:-------------------------------------------------------------------------------:|
|                       *New Variant, contagious < 3meters*                       |

A change of scenario, the virus virality changes and new data suggests that contacts
can happen under 3m. The devices in the field need to be updated, luckily contact
filtering is containerized and can easily be updated.

A new container is deployed and devices update. Activity resumes seemlesly only now
many more devices are registering and reporting encounters.

### Infected User Declaration / Exposure Status Request

| ![](https://notes.inria.fr/uploads/upload_9c75e6bee40486e8770d2455d606598a.png) |
|:-------------------------------------------------------------------------------:|
|         *One Token is declared positive, those in contact are flagged*          |

Finnally one of the nodes is declared positive (button press), the reserver receives
this information and matches with previous received PETS. It the sets the exposure status
for all registered tokens. Those that are now exposed see their led switch on.

[RIOT-fp Demo Slides]: https://docs.google.com/presentation/d/1dcmDsu2J6ER6pFq6RJeBTn46sJ4kBbZ6kDpCVUgesqc/edit#slide=id.ge95e77b560_2_325
