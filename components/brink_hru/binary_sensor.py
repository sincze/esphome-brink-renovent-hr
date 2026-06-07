import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import DEVICE_CLASS_PROBLEM, DEVICE_CLASS_RUNNING

from . import CONF_BRINK_HRU_ID, BrinkHru

DEPENDENCIES = ["brink_hru"]

CONF_FAULT = "fault"
CONF_FILTER_DIRTY = "filter_dirty"
CONF_VENTILATION_MODE = "ventilation_mode"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_BRINK_HRU_ID): cv.use_id(BrinkHru),
        cv.Optional(CONF_FAULT): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            icon="mdi:alert-circle",
        ),
        cv.Optional(CONF_FILTER_DIRTY): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            icon="mdi:air-filter",
        ),
        cv.Optional(CONF_VENTILATION_MODE): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_RUNNING,
            icon="mdi:fan",
        ),
    }
)

_SETTERS = {
    CONF_FAULT: "set_fault_binary_sensor",
    CONF_FILTER_DIRTY: "set_filter_binary_sensor",
    CONF_VENTILATION_MODE: "set_ventilation_mode_binary_sensor",
}


async def to_code(config):
    hub = await cg.get_variable(config[CONF_BRINK_HRU_ID])
    for key, setter in _SETTERS.items():
        if key in config:
            bs = await binary_sensor.new_binary_sensor(config[key])
            cg.add(getattr(hub, setter)(bs))
