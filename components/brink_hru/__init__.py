import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@sandor"]
MULTI_CONF = True

brink_hru_ns = cg.esphome_ns.namespace("brink_hru")
BrinkHru = brink_hru_ns.class_("BrinkHru", cg.PollingComponent)
BrinkTarget = brink_hru_ns.enum("BrinkTarget")

CONF_BRINK_HRU_ID = "brink_hru_id"
CONF_IN_PIN = "in_pin"
CONF_OUT_PIN = "out_pin"
CONF_MAX_VOLUME = "max_volume"
CONF_VENTILATION_TOLERANCE = "ventilation_tolerance"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(BrinkHru),
        cv.Required(CONF_IN_PIN): cv.int_range(min=0, max=39),
        cv.Required(CONF_OUT_PIN): cv.int_range(min=0, max=39),
        cv.Optional(CONF_MAX_VOLUME, default=296.0): cv.positive_float,
        cv.Optional(CONF_VENTILATION_TOLERANCE, default=4): cv.int_range(min=0, max=100),
    }
).extend(cv.polling_component_schema("60s"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_in_pin(config[CONF_IN_PIN]))
    cg.add(var.set_out_pin(config[CONF_OUT_PIN]))
    cg.add(var.set_max_volume(config[CONF_MAX_VOLUME]))
    cg.add(var.set_ventilation_tolerance(config[CONF_VENTILATION_TOLERANCE]))
