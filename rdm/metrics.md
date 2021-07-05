# DESIRE & DESIRE Metrics

### Axis

#### Axis: Energy Efficiency
Quantitative Metrics:
* time spent TX
* time spent RX
* number of messages
* time spent processing data
* time spent reading and storing data
* RAM usage (chipsize)

#### Axis: Privacy Analysis
Qualitative Metrics:
* What data is stored on device?
* How is data stored on the device?
* What data is upstreamed?
* How is data upstreamed?
* What data is advertised?
* Possible Attacks

#### Axis: Data Storage and Transmission
Quantitative metrics:
* Data transmitted to server
* Filtered Encounters
* Data stored on device
* Operational data (RAM)


#### Axis: Contact Estimation
Quantitative metrics:
* Distance estimation precision
* False positives contact detection
* False negatives contact detection
    * operation (synchronization, etc.)
    * protocol
* Scalability: performance with high device density

#### Axis: Ease of Use
Qualitative metrics:
* What are the requirements from a user perspective?
* What are the requirements from a deployment perspective
Quantitative metrics:
* Cost per user

------------------------------------------

### Notes

#### DESIRE-Phone

Vanilla DESIRE compliant implementation on a Phone. To compare and justify designing a Token.

#### DESIRE-Token
Vanilla DESIRE compliant implementation on a Token. The base line on which to improve.

#### DESIRE2.0-Token
Extensions or modification of Vanilla DESIRE implementation on a Token. Explore improvements regarding the baseline on one or many Axis.


* Axis: Energy Efficiency
    * DESIRE-Token:
        * Time spent processing data: requires floating point arithmetics (log10f and exp) ~ 170us/message
        * Time spent RX/TX: independent of amount of contacts (Passive)
    * DESIRE2.0-Token:
        * Time spent processing data: requires floating point arithmetic (sub, add , div)
        * Time spent RX/TX: if using UWB will scale linearly with the amount of contacts (Active)

* Axis: Privacy Analysis
    * DESIRE2.0-Token:
        * Possible Atacks: if using UWB then DoS becomes a concern
        * How is data upstreamed: EDHOC could be considered

* Axis: Data Storage
    * DESIRE-Token:
        * Data transmitted to server: 96/156bytes per encounter (int16_t/float for RSSI)
        * Data stored on device:
            * Encounter data: 96/156bytes per encounter x encounters before upstream
            * ETL/RTL list: 72bytes per encounter x encounters every 15 days
        * Operation data (RAM): ~200bytes per seen device
    * DESIRE2.0-Token:
        * Data transmitted to server: if using UWB a risk score could be directly calculated (see [DESIRE-algorithm] section 5.2). If Heaviside then: risk-score(uint16_t) + timestamp (uint16_t) + contact-time(uint16_t) + EncounterToken(32Bytes) = ~38
        * Data stored on device:
            * Encounter data: 38bytes per encounter x encounters before upstream
            * ETL/RTL list: 72bytes per encounter x encounters every 15 days
        * Filtered encounters: low risk encounters can potentially be discarded, see [DESIRE-algorithm]
        * Operation data (RAM): if using UWB ~130bytes per seen device
```c
typedef struct uwb_ed {
    clist_node_t list_node;     /**< list head */
    uint32_t cummulative_d;     /**< commulative tof or distance */
    ebid_t ebid;                /**< the ebid structure 48 bytes + 1 status byte */
    uint32_t cid;               /**< the cid/short address */
    uint16_t start_s;           /**< time of first message, relative to start of epoch [s] */
    uint16_t end_s;             /**< time of last message, relative to start of epoch [s] */
    uint16_t samples;           /**< range requests count */
    event_timeout_t uwb;        /**< event timeout to schedule uwb events */
} uwb_ed_t;
```

* Axis: Contact Estimation
    * DESIRE-Phone:
        * False Negatives:
            * NTP roughly synchronized
    * DESIRE-Token:
        * False Negatives:
            * not synchronized
            * BLE risk-score dependent
    * DESIRE2.0-Token:
        * False Negatives:
            * not synchronized

* Axis: Ease of Use
    * DESIRE-Phone:
        * Requirements for user:
            * install an application
        * Deployment requirements
        * Cost per user: 0 if has phone with BLE, high if does not
    * DESIRE-Token:
        * Cost per user: ~20 Euro per user + infrastructure cost + environmental cost
        * Requirements for user:
            * registration?
            * re-charging?
            * pairing with phone?
        * Deployment requirements:
            * AP?
            * Token distribution?


[DESIRE]: https://hal.inria.fr/hal-02568730/document
[DESIRE-algorithm]: https://hal.inria.fr/hal-02641630/document
