# Copyright (C) 2022 Inria
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

import pytest
import pepperctrl_shell.pepper_shell as pepper_shell

from riotctrl_shell.tests.common import init_ctrl


def test_pepper_start():
    rc = init_ctrl(
        output="""
{"tag":"door-50","epoch":183,"pets":[{"pet":{"etl":"84EABD1FB544407B83B55B5D51387FBD3BAC5A0AA670328471C1792275DB2D5B","rtl":"220A556FEFDC4DA7781707487A5649D3DA55C1A0BFEF2E7263204BDF05D1C395","uwb":{"exposure":262,"req_count":48,"avg_d_cm":21},"ble":{"exposure":276,"scan_count":69,"avg_rssi":-56.1581611,"avg_d_cm":44}}}]}
{"epoch":1085,"pets":[{"pet":{"etl":"112223E0C6B719977D5B8CCD3180609D1C7A3AB5BC5BEDA5306252E95FD0C847","rtl":"6BE4C3FEE9FC15364165A330799A45DCAE2660C599945B9280A7B42EC87DBF2B","uwb":{"exposure":268,"req_count":50,"avg_d_cm":23},"ble":{"exposure":276,"scan_count":68,"avg_rssi":-55.4878273,"avg_d_cm":40}}}]}
    """
    )
    shell = pepper_shell.PepperCmd(rc)
    params = pepper_shell.PepperParams()
    out = shell.pepper_start(params)
    parser = pepper_shell.PepperStartParser()
    epoch_data, uwb_ble_data = parser.parse(out)
    assert len(epoch_data) == 2
    assert epoch_data[0].tag == "door-50"
    assert epoch_data[1].tag == None
    assert len(uwb_ble_data) == 0


def test_pepper_start_with_ble_uwb():
    rc = init_ctrl(
        output="""
{"bn":"pepper","t":34169,"n":"9a87236","v":-60,"u":"dBm"}
{"bn":"pepper","t":34176,"n":"9a87236","v":9,"u":"cm"}
{"bn":"pepper","t":34971,"n":"9a87236","v":-59,"u":"dBm"}
{"bn":"pepper","t":35170,"n":"9a87236","v":-59,"u":"dBm"}
{"tag":"glass-120","epoch":951,"pets":[{"pet":{"etl":"754DAC9DDD6C1C494A28094208C45D7C32572535BB56792E78B4C072A3A5AA92","rtl":"AE1930495EF9A0119387E879E00C3325D9AB5B92BCFC7FC94322549E4DCAF2A4","uwb":{"exposure":265,"req_count":51,"avg_d_cm":104},"ble":{"exposure":269,"scan_count":63,"avg_rssi":-63.1740112,"avg_d_cm":135}}}]}
{"bn":"pepper","t":32361,"n":"9a87236","v":-57,"u":"dBm"}
{"bn":"pepper","t":32376,"n":"9a87236","v":13,"u":"cm"}
{"bn":"pepper","t":32463,"n":"9a87236","v":-58,"u":"dBm"}
{"bn":"pepper","t":32668,"n":"9a87236","v":-54,"u":"dBm"}
{"tag":"los-120","epoch":61064179,"pets":[{"pet":{"etl":"94069309857C25F8922F764A65BC7E305EC08EF7A01860B4AFBF614E75423F57","rtl":"A6260674EC59943E52729D056EB37878E8E8E4B3FE1BCA8A6EE8599467D8CA54","uwb":{"exposure":268,"req_count":51,"avg_d_cm":90},"ble":{"exposure":276,"scan_count":67,"avg_rssi":-58.3306350,"avg_d_cm":63}}}]}
{"bn":"pepper","t":32069,"n":"9a87236","v":-58,"u":"dBm"}
{"bn":"pepper","t":32079,"n":"9a87236","v":10,"u":"cm"}
{"bn":"pepper","t":32165,"n":"9a87236","v":-59,"u":"dBm"}
{"bn":"pepper","t":32262,"n":"9a87236","v":-58,"u":"dBm"}
{"tag":"los-120","epoch":61064481,"pets":[{"pet":{"etl":"9DF3506F1F0EFD2AFDD78FA1D21A5ED52D9DB0A20D164FB2D2A7B889AB707307","rtl":"3560A58CE8DA807C2A148DE4A3C4CB9962E1FE76074E176249091E4979EFA9E2","uwb":{"exposure":269,"req_count":55,"avg_d_cm":91},"ble":{"exposure":275,"scan_count":66,"avg_rssi":-60.0559806,"avg_d_cm":82}}}]}
"""
    )
    shell = pepper_shell.PepperCmd(rc)
    params = pepper_shell.PepperParams()
    out = shell.pepper_start(params)
    parser = pepper_shell.PepperStartParser()
    epoch_data, uwb_ble_data = parser.parse(out)
    assert len(epoch_data) == 3
    assert len(uwb_ble_data) == 12


def test_pepper_start_with_debug():
    rc = init_ctrl(
        output="""
{"tag":"pepper","epoch":20,"pets":[{"pet":{"etl":"221FCDA1EDC3873DBA1CA68853B3D99BD128D8BFB58AFAE855E4EDAF3AD012A0","rtl":"DD7DD44FA1094731199AC17A02D3298BE63FC13F243BF7C75DB343DD5BAC23BE","uwb":{"exposure":55,"lst_scheduled":54,"lst_aborted":0,"lst_timeout":24,"req_scheduled":56,"req_aborted":0,"req_timeout":18,"req_count":37,"avg_d_cm":10,"avg_los":100},"ble":{"exposure":55,"scan_count":54,"avg_rssi":-58.9687424,"avg_d_cm":69}}}]}
"""
    )
    shell = pepper_shell.PepperCmd(rc)
    params = pepper_shell.PepperParams()
    out = shell.pepper_start(params)
    parser = pepper_shell.PepperStartParser()
    epoch_data, uwb_ble_data = parser.parse(out)
    assert len(epoch_data) == 1
    assert len(uwb_ble_data) == 0
    assert epoch_data[0].pets[0].pet.uwb.avg_los != 0
