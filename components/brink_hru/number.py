import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import ENTITY_CATEGORY_CONFIG

from . import CONF_BRINK_HRU_ID, BrinkHru, BrinkTarget, brink_hru_ns

DEPENDENCIES = ["brink_hru"]

BrinkHruNumber = brink_hru_ns.class_("BrinkHruNumber", number.Number, cg.Parented)

CONF_VENTILATION = "ventilation"
CONF_MINIMUM_ATMOSPHERIC_TEMPERATURE = "minimum_atmospheric_temperature"
CONF_MINIMUM_INDOOR_TEMPERATURE = "minimum_indoor_temperature"

# key -> (target enum, min, max, step, unit, icon)
_NUMBERS = {
    CONF_VENTILATION: (BrinkTarget.TARGET_VENTILATION, 0, 100, 1, "%", "mdi:fan"),
    CONF_MINIMUM_ATMOSPHERIC_TEMPERATURE: (
        BrinkTarget.TARGET_U4,
        5,
        20,
        1,
        "°C",
        "mdi:thermometer-low",
    ),
    CONF_MINIMUM_INDOOR_TEMPERATURE: (
        BrinkTarget.TARGET_U5,
        18,
        30,
        1,
        "°C",
        "mdi:home-thermometer",
    ),
}

_SETTERS = {
    CONF_VENTILATION: "set_ventilation_number",
    CONF_MINIMUM_ATMOSPHERIC_TEMPERATURE: "set_u4_number",
    CONF_MINIMUM_INDOOR_TEMPERATURE: "set_u5_number",
}


def _number_schema(unit, icon):
    return number.number_schema(
        BrinkHruNumber,
        unit_of_measurement=unit,
        icon=icon,
        entity_category=ENTITY_CATEGORY_CONFIG,
    )


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_BRINK_HRU_ID): cv.use_id(BrinkHru),
        **{
            cv.Optional(key): _number_schema(unit, icon)
            for key, (_, _, _, _, unit, icon) in _NUMBERS.items()
        },
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_BRINK_HRU_ID])
    for key, (target, lo, hi, step, _unit, _icon) in _NUMBERS.items():
        if key not in config:
            continue
        n = await number.new_number(
            config[key], min_value=lo, max_value=hi, step=step
        )
        await cg.register_parented(n, config[CONF_BRINK_HRU_ID])
        cg.add(n.set_target(target))
        cg.add(getattr(hub, _SETTERS[key])(n))
