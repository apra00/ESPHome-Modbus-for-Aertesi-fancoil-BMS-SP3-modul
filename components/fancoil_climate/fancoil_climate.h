#pragma once
#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"

namespace esphome {
namespace fancoil_climate {

// ─────────────────────────────────────────────────────────────────────────────
// FancoilClimate
//
// Plain ESPHome climate component for the Aertesi BMS-SP3 fancoil.
// - Single target_temperature (no low/high range)
// - Modes  : off, heat, cool, heat_cool
// - Fan    : auto, low, medium, high
// - No built-in hysteresis or state machine -- SP3 owns thermal control
//
// control() is called by HA on every user interaction.
// It updates the shadow globals in the YAML and calls write_all_registers.
// Sync back from Modbus polls is done by calling set_state() from the YAML.
// ─────────────────────────────────────────────────────────────────────────────

class FancoilClimate : public climate::Climate, public Component {
 public:
  void setup() override {}

  climate::ClimateTraits traits() override {
    auto t = climate::ClimateTraits();

    t.set_supports_current_temperature(true);
    t.set_supports_two_point_target_temperature(false);  // single setpoint only
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

    // ── Mode ──────────────────────────────────────────────────────────────────
    if (call.get_mode().has_value()) {
      this->mode = *call.get_mode();
      changed = true;

      switch (this->mode) {
        case climate::CLIMATE_MODE_OFF:
          on_mode_off_callback_.call();
          break;
        case climate::CLIMATE_MODE_HEAT:
          on_mode_heat_callback_.call();
          break;
        case climate::CLIMATE_MODE_COOL:
          on_mode_cool_callback_.call();
          break;
        case climate::CLIMATE_MODE_HEAT_COOL:
        default:
          on_mode_heat_cool_callback_.call();
          break;
      }
    }

    // ── Fan mode ──────────────────────────────────────────────────────────────
    if (call.get_fan_mode().has_value()) {
      this->fan_mode = *call.get_fan_mode();
      changed = true;
      on_fan_mode_callback_.call(*this->fan_mode);
    }

    // ── Target temperature ────────────────────────────────────────────────────
    if (call.get_target_temperature().has_value()) {
      this->target_temperature = *call.get_target_temperature();
      changed = true;
      on_target_temperature_callback_.call(this->target_temperature);
    }

    if (changed)
      this->publish_state();
  }

  // ── Callbacks registered from YAML lambdas ────────────────────────────────
  void add_on_mode_off_callback(std::function<void()> cb)        { on_mode_off_callback_.add(std::move(cb)); }
  void add_on_mode_heat_callback(std::function<void()> cb)       { on_mode_heat_callback_.add(std::move(cb)); }
  void add_on_mode_cool_callback(std::function<void()> cb)       { on_mode_cool_callback_.add(std::move(cb)); }
  void add_on_mode_heat_cool_callback(std::function<void()> cb)  { on_mode_heat_cool_callback_.add(std::move(cb)); }
  void add_on_fan_mode_callback(std::function<void(climate::ClimateFanMode)> cb) { on_fan_mode_callback_.add(std::move(cb)); }
  void add_on_target_temperature_callback(std::function<void(float)> cb)        { on_target_temperature_callback_.add(std::move(cb)); }

 protected:
  CallbackManager<void()> on_mode_off_callback_;
  CallbackManager<void()> on_mode_heat_callback_;
  CallbackManager<void()> on_mode_cool_callback_;
  CallbackManager<void()> on_mode_heat_cool_callback_;
  CallbackManager<void(climate::ClimateFanMode)> on_fan_mode_callback_;
  CallbackManager<void(float)> on_target_temperature_callback_;
};

}  // namespace fancoil_climate
}  // namespace esphome
