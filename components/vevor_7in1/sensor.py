"""Plateforme `vevor_7in1` : déclare le composant et ses entités.

Une seule plateforme porte tout : les entités sont imbriquées sous la clé
`platform: vevor_7in1` (même approche que `bme280`). Aucun décor de décodage ici.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, remote_base, sensor, text_sensor
from esphome.components.remote_receiver import RemoteReceiverComponent
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_ILLUMINANCE,
    DEVICE_CLASS_PRECIPITATION,
    DEVICE_CLASS_SIGNAL_STRENGTH,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_WIND_DIRECTION,
    DEVICE_CLASS_WIND_SPEED,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_CELSIUS,
    UNIT_DEGREES,
    UNIT_KILOMETER_PER_HOUR,
    UNIT_LUX,
    UNIT_MILLIMETER,
    UNIT_PERCENT,
    UNIT_SECOND,
)

from . import Vevor7in1Component

# AUTO_LOAD doit vivre ICI et pas seulement dans __init__.py : quand un composant
# n'est chargé que comme plateforme (pas de bloc « hub »), c'est le module de
# plateforme que lit ESPHome. Or chaque domaine d'entité doit exister comme clé
# de premier niveau pour que ses sources soient copiées dans le build (sinon
# entity_includes.h inclut un text_sensor.h absent). Voir README.
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor"]

CONF_REMOTE_RECEIVER_ID = "remote_receiver_id"

CONF_TEMPERATURE = "temperature"
CONF_HUMIDITY = "humidity"
CONF_WIND_SPEED = "wind_speed"
CONF_WIND_GUST = "wind_gust"
CONF_WIND_DIRECTION = "wind_direction"
CONF_RAIN_TOTAL = "rain_total"
CONF_UV_INDEX = "uv_index"
CONF_ILLUMINANCE = "illuminance"
CONF_BATTERY_LOW = "battery_low"
CONF_STATION_ID = "station_id"
CONF_FRAME_AGE = "frame_age"
CONF_SIGNAL_QUALITY = "signal_quality"
CONF_FRAMES_VALID = "frames_valid"
CONF_FRAMES_INVALID = "frames_invalid"
CONF_BIT_PERIOD = "bit_period"
CONF_LAST_STATUS = "last_status"

# (clé de configuration, accesseur C++ généré par la macro SUB_SENSOR)
_SENSORS = (
    (
        CONF_TEMPERATURE,
        "set_temperature_sensor",
        dict(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    ),
    (
        CONF_HUMIDITY,
        "set_humidity_sensor",
        dict(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_HUMIDITY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    ),
    (
        CONF_WIND_SPEED,
        "set_wind_speed_sensor",
        dict(
            unit_of_measurement=UNIT_KILOMETER_PER_HOUR,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_WIND_SPEED,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:weather-windy",
        ),
    ),
    (
        CONF_WIND_GUST,
        "set_wind_gust_sensor",
        dict(
            unit_of_measurement=UNIT_KILOMETER_PER_HOUR,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_WIND_SPEED,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:weather-windy-variant",
        ),
    ),
    (
        CONF_WIND_DIRECTION,
        "set_wind_direction_sensor",
        dict(
            unit_of_measurement=UNIT_DEGREES,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_WIND_DIRECTION,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:compass-outline",
        ),
    ),
    (
        CONF_RAIN_TOTAL,
        "set_rain_total_sensor",
        dict(
            unit_of_measurement=UNIT_MILLIMETER,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_PRECIPITATION,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            icon="mdi:weather-pouring",
        ),
    ),
    (CONF_UV_INDEX, "set_uv_index_sensor", dict(accuracy_decimals=0, icon="mdi:weather-sunny-alert")),
    (
        CONF_ILLUMINANCE,
        "set_illuminance_sensor",
        dict(
            unit_of_measurement=UNIT_LUX,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_ILLUMINANCE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:brightness-5",
        ),
    ),
    (
        CONF_FRAME_AGE,
        "set_frame_age_sensor",
        dict(
            unit_of_measurement=UNIT_SECOND,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:timer-sand",
        ),
    ),
    (
        CONF_SIGNAL_QUALITY,
        "set_signal_quality_sensor",
        dict(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_SIGNAL_STRENGTH,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:signal",
        ),
    ),
    (
        CONF_FRAMES_VALID,
        "set_frames_valid_sensor",
        dict(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:counter"),
    ),
    (
        CONF_FRAMES_INVALID,
        "set_frames_invalid_sensor",
        dict(accuracy_decimals=0, state_class=STATE_CLASS_TOTAL_INCREASING, icon="mdi:alert-circle-outline"),
    ),
    (
        CONF_BIT_PERIOD,
        "set_bit_period_sensor",
        dict(unit_of_measurement="us", accuracy_decimals=2, icon="mdi:sine-wave"),
    ),
)

_BINARY_SENSORS = (
    (
        CONF_BATTERY_LOW,
        "set_battery_low_binary_sensor",
        dict(device_class=DEVICE_CLASS_BATTERY, icon="mdi:battery-alert"),
    ),
)

_TEXT_SENSORS = (
    (CONF_STATION_ID, "set_station_id_text_sensor", dict(icon="mdi:identifier")),
    (CONF_LAST_STATUS, "set_last_status_text_sensor", dict(icon="mdi:information-outline")),
)


def _sensor_schema(entries):
    return {cv.Optional(key): sensor.sensor_schema(**kwargs) for key, _, kwargs in entries}


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(Vevor7in1Component),
        cv.Required(CONF_REMOTE_RECEIVER_ID): cv.use_id(RemoteReceiverComponent),
        **_sensor_schema(_SENSORS),
        **{
            cv.Optional(key): binary_sensor.binary_sensor_schema(**kwargs)
            for key, _, kwargs in _BINARY_SENSORS
        },
        **{
            cv.Optional(key): text_sensor.text_sensor_schema(**kwargs)
            for key, _, kwargs in _TEXT_SENSORS
        },
    }
).extend(cv.polling_component_schema("1s"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Le lien avec le CC1101 est matériel ; côté logiciel, le remote_receiver est
    # la source des timings.
    receiver = await cg.get_variable(config[CONF_REMOTE_RECEIVER_ID])
    if hasattr(remote_base, "add_listener"):
        # Branche de développement : la place de listener est comptée ici même.
        remote_base.add_listener(receiver, var)
    else:
        # Branche stable : remote_base.register_listener() pousse directement
        # dans le vecteur de listeners du receiver.
        cg.add(receiver.register_listener(var))

    for key, setter, _ in _SENSORS:
        if key in config:
            cg.add(getattr(var, setter)(await sensor.new_sensor(config[key])))
    for key, setter, _ in _BINARY_SENSORS:
        if key in config:
            cg.add(getattr(var, setter)(await binary_sensor.new_binary_sensor(config[key])))
    for key, setter, _ in _TEXT_SENSORS:
        if key in config:
            cg.add(getattr(var, setter)(await text_sensor.new_text_sensor(config[key])))
