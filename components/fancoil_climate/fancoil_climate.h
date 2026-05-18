#pragma once
#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include <functional>

namespace esphome {
namespace fancoil_climate {

// Forward-declare globals defined in the YAML.
extern int g_system_status;
extern int g_operation_mode;
extern int g_fan_speed;
extern int g_setpoint;
extern int g_write_pending;

class FancoilClimate : public climate::Climate, public Component {
 public:
  void setup() override {}

  // Called once from the YAML on_boot lambda to wire up write_all_registers.
  void set_write_cb(std::function<void()> cb) {
    write_cb_ = std::move(cb);
  }

  climate::ClimateTraits traits() override {
    auto t = climate::ClimateTraits();

    t.add_supported_mode(climate::CLIMATE_MODE_OFF);
    t.add_supported_mode(climate::CLIMATE_MODE_HEAT);
    t.add_supported_mode(climate::CLIMATE_MODE_COOL);
    t.add_supported_mode(climate::CLIMATE_MODE_HEAT_COOL);

    t.add_supported_fan_mode(climate::CLIMATE_FAN_AUTO);
    t.add_supported_fan_mode(climate::CLIMATE_FAN_LOW);
    t.add_supported_fan_mode(climate::CLIMATE_FAN_MEDIUM);
    t.add_supported_fan_mode(climate::CLIMATE_FAN_HIGH);

    t.add_feature_flag(climate::ClimateTraitFlags::CLIMATE_TRAIT_CURRENT_TEMPERATURE);

    t.set_visual_min_temperature(16.0f);
    t.set_visual_max_temperature(30.0f);
    t.set_visual_temperature_step(0.5f);

    return t;
  }

  void control(const climate::ClimateCall &call) override {
    bool changed = false;

    // Arm write-pending guard synchronously before anything else so no
    // Modbus poll can slip in and overwrite globals between now and the write.
    g_write_pending = 6;

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

    if (call.get_target_temperature().has_value()) {
      this->target_temperature = *call.get_target_temperature();
      changed = true;
      g_setpoint = (int)(this->target_temperature * 10.0f);
    }

    if (changed) {
      this->publish_state();
      if (write_cb_) write_cb_();
    }
  }

 protected:
  std::function<void()> write_cb_;
};

}  // namespace fancoil_climate
}  // namespace esphome
