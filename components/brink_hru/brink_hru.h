#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/optional.h"

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif

#include "OpenTherm.h"

namespace esphome {
namespace brink_hru {

// Writable parameters exposed as numbers.
enum BrinkTarget {
  TARGET_VENTILATION = 0,  // VentNomVentSet, 0-100 %
  TARGET_U4,               // Minimum atmospheric temperature bypass (°C)
  TARGET_U5,               // Minimum indoor temperature bypass (°C)
};

class BrinkHru : public PollingComponent {
 public:
  void set_in_pin(uint8_t pin) { this->in_pin_ = pin; }
  void set_out_pin(uint8_t pin) { this->out_pin_ = pin; }
  // Maximum available volume of the unit in m3/h (Brink "U" max). Used to turn
  // the 0-100 % volume readings into m3/h, matching the original sketch.
  void set_max_volume(float max_volume) { this->max_volume_ = max_volume; }
  // Hysteresis (in %) for the yield-to-switch logic on the ventilation level.
  void set_ventilation_tolerance(uint8_t tolerance) { this->ventilation_tolerance_ = tolerance; }

#ifdef USE_SENSOR
  void set_supply_temperature_sensor(sensor::Sensor *s) { this->supply_temperature_sensor_ = s; }
  void set_exhaust_temperature_sensor(sensor::Sensor *s) { this->exhaust_temperature_sensor_ = s; }
  void set_input_volume_sensor(sensor::Sensor *s) { this->input_volume_sensor_ = s; }
  void set_output_volume_sensor(sensor::Sensor *s) { this->output_volume_sensor_ = s; }
  void set_fan_level_sensor(sensor::Sensor *s) { this->fan_level_sensor_ = s; }
  void set_pressure_input_sensor(sensor::Sensor *s) { this->pressure_input_sensor_ = s; }
  void set_pressure_output_sensor(sensor::Sensor *s) { this->pressure_output_sensor_ = s; }
  void set_bypass_status_sensor(sensor::Sensor *s) { this->bypass_status_sensor_ = s; }
  void set_frost_status_sensor(sensor::Sensor *s) { this->frost_status_sensor_ = s; }
  void set_fault_code_sensor(sensor::Sensor *s) { this->fault_code_sensor_ = s; }
  void set_imbalance_sensor(sensor::Sensor *s) { this->imbalance_sensor_ = s; }
#endif
#ifdef USE_BINARY_SENSOR
  void set_fault_binary_sensor(binary_sensor::BinarySensor *s) { this->fault_binary_sensor_ = s; }
  void set_filter_binary_sensor(binary_sensor::BinarySensor *s) { this->filter_binary_sensor_ = s; }
  void set_ventilation_mode_binary_sensor(binary_sensor::BinarySensor *s) {
    this->ventilation_mode_binary_sensor_ = s;
  }
#endif
#ifdef USE_NUMBER
  void set_ventilation_number(number::Number *n) { this->ventilation_number_ = n; }
  void set_u4_number(number::Number *n) { this->u4_number_ = n; }
  void set_u5_number(number::Number *n) { this->u5_number_ = n; }
#endif

  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Called from BrinkHruNumber::control().
  void write_target(BrinkTarget target, float value);

  OpenTherm *opentherm() { return this->ot_; }

 protected:
  uint8_t in_pin_{4};
  uint8_t out_pin_{5};
  float max_volume_{296.0f};
  uint8_t ventilation_tolerance_{4};

  OpenTherm *ot_{nullptr};
  // Round-robin read pointer: one OpenTherm exchange (~34 ms, blocking) per
  // update() tick so we never stall the main loop with a full burst.
  uint8_t step_{0};

  // Yield-to-switch state for the nominal ventilation level. desired_ventilation_
  // holds what HA last commanded (empty = HA has issued nothing / has yielded);
  // last_seen_ is the previous reading, used to spot an external (wall-switch)
  // change versus a passive drift.
  optional<uint8_t> desired_ventilation_{};
  optional<uint8_t> last_seen_ventilation_{};

#ifdef USE_SENSOR
  sensor::Sensor *supply_temperature_sensor_{nullptr};
  sensor::Sensor *exhaust_temperature_sensor_{nullptr};
  sensor::Sensor *input_volume_sensor_{nullptr};
  sensor::Sensor *output_volume_sensor_{nullptr};
  sensor::Sensor *fan_level_sensor_{nullptr};
  sensor::Sensor *pressure_input_sensor_{nullptr};
  sensor::Sensor *pressure_output_sensor_{nullptr};
  sensor::Sensor *bypass_status_sensor_{nullptr};
  sensor::Sensor *frost_status_sensor_{nullptr};
  sensor::Sensor *fault_code_sensor_{nullptr};
  sensor::Sensor *imbalance_sensor_{nullptr};
#endif
#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *fault_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *filter_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *ventilation_mode_binary_sensor_{nullptr};
#endif
#ifdef USE_NUMBER
  number::Number *ventilation_number_{nullptr};
  number::Number *u4_number_{nullptr};
  number::Number *u5_number_{nullptr};
#endif
};

#ifdef USE_NUMBER
class BrinkHruNumber : public number::Number, public Parented<BrinkHru> {
 public:
  void set_target(BrinkTarget target) { this->target_ = target; }

 protected:
  void control(float value) override { this->parent_->write_target(this->target_, value); }
  BrinkTarget target_{TARGET_VENTILATION};
};
#endif

}  // namespace brink_hru
}  // namespace esphome
