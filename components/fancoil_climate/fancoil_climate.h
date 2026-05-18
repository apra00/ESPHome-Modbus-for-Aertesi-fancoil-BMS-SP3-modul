#pragma once
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/climate/climate.h"

namespace esphome {
namespace fancoil_climate {

class FancoilClimate;

// Trigger fired after control() updates state -- YAML automation calls
// write_all_registers in response.
class FancoilClimateControlTrigger : public Trigger<const climate::ClimateCall &> {
 public:
  explicit FancoilClimateControlTrigger(FancoilClimate *parent);
};

// ─────────────────────────────────────────────────────────────────────────────
// FancoilClimate
//
// Plain ESPHome climate component for the Aertesi BMS-SP3 fancoil.
// - Single target_temperature (no low/high range)
// - Modes  : off, heat, cool, heat_cool
// - Fan    : auto, low, medium, high
// - No built-in hysteresis -- SP3 owns thermal control
//
// control() updates the shadow globals and fires on_control so the YAML
// automation can call write_all_registers.
// ─────────────────────────────────────────────────────────────────────────────

// Forward-declare globals set in the YAML.
extern int g_system_status;
extern int g_operation_mode;
extern int g_fan_speed;
extern int g_setpoint;

class FancoilClimate : public climate::Climate, public Component {
 public:
  void setup() override {}

  climate::ClimateTraits traits() override {
    auto t = climate::ClimateTraits();
    t.set_supports_current_temperature(true);
    t.set_supports_two_point_target_temperature(false);
    t.set_visual_min_temperature(16.0f);
    t.set_visual_max_temperature(30.0f);
    t.set_visual_temperature_step(0.5f);
    t.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_COOL,
      climate::CLIMATE_MODE_HEAT_COOL,
    });
    t.set_supported_fan_modes({
      climate::CLIMATE_FAN_AUTO,
      climate::CLIMATE_FAN_LOW,
      climate::CLIMATE_FAN_MEDIUM,
      climate::CLIMATE_FAN_HIGH,
    });
    return t;
  }

  void control(const climate::ClimateCall &call) override {
    bool changed = false;

    // ── Mode → system_status + operation_mode ────────────────────────────────
    if (call.get_mode().has_value()) {
      this->mode = *call.get_mode();
      changed = true;
      switch (this->mode) {
        case climate::CLIMATE_MODE_OFF:
          g_system_status  = 0;
          break;
        case climate::CLIMATE_MODE_HEAT:
          g_system_status  = 2;
          g_operation_mode = 1;
          break;
        case climate::CLIMATE_MODE_COOL:
          g_system_status  = 2;
          g_operation_mode = 0;
          break;
        case climate::CLIMATE_MODE_HEAT_COOL:
        default:
          g_system_status  = 2;
          g_operation_mode = 2;
          break;
      }
    }

    // ── Fan mode → fan_speed ─────────────────────────────────────────────────
    if (call.get_fan_mode().has_value()) {
      this->fan_mode = *call.get_fan_mode();
      changed = true;
      switch (*this->fan_mode) {
        case climate::CLIMATE_FAN_LOW:    g_fan_speed = 1; break;
        case climate::CLIMATE_FAN_MEDIUM: g_fan_speed = 2; break;
        case climate::CLIMATE_FAN_HIGH:   g_fan_speed = 3; break;
        default:                          g_fan_speed = 4; break;
      }
    }

    // ── Target temperature → setpoint (x10) ──────────────────────────────────
    if (call.get_target_temperature().has_value()) {
      this->target_temperature = *call.get_target_temperature();
      changed = true;
      g_setpoint = (int)(this->target_temperature * 10.0f);
    }

    if (changed) {
      this->publish_state();
      this->control_trigger_->trigger(call);
    }
  }

  void register_control_trigger(FancoilClimateControlTrigger *t) {
    this->control_trigger_ = t;
  }

 protected:
  FancoilClimateControlTrigger *control_trigger_{nullptr};
};

// ── Trigger implementation ────────────────────────────────────────────────────
inline FancoilClimateControlTrigger::FancoilClimateControlTrigger(FancoilClimate *parent) {
  parent->register_control_trigger(this);
}

}  // namespace fancoil_climate
}  // namespace esphome
