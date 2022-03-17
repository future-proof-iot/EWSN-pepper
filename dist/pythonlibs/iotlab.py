import logging
import re

from iotlabcli.auth import get_user_credentials
from iotlabcli.rest import Api
from iotlabcli.experiment import (submit_experiment, wait_experiment,
                                  stop_experiment, get_experiment,
                                  exp_resources, AliasNodes)


DEFAULT_SITE = 'lille'
IOTLAB_DOMAIN = 'iot-lab.info'


class IoTLABExperiment:
    """Utility for running iotlab-experiments based on a list of RIOT
       environments expects BOARD or IOTLAB_NODE variable to be set for
       received nodes"""
    BOARD_ARCHI_MAP = {
        'arduino-zero': {'name': 'arduino-zero', 'radio': 'xbee'},
        'b-l072z-lrwan1': {'name': 'st-lrwan1', 'radio': 'sx1276'},
        'b-l475e-iot01a': {'name': 'st-iotnode', 'radio': 'multi'},
        'dwm1001': {'name': 'dwm1001', 'radio': 'dw1000'},
        'firefly': {'name': 'firefly', 'radio': 'multi'},
        'frdm-kw41z': {'name': 'frdm-kw41z', 'radio': 'multi'},
        'iotlab-a8-m3': {'name': 'a8', 'radio': 'at86rf231'},
        'iotlab-m3': {'name': 'm3', 'radio': 'at86rf231'},
        'microbit': {'name': 'microbit', 'radio': 'ble'},
        'nrf51dk': {'name': 'nrf51dk', 'radio': 'ble'},
        'nrf52dk': {'name': 'nrf52dk', 'radio': 'ble'},
        'nrf52832-mdk': {'name': 'nrf52832mdk', 'radio': 'ble'},
        'nrf52840dk': {'name': 'nrf52840dk', 'radio': 'multi'},
        'nrf52840-mdk': {'name': 'nrf52840mdk', 'radio': 'multi'},
        'pba-d-01-kw2x': {'name': 'phynode', 'radio': 'kw2xrf'},
        'samr21-xpro': {'name': 'samr21', 'radio': 'at86rf233'},
        'samr30-xpro': {'name': 'samr30', 'radio': 'at86rf212b'},
    }

    SITES = ['grenoble', 'lille', 'saclay']

    def __init__(self, name, envs, site=DEFAULT_SITE):
        IoTLABExperiment._check_site(site)
        self.site = site
        IoTLABExperiment._check_envs(site, envs)
        self.envs = envs
        self.name = name
        self.exp_id = None

    @staticmethod
    def board_from_iotlab_node(iotlab_node):
        """Return BOARD matching iotlab_node"""
        reg = r'([0-9a-zA-Z\-]+)-\d+\.[a-z]+\.iot-lab\.info'
        match = re.search(reg, iotlab_node)
        if match is None:
            raise ValueError("Unable to parse {} as IoT-LAB node name of "
                             "format <node-name>.<site-name>.iot-lab.info"
                             .format(iotlab_node))
        iotlab_node_name = match.group(1)
        dict_values = IoTLABExperiment.BOARD_ARCHI_MAP.values()
        dict_names = [value['name'] for value in dict_values]
        dict_keys = list(IoTLABExperiment.BOARD_ARCHI_MAP.keys())
        return dict_keys[dict_names.index(iotlab_node_name)]

    @staticmethod
    def valid_board(board):
        return board in IoTLABExperiment.BOARD_ARCHI_MAP

    @staticmethod
    def valid_iotlab_node(iotlab_node, site, board=None):
        if site not in iotlab_node:
            raise ValueError("All nodes must be on the same site")
        if board is not None:
            if IoTLABExperiment.board_from_iotlab_node(iotlab_node) != board:
                raise ValueError("IOTLAB_NODE doesn't match BOARD")

    @classmethod
    def check_user_credentials(cls):
        res = cls.user_credentials()
        return res != (None, None)

    @staticmethod
    def user_credentials():
        return get_user_credentials()

    @staticmethod
    def _archi_from_board(board):
        """Return iotlab 'archi' format for BOARD"""
        return '{}:{}'.format(IoTLABExperiment.BOARD_ARCHI_MAP[board]['name'],
                              IoTLABExperiment.BOARD_ARCHI_MAP[board]['radio'])

    @staticmethod
    def _check_site(site):
        if site not in IoTLABExperiment.SITES:
            raise ValueError("iotlab site must be one of {}"
                             .format(IoTLABExperiment.SITES))

    @staticmethod
    def _valid_addr(env, addr):
        """Check id addr matches a specific BOARD"""
        return addr.startswith(
            IoTLABExperiment.BOARD_ARCHI_MAP[env['BOARD']]['name'])

    @staticmethod
    def _check_envs(site, envs):
        """Takes a list of environments and validates BOARD or IOTLAB_NODE"""
        for env in envs:
            # If BOARD is set it must be supported in iotlab
            if 'BOARD' in env:
                if not IoTLABExperiment.valid_board(env['BOARD']):
                    raise ValueError("{} BOARD unsupported in iotlab"
                                     .format(env))
                if 'IOTLAB_NODE' in env:
                    IoTLABExperiment.valid_iotlab_node(env['IOTLAB_NODE'],
                                                       site,
                                                       env['BOARD'])
            elif 'IOTLAB_NODE' in env:
                IoTLABExperiment.valid_iotlab_node(env['IOTLAB_NODE'],
                                                   site)
                board = IoTLABExperiment.board_from_iotlab_node(
                    env["IOTLAB_NODE"]
                )
                env['BOARD'] = board
            else:
                raise ValueError("BOARD or IOTLAB_NODE must be set")

    def stop(self):
        """If running stop the experiment"""
        ret = None
        if self.exp_id is not None:
            ret = stop_experiment(Api(*self.user_credentials()), self.exp_id)
            self.exp_id = None
        return ret

    def start(self, duration=60):
        """Submit an experiment, wait for it to be ready and map assigned
           nodes"""
        logging.debug("submitting experiment")
        self.exp_id = self._submit(site=self.site, duration=duration)
        logging.debug("waiting for experiment {} to go to state \"Running\""
                     .format(self.exp_id))
        self._wait()
        self._map_iotlab_nodes_to_riot_env(self._get_nodes())

    def get_nodes_position(self):
        info = list()
        if self.exp_id is not None:
            ret = get_experiment(Api(*self.user_credentials()), self.exp_id, 'nodes')
            for item in ret['items']:
                info.append({
                    'network_address': item['network_address'],
                    'position': (float(item['x']), float(item['y']), float(item['z'])),
                })
        info = sorted(info, key=lambda d: d['network_address'])
        return info

    def _wait(self):
        """Wait for the experiment to finish launching"""
        ret = wait_experiment(Api(*self.user_credentials()), self.exp_id)
        return ret

    def _submit(self, site, duration):
        """Submit an experiment with required nodes"""
        api = Api(*self.user_credentials())
        resources = list()
        for env in self.envs:
            if 'IOTLAB_NODE' in env:
                resources.append(exp_resources([env['IOTLAB_NODE']]))
            elif env['BOARD'] is not None:
                board = IoTLABExperiment._archi_from_board(env['BOARD'])
                alias = AliasNodes(1, site, board)
                resources.append(exp_resources(alias))
            else:
                raise ValueError("neither BOARD or IOTLAB_NODE are set")
        return submit_experiment(api, self.name, duration, resources)['id']

    def _map_iotlab_nodes_to_riot_env(self, iotlab_nodes):
        """Fetch reserved nodes and map each one to a RIOT env"""
        for env in self.envs:
            if 'IOTLAB_NODE' in env:
                if env['IOTLAB_NODE'] in iotlab_nodes:
                    iotlab_nodes.remove(env['IOTLAB_NODE'])
            else:
                for iotlab_node in iotlab_nodes:
                    if IoTLABExperiment._valid_addr(env, iotlab_node):
                        iotlab_nodes.remove(iotlab_node)
                        env['IOTLAB_NODE'] = str(iotlab_node)
                        break
            env['IOTLAB_EXP_ID'] = str(self.exp_id)

    def _get_nodes(self):
        """Return all nodes reserved by the experiment"""
        ret = get_experiment(Api(*self.user_credentials()), self.exp_id)
        return ret['nodes']
