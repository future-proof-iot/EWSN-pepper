import pytest

import iotlab


@pytest.mark.parametrize(
    "iotlab_node,expected",
    [
        ("arduino-zero-1.saclay.iot-lab.info", "arduino-zero"),
        ("st-lrwan1-2.saclay.iot-lab.info", "b-l072z-lrwan1"),
        ("st-iotnode-3.saclay.iot-lab.info", "b-l475e-iot01a"),
        ("dwm1001-1.saclay.iot-lab.info", "dwm1001"),
        ("firefly-10.lille.iot-lab.info", "firefly"),
        ("frdm-kw41z-4.saclay.iot-lab.info", "frdm-kw41z"),
        ("a8-125.grenoble.iot-lab.info", "iotlab-a8-m3"),
        ("m3-23.lyon.iot-lab.info", "iotlab-m3"),
        ("microbit-5.saclay.iot-lab.info", "microbit"),
        ("nrf51dk-2.saclay.iot-lab.info", "nrf51dk"),
        ("nrf52dk-6.saclay.iot-lab.info", "nrf52dk"),
        ("nrf52832mdk-1.saclay.iot-lab.info", "nrf52832-mdk"),
        ("nrf52840dk-7.saclay.iot-lab.info", "nrf52840dk"),
        ("nrf52840mdk-1.saclay.iot-lab.info", "nrf52840-mdk"),
        ("phynode-1.saclay.iot-lab.info", "pba-d-01-kw2x"),
        ("samr21-19.saclay.iot-lab.info", "samr21-xpro"),
        ("samr30-3.saclay.iot-lab.info", "samr30-xpro"),
    ],
)
def test_board_from_iotlab_node(iotlab_node, expected):
    assert iotlab.IoTLABExperiment.board_from_iotlab_node(iotlab_node) == expected


def test_board_from_iotlab_node_invalid():
    with pytest.raises(ValueError):
        iotlab.IoTLABExperiment.board_from_iotlab_node("foobar")


def test_valid_board():
    assert iotlab.IoTLABExperiment.valid_board(
        next(iter(iotlab.IoTLABExperiment.BOARD_ARCHI_MAP))
    )


def test_invalid_board():
    assert not iotlab.IoTLABExperiment.valid_board("ghgsoqwczoe")


@pytest.mark.parametrize(
    "iotlab_node,site,board",
    [
        ("m3-84.grenoble.iot-lab.info", "grenoble", None),
        ("a8-11.lyon.iot-lab.info", "lyon", None),
        ("m3-84.lille.iot-lab.info", "lille", "iotlab-m3"),
        ("a8-84.saclay.iot-lab.info", "saclay", "iotlab-a8-m3"),
    ],
)
def test_valid_iotlab_node(iotlab_node, site, board):
    iotlab.IoTLABExperiment.valid_iotlab_node(iotlab_node, site, board)


@pytest.mark.parametrize(
    "iotlab_node,site,board",
    [
        ("m3-84.grenoble.iot-lab.info", "lyon", None),
        ("wuadngum", "gvcedudng", None),
        ("m3-84.lille.iot-lab.info", "lille", "samr21-xpro"),
        ("a8-84.saclay.iot-lab.info", "saclay", "eauneguä"),
    ],
)
def test_invalid_iotlab_node(iotlab_node, site, board):
    with pytest.raises(ValueError):
        iotlab.IoTLABExperiment.valid_iotlab_node(iotlab_node, site, board)


def test_user_credentials(monkeypatch):
    creds = ("user", "password")
    monkeypatch.setattr(iotlab, "get_user_credentials", lambda: creds)
    assert iotlab.IoTLABExperiment.user_credentials() == creds


def test_check_user_credentials(monkeypatch):
    monkeypatch.setattr(iotlab, "get_user_credentials", lambda: ("user", "password"))
    assert iotlab.IoTLABExperiment.check_user_credentials()


