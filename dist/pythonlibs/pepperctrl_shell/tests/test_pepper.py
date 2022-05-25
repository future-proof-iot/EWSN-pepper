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
{"tag":"DWAFDE","epoch":731,"pets":[{"pet":{"etl":"4FA21E5EDBAF4F52765DC267EBD38E8558F300DAA180DF24CD32378F2D004B8B","rtl":"15E32EE320E98F4AB3D8DB59381B9ADA67FFD266FBCA917E6F0B2B98DBDBCC3A","uwb":{"exposure":24,"req_count":20,"avg_d_cm":42},"ble":{"exposure":24,"scan_count":25,"avg_rssi":-47.9868011,"avg_d_cm":8}}}]}
{"tag":"DWAFDE:door","epoch":1367,"pets":[{"pet":{"etl":"E5392F096D847C457BF175EA048FA882E2BC91648CA33F55B706DBF58A2D9D7B","rtl":"4BC59A4BD82FD96FC68E61053090C87C9F586CF635B5B54037A19180F903C175","uwb":{"exposure":14,"lst_scheduled":22,"lst_aborted":0,"lst_timeout":5,"req_scheduled":25,"req_aborted":0,"req_timeout":17,"req_count":7,"avg_d_cm":51,"avg_los":100,"avg_rssi":-79.7874527},"ble":{"exposure":24,"scan_count":22,"avg_rssi":-44.8905716,"avg_d_cm":5}}}]}
    """
    )
    shell = pepper_shell.PepperCmd(rc)
    params = pepper_shell.PepperParams()
    out = shell.pepper_start(params)
    parser = pepper_shell.PepperStartParser()
    epoch_data, uwb_ble_data = parser.parse(out)
    assert len(epoch_data) == 2
    assert epoch_data[0].tag == "DWAFDE"
    assert epoch_data[1].tag == "DWAFDE:door"


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
