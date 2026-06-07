import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_PRESSURE,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLUME_FLOW_RATE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PASCAL,
)

from . import CONF_BRINK_HRU_ID, BrinkHru

DEPENDENCIES = ["brink_hru"]

UNIT_CUBIC_METER_PER_HOUR = "m³/h"

CONF_SUPPLY_TEMPERATURE = "supply_temperature"
CONF_EXHAUST_TEMPERATURE = "exhaust_temperature"
CONF_INPUT_VOLUME = "input_volume"
CONF_OUTPUT_VOLUME = "output_volume"
CONF_FAN_LEVEL = "fan_level"
CONF_PRESSURE_INPUT = "pressure_input"
CONF_PRESSURE_OUTPUT = "pressure_output"
CONF_BYPASS_STATUS = "bypass_status"
CONF_FROST_STATUS = "frost_status"
CONF_FAULT_CODE = "fault_code"
CONF_IMBALANCE = "imbalance"


def _temperature_schema():
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=1,
    )


def _volume_schema():
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_CUBIC_METER_PER_HOUR,
        device_class=DEVICE_CLASS_VOLUME_FLOW_RATE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=0,
        icon="mdi:wind-power",
    )


def _pressure_schema():
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_PASCAL,
        device_class=DEVICE_CLASS_PRESSURE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=0,
    )


def _enum_schema(icon="mdi:numeric"):
    return sensor.sensor_schema(
        accuracy_decimals=0,
        icon=icon,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    )


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_BRINK_HRU_ID): cv.use_id(BrinkHru),
        cv.Optional(CONF_SUPPLY_TEMPERATURE): _temperature_schema(),
        cv.Optional(CONF_EXHAUST_TEMPERATURE): _temperature_schema(),
        cv.Optional(CONF_INPUT_VOLUME): _volume_schema(),
        cv.Optional(CONF_OUTPUT_VOLUME): _volume_schema(),
        cv.Optional(CONF_FAN_LEVEL): _enum_schema(icon="mdi:fan"),
        cv.Optional(CONF_PRESSURE_INPUT): _pressure_schema(),
        cv.Optional(CONF_PRESSURE_OUTPUT): _pressure_schema(),
        cv.Optional(CONF_BYPASS_STATUS): _enum_schema(icon="mdi:valve"),
        cv.Optional(CONF_FROST_STATUS): _enum_schema(icon="mdi:snowflake"),
        cv.Optional(CONF_FAULT_CODE): _enum_schema(icon="mdi:alert-circle"),
        cv.Optional(CONF_IMBALANCE): _enum_schema(icon="mdi:scale-balance"),
    }
)

_SETTERS = {
    CONF_SUPPLY_TEMPERATURE: "set_supply_temperature_sensor",
    CONF_EXHAUST_TEMPERATURE: "set_exhaust_temperature_sensor",
    CONF_INPUT_VOLUME: "set_input_volume_sensor",
    CONF_OUTPUT_VOLUME: "set_output_volume_sensor",
    CONF_FAN_LEVEL: "set_fan_level_sensor",
    CONF_PRESSURE_INPUT: "set_pressure_input_sensor",
    CONF_PRESSURE_OUTPUT: "set_pressure_output_sensor",
    CONF_BYPASS_STATUS: "set_bypass_status_sensor",
    CONF_FROST_STATUS: "set_frost_status_sensor",
    CONF_FAULT_CODE: "set_fault_code_sensor",
    CONF_IMBALANCE: "set_imbalance_sensor",
}


async def to_code(config):
    hub = await cg.get_variable(config[CONF_BRINK_HRU_ID])
    for key, setter in _SETTERS.items():
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(hub, setter)(sens))
