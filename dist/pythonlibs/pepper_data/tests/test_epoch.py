# Copyright (C) 2022 Inria
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

import pytest
import os
import pathlib

from pepper_data.epoch import EpochData

TEST_EPOCH_DATA_1 = '{"tag":"DWAFDE","epoch":1427,"pets":[{"pet":{"etl":"7720B580293BA6B973DCD8BB07B6F5758F422593DB70F2B7B90BDC2FE082EE98","rtl":"66B30BC00CDB1A466C6FB9BBA0B5143D9F19F45F18440CA5A8A55532494BE1E7","uwb":{"exposure":24,"lst_scheduled":23,"lst_aborted":0,"lst_timeout":1,"req_scheduled":25,"req_aborted":0,"req_timeout":4,"req_count":21,"avg_d_cm":54,"avg_los":100,"avg_rssi":-79.6157379},"ble":{"exposure":24,"scan_count":23,"avg_rssi":-45.6186065,"avg_d_cm":5}}}]}'
TEST_EPOCH_DATA_2 = '{"tag":"DWAFDE","epoch":671,"pets":[{"pet":{"etl":"4EB7F3AB504D20680687A6D2BECF44214DB23E4FD4C2AC99BEE49765A6BF1F20","rtl":"FD6B4A529C39142D8BD6ED519FF5F68AF9FE935E54EEFA9DDFD255DD1FAD8C34","uwb":{"exposure":23,"req_count":20,"avg_d_cm":41},"ble":{"exposure":24,"scan_count":24,"avg_rssi":-47.7066650,"avg_d_cm":8}}}]}'

@pytest.mark.parametrize(
    "strings",
    [
        TEST_EPOCH_DATA_1,
        TEST_EPOCH_DATA_2,
    ],
)
def test_epoch_data_from_json_str(strings):
    data = EpochData.from_json_str(strings)
    assert data.node_id == "DWAFDE"

@pytest.mark.parametrize(
    "files",
    [
        os.path.join(pathlib.Path(__file__).parent.resolve(), "../static/epoch_data.txt"),
        os.path.join(pathlib.Path(__file__).parent.resolve(), "../static/epoch_data_stats.txt"),
    ],
)
def test_epoch_data_from_file(files):
    datas = EpochData.from_file(files)
    assert len(datas) > 1
