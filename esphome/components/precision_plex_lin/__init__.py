import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import uart
from esphome.const import CONF_ID, CONF_TRIGGER_ID

CONF_UART_ID = "uart_id"
CONF_ON_SNAPSHOT = "on_snapshot"

DEPENDENCIES = ["uart"]
MULTI_CONF = False

precision_plex_lin_ns = cg.esphome_ns.namespace("precision_plex_lin")
PrecisionPlexLinComponent = precision_plex_lin_ns.class_(
    "PrecisionPlexLinComponent", cg.Component
)
SnapshotTrigger = precision_plex_lin_ns.class_(
    "SnapshotTrigger", automation.Trigger.template(cg.std_string)
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PrecisionPlexLinComponent),
        cv.Required(CONF_UART_ID): cv.use_id(uart.UARTComponent),
        cv.Optional(CONF_ON_SNAPSHOT): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(SnapshotTrigger),
            }
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    uart_parent = await cg.get_variable(config[CONF_UART_ID])
    cg.add(var.set_uart_parent(uart_parent))

    for conf in config.get(CONF_ON_SNAPSHOT, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(
            trigger,
            [(cg.std_string, "snapshot")],
            conf,
        )
