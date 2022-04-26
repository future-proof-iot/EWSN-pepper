import pytest

import os
import pathlib
import iotlab
from factories import FileCtrlEnvFactory, IoTLABCtrlEnvFactory


@pytest.mark.parametrize(
    "boards, exp_nodes, exp_envs, exp_positions",
    [
        (
            ["dwm1001", "iotlab-m3"],
            ["dwm1001-1.lille.iot-lab.info", "m3-102.lille.iot-lab.info"],
            [
                {
                    "BOARD": "dwm1001",
                    "IOTLAB_NODE": "dwm1001-1.lille.iot-lab.info",
                    "IOTLAB_EXP_ID": "12345",
                },
                {
                    "BOARD": "iotlab-m3",
                    "IOTLAB_NODE": "m3-102.lille.iot-lab.info",
                    "IOTLAB_EXP_ID": "12345",
                },
            ],
            [
                {
                    "network_address": "dwm1001-1.lille.iot-lab.info",
                    "position": [0, 0, 0],
                },
                {"network_address": "m3-102.lille.iot-lab.info", "position": [0, 0, 0]},
            ],
        ),
        (
            ["dwm1001-2.lille.iot-lab.info"],
            ["dwm1001-2.lille.iot-lab.info"],
            [
                {
                    "BOARD": "dwm1001",
                    "IOTLAB_NODE": "dwm1001-2.lille.iot-lab.info",
                    "IOTLAB_EXP_ID": "12345",
                }
            ],
            [
                {
                    "network_address": "dwm1001-2.lille.iot-lab.info",
                    "position": [0, 0, 0],
                }
            ],
        ),
    ],
)
def test_iotlab_get_envs(monkeypatch, boards, exp_nodes, exp_envs, exp_positions):
    monkeypatch.setattr(
        iotlab.IoTLABExperiment,
        "user_credentials",
        lambda cls: ("user", "password"),
    )
    monkeypatch.setattr(iotlab, "Api", lambda user, password: None)
    monkeypatch.setattr(iotlab, "exp_resources", lambda arg: arg)
    monkeypatch.setattr(
        iotlab,
        "submit_experiment",
        lambda api, name, duration, resources: {"id": 12345},
    )
    monkeypatch.setattr(
        iotlab, "get_experiment", lambda api, exp_id: {"nodes": exp_nodes}
    )
    monkeypatch.setattr(iotlab, "wait_experiment", lambda api, exp_id: {})
    monkeypatch.setattr(
        iotlab.IoTLABExperiment, "get_nodes_position", lambda self: exp_positions
    )
    factory = IoTLABCtrlEnvFactory()
    assert len(factory.exps) == 0
    res = factory.get_envs(boards=boards)
    assert len(factory.exps) == 1
    for res, exp_env, exp_position in zip(res, exp_envs, exp_positions):
        assert res["env"] == exp_env
        assert res["position"] == exp_position["position"]


@pytest.mark.parametrize(
    "streams",
    [
        os.path.join(pathlib.Path(__file__).parent.resolve(), "boards.yaml"),
        os.path.join(pathlib.Path(__file__).parent.resolve(), "boards.json"),
    ],
)
def test_file_get_envs(streams):
    factory = FileCtrlEnvFactory()
    res = factory.get_envs(streams)
    assert res[0]["env"] == {"BOARD": "dwm1001"}
    assert res[0]["position"] == [0, 0, 0]
    assert res[1]["env"] == {"BOARD": "iotlab-m3", "DEBUG_ADAPTER_ID": 12345678}
    assert res[1]["position"] == [1, 2, 3]
