# Copyright (C) 2022 Inria
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

import pytest
import os
import pathlib

from pepper_data.datum import Datums, UWBDatum, BLEDatum, DebugDatums

TEST_DATUM_1 = '[{"bn":"DWABCD:ble:2ede8fd6","bt":83782,"n":"rssi","v":-52,"u":"dBm"}]'
TEST_DATUM_2 = '[{"bn":"DWABCD:pepper:uwb:8fd6","bt":84060,"n":"d_cm","v":218,"u":"cm"},{"n":"los","v":100,"u":"%"},{"n":"rssi","v":0,"u":"dBm"}]'


def test_datum_from_json_string_1():
    datums = Datums.from_json_str(TEST_DATUM_1)
    assert datums.node_id == "DWABCD"
    assert datums.tag == None
    assert datums.cid == "2ede8fd6"
    assert datums.neigh_id == None
    assert datums.tag_type == "ble"
    assert len(datums.meas) == 1


def test_datum_from_json_string_2():
    datums = Datums.from_json_str(TEST_DATUM_2)
    assert datums.node_id == "DWABCD"
    assert datums.tag == "pepper"
    assert datums.cid == "8fd6"
    assert datums.neigh_id == "DW8FD6"
    assert datums.tag_type == "uwb"
    assert len(datums.meas) == 3


def test_uwb_datum_from_json_str():
    with pytest.raises(ValueError) as e_info:
        UWBDatum.from_json_str(TEST_DATUM_1)
    uwb = UWBDatum.from_json_str(TEST_DATUM_2)
    assert uwb.node_id == "DWABCD"
    assert uwb.neigh_id == "DW8FD6"


def test_ble_datum_from_json_str():
    with pytest.raises(ValueError) as e_info:
        BLEDatum.from_json_str(TEST_DATUM_2)
    ble = BLEDatum.from_json_str(TEST_DATUM_1)
    assert ble.node_id == "DWABCD"
    assert ble.neigh_id == None


@pytest.mark.parametrize(
    "files",
    [
        os.path.join(pathlib.Path(__file__).parent.resolve(), "../static/ble.txt"),
    ],
)
def test_ble_datum_from_file(files):
    datums = BLEDatum.from_file(files)
    assert len(datums) > 0


@pytest.mark.parametrize(
    "files",
    [
        os.path.join(pathlib.Path(__file__).parent.resolve(), "../static/uwb.txt"),
        os.path.join(pathlib.Path(__file__).parent.resolve(), "../static/uwb_los_rssi.txt"),
    ],
)
def test_uwb_datum_from_file(files):
    datums = UWBDatum.from_file(files)
    assert len(datums) > 0

@pytest.mark.parametrize(
    "files",
    [
        os.path.join(pathlib.Path(__file__).parent.resolve(), "../static/ble_uwb.txt"),
    ],
)
def test_debug_datum_from_file(files):
    datum = DebugDatums.from_file(files)
    assert len(datum.uwb) > 0
    assert len(datum.ble) > 0
