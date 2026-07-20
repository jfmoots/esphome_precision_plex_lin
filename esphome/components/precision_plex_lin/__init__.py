import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

CONF_UART_ID = "uart_id"

DEPENDENCIES = ["uart"]
MULTI_CONF = False

precision_plex_lin_ns = cg.esphome_ns.namespace("precision_plex_lin")
PrecisionPlexLinComponent = precision_plex_lin_ns.class_(
    "PrecisionPlexLinComponent", cg.Component
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PrecisionPlexLinComponent),
        cv.Required(CONF_UART_ID): cv.use_id(uart.UARTComponent),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    uart_parent = await cg.get_variable(config[CONF_UART_ID])
    cg.add(var.set_uart_parent(uart_parent))
