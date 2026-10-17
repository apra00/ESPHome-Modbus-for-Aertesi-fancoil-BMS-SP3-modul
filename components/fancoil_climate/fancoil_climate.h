#pragma once
#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"

namespace esphome {
namespace fancoil_climate {

class FancoilClimate : public climate::Climate, public Component {
 public:

  climate::ClimateTraits traits() override {
    auto t = climate::ClimateTraits();
    t.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE |
                        climate::CLIMATE_SUPPORTS_ACTION);
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

  // Called from raw_* sensor on_value lambdas after a poll (write lock clear).
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
    update_action_();
    publish_state();
  }

  // Valve 1 (addr 1164) is the only signal that the unit is actually
  // conditioning the room; "Plant Running" only means it is switched on.
  void set_valve_open(bool open) {
    valve_open_ = open;
    if (update_action_())
      publish_state();
  }

  // Heat|Cool Status (addr 1229): what the unit is really doing, also in auto.
  void set_heating_season(bool heating) {
    heating_season_ = heating;
    if (update_action_())
      publish_state();
  }

 protected:
  bool valve_open_{false};
  bool heating_season_{true};

  // Derives action from mode + valve + season; returns true if it changed.
  bool update_action_() {
    climate::ClimateAction a;
    if (this->mode == climate::CLIMATE_MODE_OFF)
      a = climate::CLIMATE_ACTION_OFF;
    else if (!valve_open_)
      a = climate::CLIMATE_ACTION_IDLE;
    else
      a = heating_season_ ? climate::CLIMATE_ACTION_HEATING : climate::CLIMATE_ACTION_COOLING;
    if (a == this->action)
      return false;
    this->action = a;
    return true;
  }

  // The base class Climate::perform() fires add_on_control_callback (which
  // runs the YAML on_control: automation) THEN calls this control() override.
  // So by the time we arrive here the on_control lambda has already updated
  // the globals and queued the FC16 write. We just apply the call fields to
  // our own published state for an immediate optimistic HA card update.
  void control(const climate::ClimateCall &call) override {
    if (call.get_mode().has_value())
      this->mode = *call.get_mode();
    if (call.get_fan_mode().has_value())
      this->fan_mode = *call.get_fan_mode();
    if (call.get_target_temperature().has_value())
      this->target_temperature = *call.get_target_temperature();
    update_action_();
    publish_state();
  }
};

}  // namespace fancoil_climate
}  // namespace esphome