def test_check_user_credentials_unset(monkeypatch):
    monkeypatch.setattr(iotlab, "get_user_credentials", lambda: (None, None))
    assert not iotlab.IoTLABExperiment.check_user_credentials()


@pytest.mark.parametrize(
    "envs,args,exp_boards",
    [
        (
            [{"IOTLAB_NODE": "m3-23.lille.iot-lab.info", "BOARD": "iotlab-m3"}],
            (),
            ["iotlab-m3"],
        ),
        ([{"IOTLAB_NODE": "m3-23.lille.iot-lab.info"}], (), ["iotlab-m3"]),
        ([{"BOARD": "iotlab-m3"}], (), ["iotlab-m3"]),
        (
            [{"IOTLAB_NODE": "m3-23.lille.iot-lab.info", "BOARD": "iotlab-m3"}],
            ("lille",),
            ["iotlab-m3"],
        ),
        (
            [{"IOTLAB_NODE": "m3-23.grenoble.iot-lab.info"}],
            ("grenoble",),
            ["iotlab-m3"],
        ),
        ([{"BOARD": "iotlab-m3"}], ("saclay",), ["iotlab-m3"]),
    ],
)
def test_init(envs, args, exp_boards):
    assert iotlab.DEFAULT_SITE == "lille"
    exp = iotlab.IoTLABExperiment("test", envs, *args)
    if args:
        assert exp.site == args[0]
    else:
        assert exp.site == "lille"
    assert exp.envs == envs
    for env, exp_board in zip(exp.envs, exp_boards):
        assert env["BOARD"] == exp_board
    assert exp.name == "test"
    assert exp.exp_id is None


@pytest.mark.parametrize(
    "envs,args",
    [
        (
            [{"IOTLAB_NODE": "m3-23.saclay.iot-lab.info", "BOARD": "iotlab-m3"}],
            ("khseaip",),
        ),
        ([{"BOARD": "uaek-eaqfgnic"}], ()),
        ([{"IOTLAB_NODE": "go5wxbp-124.saclay.iot-lab.info"}], ()),
        ([{}], ()),
    ],
)
def test_init_value_error(envs, args):
    with pytest.raises(ValueError):
        iotlab.IoTLABExperiment("test", envs, *args)


@pytest.mark.parametrize("exp_id,expected", [(None, None), (1234, "This is a test")])
def test_stop(monkeypatch, exp_id, expected):
    monkeypatch.setattr(
        iotlab.IoTLABExperiment,
        "user_credentials",
        lambda cls: ("user", "password"),
    )
    monkeypatch.setattr(iotlab, "Api", lambda user, password: None)
    monkeypatch.setattr(iotlab, "stop_experiment", lambda api, exp_id: expected)
    envs = [{"BOARD": "iotlab-m3"}]
    exp = iotlab.IoTLABExperiment("test", envs)
    exp.exp_id = exp_id
    assert exp.stop() == expected


@pytest.mark.parametrize(
    "envs, exp_nodes",
    [
        ([{"BOARD": "nrf52dk"}], ["nrf52dk-5.lille.iot-lab.info"]),
        (
            [{"IOTLAB_NODE": "samr21-21.lille.iot-lab.info"}],
            ["samr21-21.lille.iot-lab.info"],
        ),
    ],
)
def test_start(monkeypatch, envs, exp_nodes):
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
    exp = iotlab.IoTLABExperiment("test", envs)
    exp.start()
    assert exp.exp_id == 12345


def test_start_error(monkeypatch):
    monkeypatch.setattr(
        iotlab.IoTLABExperiment,
        "user_credentials",
        lambda cls: ("user", "password"),
    )
    monkeypatch.setattr(iotlab, "Api", lambda user, password: None)
    envs = [{"BOARD": "iotlab-m3"}]
    exp = iotlab.IoTLABExperiment("test", envs)
    envs[0].pop("BOARD")
    with pytest.raises(ValueError):
        exp.start()
