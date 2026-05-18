#pragma once
#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"

namespace esphome {
namespace fancoil_climate {

class FancoilClimate : public climate::Climate, public Component {
 public:

  climate::ClimateTraits traits() override {
    auto t = climate::ClimateTraits();
    t.set_supports_current_temperature(true);
    t.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_COOL,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_AUTO,
      climate::CLIMATE_MODE_FAN_ONLY,
    });
    t.set_supported_fan_modes({
      climate::CLIMATE_FAN_AUTO,
      climate::CLIMATE_FAN_LOW,
      climate::CLIMATE_FAN_MEDIUM,
      climate::CLIMATE_FAN_HIGH,
    });
    t.set_visual_min_temperature(16);
    t.set_visual_max_temperature(30);
    t.set_visual_temperature_step(0.5f);
    return t;
  }

  // Called from raw_* sensor on_value lambdas after a successful poll.
  // The caller checks g_write_lock before calling this.
  void sync_from_globals(int sys, int op_mode, int fan, int sp_raw, float cur_temp) {
    current_temperature = cur_temp;
    target_temperature  = sp_raw / 10.0f;

    switch (fan) {
      case 1:  fan_mode = climate::CLIMATE_FAN_LOW;    break;
      case 2:  fan_mode = climate::CLIMATE_FAN_MEDIUM; break;
      case 3:  fan_mode = climate::CLIMATE_FAN_HIGH;   break;
      default: fan_mode = climate::CLIMATE_FAN_AUTO;   break;
    }

    if (sys == 0) {
      this->mode = climate::CLIMATE_MODE_OFF;
    } else if (sys == 1) {
      this->mode = climate::CLIMATE_MODE_FAN_ONLY;
    } else {
      switch (op_mode) {
        case 0:  this->mode = climate::CLIMATE_MODE_COOL; break;
        case 1:  this->mode = climate::CLIMATE_MODE_HEAT; break;
        default: this->mode = climate::CLIMATE_MODE_AUTO; break;
      }
    }
    publish_state();
  }

 protected:
  // control() is called by the ESPHome framework when HA sends a command.
  // The on_control automation (built by climate.py -> build_automation) is
  // wired via ControlTrigger which fires from Climate::perform() BEFORE
  // control() is called, so by the time we arrive here the globals are
  // already updated and write_all_registers has been queued.
  // We apply the ClimateCall fields to our own published state so HA gets
  // an immediate optimistic update without waiting for the next 15 s poll.
  void control(const climate::ClimateCall &call) override {
    if (call.get_mode().has_value())
      this->mode = *call.get_mode();
    if (call.get_fan_mode().has_value())
      this->fan_mode = *call.get_fan_mode();
    if (call.get_target_temperature().has_value())
      this->target_temperature = *call.get_target_temperature();
    publish_state();
  }
};

}  // namespace fancoil_climate
}  // namespace esphome
