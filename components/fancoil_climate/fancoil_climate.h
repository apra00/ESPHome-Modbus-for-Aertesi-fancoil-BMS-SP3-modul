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

  // Called from raw_* sensor on_value lambdas after every successful poll.
  void sync_state_from_globals(
      int system_status, int operation_mode, int fan_speed,
      int setpoint_raw, float current_temp) {

    this->current_temperature = current_temp;
    this->target_temperature  = setpoint_raw / 10.0f;

    switch (fan_speed) {
      case 1:  this->fan_mode = climate::CLIMATE_FAN_LOW;    break;
      case 2:  this->fan_mode = climate::CLIMATE_FAN_MEDIUM; break;
      case 3:  this->fan_mode = climate::CLIMATE_FAN_HIGH;   break;
      default: this->fan_mode = climate::CLIMATE_FAN_AUTO;   break;
    }

    if (system_status == 0) {
      this->mode = climate::CLIMATE_MODE_OFF;
    } else if (system_status == 1) {
      this->mode = climate::CLIMATE_MODE_FAN_ONLY;
    } else {
      switch (operation_mode) {
        case 0:  this->mode = climate::CLIMATE_MODE_COOL; break;
        case 1:  this->mode = climate::CLIMATE_MODE_HEAT; break;
        default: this->mode = climate::CLIMATE_MODE_AUTO; break;
      }
    }

    this->publish_state();
  }

  // Callback type so the YAML lambda can inject the write logic without
  // the component needing to know about globals or scripts directly.
  void set_control_callback(std::function<void(climate::ClimateCall)> cb) {
    this->control_cb_ = cb;
  }

 protected:
  void control(const climate::ClimateCall &call) override {
    if (this->control_cb_) {
      this->control_cb_(call);
    }
  }

 private:
  std::function<void(climate::ClimateCall)> control_cb_;
};

}  // namespace fancoil_climate
}  // namespace esphome
