#include "brink_hru.h"
#include "esphome/core/log.h"

namespace esphome {
namespace brink_hru {

static const char *const TAG = "brink_hru";

// OpenTherm needs a free-function ISR. The DIYLess Master OpenTherm Shield is a
// single bus, so we support one hub instance and route its interrupt here.
static OpenTherm *global_ot = nullptr;          // NOLINT
static void IRAM_ATTR brink_handle_interrupt() {
  if (global_ot != nullptr)
    global_ot->handleInterrupt();
}

void BrinkHru::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Brink Renovent HR (in=%u out=%u)...", this->in_pin_, this->out_pin_);
  this->ot_ = new OpenTherm(this->in_pin_, this->out_pin_);  // NOLINT(cppcoreguidelines-owning-memory)
  global_ot = this->ot_;
  this->ot_->begin(brink_handle_interrupt);
}

void BrinkHru::update() {
  if (this->ot_ == nullptr)
    return;

  const float vol_factor = this->max_volume_ / 100.0f;
  const uint8_t num_steps = 17;

  // Issue exactly one OpenTherm exchange per tick. Walk forward until we hit a
  // step whose entity is configured, so unconfigured entities cost no bus time.
  for (uint8_t tries = 0; tries < num_steps; tries++) {
    const uint8_t s = this->step_;
    this->step_ = (this->step_ + 1) % num_steps;
    switch (s) {
#ifdef USE_SENSOR
      case 0:
        if (this->supply_temperature_sensor_ != nullptr) {
          this->supply_temperature_sensor_->publish_state(this->ot_->getVentSupplyInTemperature());
          return;
        }
        break;
      case 1:
        if (this->exhaust_temperature_sensor_ != nullptr) {
          this->exhaust_temperature_sensor_->publish_state(this->ot_->getVentExhaustInTemperature());
          return;
        }
        break;
      case 2:
        if (this->input_volume_sensor_ != nullptr) {
          this->input_volume_sensor_->publish_state(this->ot_->getBrinkTSP(CurrentInputVol) * vol_factor);
          return;
        }
        break;
      case 3:
        if (this->output_volume_sensor_ != nullptr) {
          this->output_volume_sensor_->publish_state(this->ot_->getBrinkTSP(CurrentOutputVol) * vol_factor);
          return;
        }
        break;
      case 4:
        if (this->fan_level_sensor_ != nullptr) {
          this->fan_level_sensor_->publish_state(this->ot_->getBrinkTSP(CurrentVol));
          return;
        }
        break;
      case 5:
        if (this->pressure_input_sensor_ != nullptr) {
          this->pressure_input_sensor_->publish_state(this->ot_->getBrinkTSP(CPID));
          return;
        }
        break;
      case 6:
        if (this->pressure_output_sensor_ != nullptr) {
          this->pressure_output_sensor_->publish_state(this->ot_->getBrinkTSP(CPOD));
          return;
        }
        break;
      case 7:
        if (this->bypass_status_sensor_ != nullptr) {
          this->bypass_status_sensor_->publish_state(this->ot_->getBrinkTSP(BypassStatus));
          return;
        }
        break;
      case 8:
        if (this->frost_status_sensor_ != nullptr) {
          this->frost_status_sensor_->publish_state(this->ot_->getBrinkTSP(FrostStatus));
          return;
        }
        break;
      case 9:
        if (this->fault_code_sensor_ != nullptr) {
          this->fault_code_sensor_->publish_state(this->ot_->getVentFaultCode());
          return;
        }
        break;
      case 10:
        if (this->imbalance_sensor_ != nullptr) {
          this->imbalance_sensor_->publish_state((int) this->ot_->getBrinkTSP(I1) - 100);
          return;
        }
        break;
#endif
#ifdef USE_NUMBER
      case 11:
        if (this->u4_number_ != nullptr) {
          this->u4_number_->publish_state(this->ot_->getBrinkTSP(U4) / 2.0f);
          return;
        }
        break;
      case 12:
        if (this->u5_number_ != nullptr) {
          this->u5_number_->publish_state(this->ot_->getBrinkTSP(U5) / 2.0f);
          return;
        }
        break;
      case 16:
        if (this->ventilation_number_ != nullptr) {
          const uint8_t cur = (uint8_t) this->ot_->getVentilation();
          // Yield-to-switch arbitration. HA is primary, but if someone moves the
          // physical 3-way switch the unit's level changes out from under us; we
          // honour that (mirror it to HA) instead of fighting it. HA reclaims
          // control on the next write_target().
          if (this->desired_ventilation_.has_value()) {
            const int diff = (int) cur - (int) this->desired_ventilation_.value();
            const bool in_sync = (diff >= -(int) this->ventilation_tolerance_ &&
                                  diff <= (int) this->ventilation_tolerance_);
            const bool external_change =
                this->last_seen_ventilation_.has_value() && cur != this->last_seen_ventilation_.value();
            if (in_sync) {
              // Holding the commanded level, nothing to do.
            } else if (external_change) {
              ESP_LOGD(TAG, "External ventilation change %u -> %u (wall switch?); yielding",
                       this->last_seen_ventilation_.value(), cur);
              this->desired_ventilation_ = cur;
            } else {
              ESP_LOGD(TAG, "Re-asserting ventilation %u (was %u)", this->desired_ventilation_.value(), cur);
              this->ot_->setVentilation(this->desired_ventilation_.value());
            }
          }
          this->last_seen_ventilation_ = cur;
          this->ventilation_number_->publish_state(cur);
          return;
        }
        break;
#endif
#ifdef USE_BINARY_SENSOR
      case 13:
        if (this->fault_binary_sensor_ != nullptr) {
          this->fault_binary_sensor_->publish_state(this->ot_->getFaultIndication());
          return;
        }
        break;
      case 14:
        if (this->filter_binary_sensor_ != nullptr) {
          this->filter_binary_sensor_->publish_state(this->ot_->getDiagnosticIndication());
          return;
        }
        break;
      case 15:
        if (this->ventilation_mode_binary_sensor_ != nullptr) {
          this->ventilation_mode_binary_sensor_->publish_state(this->ot_->getVentilationMode());
          return;
        }
        break;
#endif
      default:
        break;
    }
  }
}

void BrinkHru::write_target(BrinkTarget target, float value) {
  if (this->ot_ == nullptr)
    return;
  switch (target) {
    case TARGET_VENTILATION:
      ESP_LOGD(TAG, "Set ventilation -> %.0f %% (HA reclaims control)", value);
      // HA reclaims control: remember the target so the readback step keeps
      // re-asserting it until a wall-switch change overrides it.
      this->desired_ventilation_ = (uint8_t) value;
      this->ot_->setVentilation((unsigned int) value);
#ifdef USE_NUMBER
      if (this->ventilation_number_ != nullptr)
        this->ventilation_number_->publish_state(value);
#endif
      break;
    case TARGET_U4:
      ESP_LOGD(TAG, "Set U4 (min atmospheric bypass) -> %.0f C", value);
      this->ot_->setBrinkTSP(U4, (uint8_t) (value * 2));
#ifdef USE_NUMBER
      if (this->u4_number_ != nullptr)
        this->u4_number_->publish_state(value);
#endif
      break;
    case TARGET_U5:
      ESP_LOGD(TAG, "Set U5 (min indoor bypass) -> %.0f C", value);
      this->ot_->setBrinkTSP(U5, (uint8_t) (value * 2));
#ifdef USE_NUMBER
      if (this->u5_number_ != nullptr)
        this->u5_number_->publish_state(value);
#endif
      break;
  }
}

void BrinkHru::dump_config() {
  ESP_LOGCONFIG(TAG, "Brink Renovent HR:");
  ESP_LOGCONFIG(TAG, "  In pin: %u", this->in_pin_);
  ESP_LOGCONFIG(TAG, "  Out pin: %u", this->out_pin_);
  ESP_LOGCONFIG(TAG, "  Max volume: %.0f m3/h", this->max_volume_);
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace brink_hru
}  // namespace esphome
