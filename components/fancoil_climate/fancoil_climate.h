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

  // Inject the write callback from YAML (has access to id() and globals).
  void set_control_callback(std::function<void(const climate::ClimateCall &)> cb) {
    control_cb_ = std::move(cb);
  }

  // Called from every raw_* sensor on_value that feeds the climate card.
  void sync_from_globals(int sys, int mode, int fan, int sp_raw, float cur_temp) {
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
      switch (mode) {
        case 0:  this->mode = climate::CLIMATE_MODE_COOL; break;
        case 1:  this->mode = climate::CLIMATE_MODE_HEAT; break;
        default: this->mode = climate::CLIMATE_MODE_AUTO; break;
      }
    }
    publish_state();
  }

 protected:
  void control(const climate::ClimateCall &call) override {
    if (control_cb_) control_cb_(call);
  }

 private:
  std::function<void(const climate::ClimateCall &)> control_cb_;
};

}  // namespace fancoil_climate
}  // namespace esphome
