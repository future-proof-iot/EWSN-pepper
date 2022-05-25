# Copyright (C) 2021 Inria
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

"""
- author:      Roudy DAGHER <roudy.dagher@inria.fr>
"""

from dataclasses import dataclass, field
from typing import List, Dict, Callable, Any, Optional
from enum import IntFlag, unique

import re
import struct
import traceback

import asyncio
from bleak import BleakScanner, BleakClient
from bleak.backends.device import BLEDevice

import logging

logger = logging.getLogger(__name__)

# Define BLE objects: Service, characteristics and data structs
PEPPER_SERVICE = "5ea4f59a-78da-499d-931d-cfbd25ed365b"


class PEPPERCharacteristics:
    PEPPER_CONFIG_CHARACTERISTIC = "ce2ae1d5-b067-c348-985a-ba2f2b0ac8d0"
    PEPPER_START_CHARACTERISTIC = "ce2ae1d5-b067-c348-985a-ba2f2b0ac8d1"
    PEPPER_STOP_CHARACTERISTIC = "ce2ae1d5-b067-c348-985a-ba2f2b0ac8d2"


def fmt_addr(addr: int, width=2):
    return ":".join(re.findall("..", f"%0{width}X" % addr))


@dataclass(repr=False)
class PEPPERConfig:
    base_name: str

    def to_bytes(self) -> bytearray:
        return self.base_name.encode()

    @staticmethod
    def from_bytes(tlv_bytes: bytearray):
        return PEPPERConfig(base_name=tlv_bytes.decode())


@dataclass(repr=False)
class PEPPERStart:
    SIZE = 17
    BINARY_REPR = "<" + "I" + "I" + "I" + "I" + "I" + "I" + "?"

    epoch_duration_s: int
    epoch_iterations: int
    advs_per_slice: int
    adv_itvl_ms: int
    scan_itvl_ms: int
    scan_win_ms: int
    align: bool

    def to_bytes(self) -> bytearray:
        return bytearray(
            struct.pack(
                PEPPERStart.BINARY_REPR,
                self.epoch_duration_s,
                self.epoch_iterations,
                self.advs_per_slice,
                self.adv_itvl_ms,
                self.scan_itvl_ms,
                self.scan_win_ms,
                self.align,
            )
        )

    @staticmethod
    def from_bytes(tlv_bytes: bytearray):
        assert len(tlv_bytes) == PEPPERStart.SIZE
        (
            epoch_duration_s,
            epoch_iterations,
            advs_per_slice,
            adv_itvl_ms,
            scan_itvl_ms,
            scan_win_ms,
            align,
        ) = struct.unpack(PEPPERStart.BINARY_REPR, tlv_bytes)

        return PEPPERStart(
            epoch_duration_s,
            epoch_iterations,
            advs_per_slice,
            adv_itvl_ms,
            scan_itvl_ms,
            scan_win_ms,
            align,
        )


@dataclass
class PEPPERNode:
    device: BLEDevice
    connection_timeout: Optional[int] = 10
    client: BleakClient = field(init=False)

    def __post_init__(self):
        def disc_callback(client: BleakClient):
            print(f"Disconnected from device {self.device}")

        self.client = BleakClient(
            self.device,
            timeout=self.connection_timeout,
            disconnected_callback=disc_callback,
        )

    async def connect(self):
        await self.client.connect(timeout=self.connection_timeout)

    async def disconnect(self):
        await self.client.disconnect()

    @property
    def name(self) -> str:
        return self.device.name

    @property
    def is_connected(self) -> bool:
        return self.client.is_connected

    async def read_pepper_config(self) -> PEPPERConfig:
        value = await self.__read_ble_char(
            PEPPERCharacteristics.PEPPER_CONFIG_CHARACTERISTIC
        )
        return PEPPERConfig.from_bytes(value)

    async def write_pepper_config(self, config: PEPPERConfig):
        value = config.to_bytes()
        await self.__write_ble_char(
            PEPPERCharacteristics.PEPPER_CONFIG_CHARACTERISTIC, value
        )

    async def write_pepper_start(self, start: PEPPERStart):
        value = start.to_bytes()
        await self.__write_ble_char(
            PEPPERCharacteristics.PEPPER_START_CHARACTERISTIC, value
        )

    async def write_pepper_stop(self):
        await self.__write_ble_char(
            PEPPERCharacteristics.PEPPER_STOP_CHARACTERISTIC, b"0"
        )

    async def __read_ble_char(self, ble_char: str):
        if not self.is_connected:
            raise ConnectionError("Device not connected")
        value = await self.client.read_gatt_char(ble_char)
        return value

    async def __write_ble_char(self, ble_char: str, value: bytearray):
        if not self.is_connected:
            raise ConnectionError("Device not connected")
        await self.client.write_gatt_char(ble_char, value)


@dataclass
class BLEScanner:
    def __init__(self):
        self.scanner = BleakScanner()

    @staticmethod
    def detection_callback(device, advertisement_data):
        print(
            "detected:",
            device,
            " – RSSI:",
            device.rssi,
            " – adv data: ",
            advertisement_data,
        )

    async def scan(
        self,
        scan_duration: int,
        filter_name="^DW[A-Fa-f0-9]",
        filter_service=PEPPER_SERVICE,
        log_detections=True,
    ) -> List[BLEDevice]:
        # TODO: handle service uuid filter once bleak supports it properly cross-platform, meanwhile filter on manufcaturer id
        if log_detections:
            self.scanner.register_detection_callback(self.detection_callback)
        else:
            self.scanner.register_detection_callback(None)

        # do scan
        await self.scanner.start()
        await asyncio.sleep(scan_duration)
        await self.scanner.stop()
        scanned_devices = self.scanner.discovered_devices

        # filter on device name
        pattern = re.compile(filter_name)
        devices = [*filter(lambda dev: pattern.match(dev.name), scanned_devices)]

        logger.debug(
            "No devices found"
            if len(devices) == 0
            else f"found {len(devices)} devices: {devices}"
        )

        return devices

    async def discover(self, device: BLEDevice, connection_timeout=5):
        # dumps services and characteristics including values
        print(f"Discovering services on device {device}")
        try:
            async with BleakClient(device) as client:
                # x = await client.is_connected()
                x = await asyncio.wait_for(
                    client.is_connected(), timeout=connection_timeout
                )
                print("Connected: {0}".format(x))
                for service in client.services:
                    print(
                        "[Service] {0}: {1}".format(service.uuid, service.description)
                    )
                    await asyncio.wait_for(
                        client.is_connected(), timeout=connection_timeout
                    )
                    for char in service.characteristics:
                        if "read" in char.properties:
                            try:
                                await asyncio.wait_for(
                                    client.is_connected(), timeout=connection_timeout
                                )
                                value = bytes(await client.read_gatt_char(char.uuid))
                            except Exception as e:
                                print("Connection lost\n", e)
                                print(traceback.print_exc())
                                value = None
                        else:
                            value = None
                        print(
                            "\t[Characteristic] {0}: (Handle: {1}) ({2}) | Name: {3}, Value: {4} ".format(
                                char.uuid,
                                char.handle,
                                ",".join(char.properties),
                                char.description,
                                value,
                            )
                        )
        except Exception as e:
            print(
                "Connection error for service discovery, aborting (may happen on secured or already paired devices)\n",
                e,
            )
            print(traceback.print_exc())
            raise e
