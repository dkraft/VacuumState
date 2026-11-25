# VacuumState

ESP32 MQTT vacuum controller: analog pressure on GPIO36, on/off setpoints, duty-cycle publish.

Copy `esp32_MQTT_VacuumState/secrets.h.example` to `esp32_MQTT_VacuumState/secrets.h` and fill in Wi-Fi plus MQTT. `secrets.h` is gitignored.

## Pins

| GPIO | Function |
|---|---|
| 36 | Vacuum / pressure analog |
| 17 | Up |
| 18 | Down |
| 19 | Enable switch (output in current sketch) |
| 2 | LED |

## MQTT (`AIO_USERNAME`, default `Vacuum_1`)

| Topic | Dir |
|---|---|
| `Vacuum_1/feeds/VacOn` | pub |
| `Vacuum_1/feeds/VacOff` | pub |
| `Vacuum_1/feeds/VacDuty` | pub |
| `Vacuum_1/feeds/VacVal` | pub |
| `Vacuum_1/feeds/MQTTConnections` | pub |
| `Vacuum_1/feeds/SetVacEnable` | sub/pub |
| `Vacuum_1/feeds/SetVacON` | sub/pub |
| `Vacuum_1/feeds/SetVacOFF` | sub/pub |

## Libraries

Adafruit MQTT.

## Flash

Board: ESP32. Sketch folder: `esp32_MQTT_VacuumState`.
