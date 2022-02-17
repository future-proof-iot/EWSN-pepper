# Copyright (C) 2022 Inria
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

from dataclasses import asdict
from pepper_data.datum import NamedDebugDatums
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


def test_pepper_start_with_ble_uwb():
    rc = init_ctrl(
        output="""
[{"bn":"pepper:ble:2ede8fd6","bt":83782,"n":"rssi","v":-52,"u":"dBm"}]
[{"bn":"pepper:uwb:2ede8fd6","bt":84060,"n":"d_cm","v":218,"u":"cm"},{"n":"los","v":100,"u":"%"},{"n":"rssi","v":0,"u":"dBm"}]
[{"bn":"pepper:ble:2ede8fd6","bt":84783,"n":"rssi","v":-62,"u":"dBm"}]
[{"bn":"pepper:uwb:2ede8fd6","bt":85063,"n":"d_cm","v":221,"u":"cm"},{"n":"los","v":100,"u":"%"},{"n":"rssi","v":0,"u":"dBm"}]
[{"bn":"pepper:ble:2ede8fd6","bt":85783,"n":"rssi","v":-73,"u":"dBm"}]
[{"bn":"pepper:uwb:2ede8fd6","bt":86067,"n":"d_cm","v":215,"u":"cm"},{"n":"los","v":100,"u":"%"},{"n":"rssi","v":0,"u":"dBm"}]
{"tag":"glass-120","epoch":951,"pets":[{"pet":{"etl":"754DAC9DDD6C1C494A28094208C45D7C32572535BB56792E78B4C072A3A5AA92","rtl":"AE1930495EF9A0119387E879E00C3325D9AB5B92BCFC7FC94322549E4DCAF2A4","uwb":{"exposure":265,"req_count":51,"avg_d_cm":104},"ble":{"exposure":269,"scan_count":63,"avg_rssi":-63.1740112,"avg_d_cm":135}}}]}
[{"bn":"pepper:ble:2ede8fd6","bt":83782,"n":"rssi","v":-52,"u":"dBm"}]
[{"bn":"pepper:uwb:2ede8fd6","bt":84060,"n":"d_cm","v":218,"u":"cm"},{"n":"los","v":100,"u":"%"},{"n":"rssi","v":0,"u":"dBm"}]
[{"bn":"pepper:ble:2ede8fd6","bt":84783,"n":"rssi","v":-62,"u":"dBm"}]
[{"bn":"pepper:uwb:2ede8fd6","bt":85063,"n":"d_cm","v":221,"u":"cm"},{"n":"los","v":100,"u":"%"},{"n":"rssi","v":0,"u":"dBm"}]
[{"bn":"pepper:ble:2ede8fd6","bt":85783,"n":"rssi","v":-73,"u":"dBm"}]
[{"bn":"pepper:uwb:2ede8fd6","bt":86067,"n":"d_cm","v":215,"u":"cm"},{"n":"los","v":100,"u":"%"},{"n":"rssi","v":0,"u":"dBm"}]
{"tag":"los-120","epoch":61064179,"pets":[{"pet":{"etl":"94069309857C25F8922F764A65BC7E305EC08EF7A01860B4AFBF614E75423F57","rtl":"A6260674EC59943E52729D056EB37878E8E8E4B3FE1BCA8A6EE8599467D8CA54","uwb":{"exposure":268,"req_count":51,"avg_d_cm":90},"ble":{"exposure":276,"scan_count":67,"avg_rssi":-58.3306350,"avg_d_cm":63}}}]}
{"tag":"los-120","epoch":30000000,"pets":[{"pet":{"etl":"A64C359A30BE9C042EB6E455123B6A49621D50B4EDD7A8B7CCD6E0C1408B8472","rtl":"9EBD01E8CF8ED79772207942FE7EFDB675532158658EBC624B0C8B3223E1EF03","uwb":{"exposure":53,"lst_scheduled":45,"lst_aborted":0,"lst_timeout":3,"req_scheduled":55,"req_aborted":0,"req_timeout":11,"req_count":43,"avg_d_cm":218,"avg_los":100,"avg_rssi":-82.0859527},"ble":{"exposure":53,"scan_count":45,"avg_rssi":-56.8697281,"avg_d_cm":34}}}]}
"""
    )
    shell = pepper_shell.PepperCmd(rc)
    params = pepper_shell.PepperParams()
    out = shell.pepper_start(params)
    parser = pepper_shell.PepperStartParser()
    epoch_data, debug_data = parser.parse(out)
    assert len(epoch_data) == 3
    assert len(debug_data.ble) == 6
    assert len(debug_data.uwb) == 6
