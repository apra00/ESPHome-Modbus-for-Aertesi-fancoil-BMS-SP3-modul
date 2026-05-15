# ESPHome – Aertesi BMS-SP3 Fancoil Controller

> Plug-and-play ESPHome configuration for integrating the **Aertesi BMS-SP3** fancoil controller with Home Assistant via Modbus RTU over RS485.

---

## Hardware

| Component | Details |
|---|---|
| MCU | ESP32-C3 (tested: `esp32-c3-devkitm-1`) |
| RS485 board | EstarDyn (or compatible) RS485 TTL module |
| UART RX | GPIO20 |
| UART TX | GPIO21 |
| Baud rate | 9600, 8N2 (no parity, 2 stop bits) |
| Protocol | Modbus RTU |

---

## Features

- **Full HA integration** — auto-discovered via native ESPHome API
- **Read sensors** — room temperature, water temperature, active setpoint, PWM valve output
- **Binary sensors** — plant running, valve 1 state, "too cool" alarm, hot-start flag
- **Status text sensors** — actual fan output speed and heat/cool mode from device
- **Writable controls** — system status (on/off/forced), operation mode, fan speed, temperature setpoint (slider), setpoint priority, supervision enable
- **Bidirectional sync** — manual keypad changes on the SP3 are reflected in HA within 15 seconds
- **Watchdog-safe** — 15 s polling keeps the SP3 supervision watchdog (60 s timeout) fed

---

## Protocol Compliance (§5 & §7)

The SP3 only accepts two Modbus function codes:

- **FC03** — read holding registers
- **FC16** — write multiple registers

Critically, **§7 of the SP3 manual** requires that all 8 R/W registers (addresses 1096–1103) be written **atomically in a single FC16 command**. ESPHome's built-in `modbus_controller` SELECT uses FC06 and NUMBER uses single-register FC16 — both are rejected by the SP3 with exception `0x86`.

### Architecture

This config works around that constraint with a clean two-layer design:

```
HA entity change
      │
      ▼
template select / template number
  (set_action → update shadow global → write_all_registers)
      │
      ▼
write_all_registers (sole write path)
  FC16 · addr 1096 · count 8  (all 8 registers in one command)
      │
      ▼
Aertesi BMS-SP3

      ▲
      │
modbus_controller sensors (FC03, every 15 s)
  → update shadow globals → component.update → template lambda
  (reflects SP3 state back to HA, including keypad changes)
```

- **No FC06 ever.** No single-register FC16 ever.
- Shadow globals (`g_*`) are the source of truth; `restore_value: true` persists them in NVS across reboots.
- `write_all_registers` script always sends the full 8-register payload.

---

## Setup

1. Copy `fancoil-bms-sp3.yaml` into your ESPHome config directory.
2. Add the following to your `secrets.yaml`:
   ```yaml
   wifi_ssid: "YourSSID"
   wifi_password: "YourPassword"
   api_encryption_key: "your-32-byte-base64-key"
   ota_password: "your-ota-password"
   ```
3. Edit the `substitutions` block at the top of the YAML if needed:
   ```yaml
   substitutions:
     modbus_address: "1"      # Match the Adr set on the SP3 board
     setpoint_min: "16"
     setpoint_max: "30"
   ```
4. If your RS485 board requires a manual DE/RE direction pin, uncomment `flow_control_pin` under the `modbus:` section and set the correct GPIO.
5. Adjust `static_ip`, `gateway`, and `subnet` under `wifi: manual_ip:` to match your network.
6. Flash: `esphome run fancoil-bms-sp3.yaml`

---

## Entities

### Sensors
| Name | Address | Notes |
|---|---|---|
| Air Temperature (Keyboard Probe) | 1066 | °C × 0.1, signed |
| Water Temperature | 1129 | °C × 0.1, signed |
| Active Setpoint | 1239 | °C × 0.1, diagnostic |
| PWM AO1 Output | 1238 | V × 0.1, diagnostic |

### Binary Sensors
| Name | Address | Bitmask |
|---|---|---|
| Plant Running | 1227 | 0x1 |
| Valve 1 | 1164 | 0x1 |
| Too Cool Alarm | 1130 | 0x1 |
| Hot Start | 1131 | 0x1 |

### Text Sensors
| Name | Address | Values |
|---|---|---|
| Fan Output Speed | 1166 | OFF / Min / Med / Max |
| Heat/Cool Status | 1229 | Summer (Cooling) / Winter (Heating) |

### Controls (all write via FC16 addr 1096, count 8)
| Name | Type | Values / Range |
|---|---|---|
| System Status | Select | OFF Forced / ON/OFF from Keyboard / ON Forced |
| Operation Mode | Select | Summer (Cooling) / Winter (Heating) / Automatic |
| Fan Speed | Select | From Keyboard / Min / Med / Max / Auto |
| Temperature Setpoint | Number | 16–30 °C, step 0.5 |
| Setpoint Priority | Select | Keyboard / Supervision System |
| Supervision Control | Select | Disabled / Enabled |

---

## Notes

- `logger: baud_rate: 0` is required — the ESP32-C3's UART0 shares GPIO20/21 with the RS485 module; enabling hardware logging corrupts Modbus traffic.
- `output_power: 10dB` is set conservatively to reduce RF noise near sensitive RS485 wiring.
- The `on_boot` sequence waits 5 s then writes all registers, ensuring the SP3 is in the expected supervision state on every restart.

---

## License

MIT
