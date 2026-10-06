#include "vevor_7in1.h"

namespace esphome::vevor_7in1 {

static const char *const TAG = "vevor_7in1";

// Cadence nominale d'émission de la station : une trame toutes les 20 s
// (documenté dans rtl_433/src/devices/vevor_7in1.c). Sert de dénominateur au
// calcul de la qualité du signal.
static constexpr float kNominalPeriodS = 20.0f;

void Vevor7in1Component::setup() {
  // Rien à initialiser côté radio : le CC1101 est configuré par le composant
  // cc1101 du YAML et le lien avec le remote_receiver est matériel (GDO0).
  this->assembler_.reset();
}

void Vevor7in1Component::dump_config() {
  ESP_LOGCONFIG(TAG, "Vevor 7-in-1 :");
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("  ", "Humidite", this->humidity_sensor_);
  LOG_SENSOR("  ", "Vent moyen", this->wind_speed_sensor_);
  LOG_SENSOR("  ", "Rafale", this->wind_gust_sensor_);
  LOG_SENSOR("  ", "Direction", this->wind_direction_sensor_);
  LOG_SENSOR("  ", "Pluie", this->rain_total_sensor_);
  LOG_SENSOR("  ", "Indice UV", this->uv_index_sensor_);
  LOG_SENSOR("  ", "Luminosite", this->illuminance_sensor_);
  LOG_SENSOR("  ", "Age derniere trame", this->frame_age_sensor_);
  LOG_SENSOR("  ", "Qualite du signal", this->signal_quality_sensor_);
  LOG_SENSOR("  ", "Trames valides", this->frames_valid_sensor_);
  LOG_SENSOR("  ", "Trames rejetees", this->frames_invalid_sensor_);
  LOG_SENSOR("  ", "Periode de bit", this->bit_period_sensor_);
}

void Vevor7in1Component::publish_frame_(const ::vevor7in1::Frame &f) {
  if (this->temperature_sensor_ != nullptr)
    this->temperature_sensor_->publish_state(f.temperature_c);
  if (this->humidity_sensor_ != nullptr)
    this->humidity_sensor_->publish_state(f.humidity_pct);
  if (this->wind_speed_sensor_ != nullptr)
    this->wind_speed_sensor_->publish_state(f.wind_avg_kmh);
  if (this->wind_gust_sensor_ != nullptr)
    this->wind_gust_sensor_->publish_state(f.wind_gust_kmh);
  if (this->wind_direction_sensor_ != nullptr)
    this->wind_direction_sensor_->publish_state(f.wind_dir_deg);
  if (this->rain_total_sensor_ != nullptr)
    this->rain_total_sensor_->publish_state(f.rain_mm);
  if (this->uv_index_sensor_ != nullptr)
    this->uv_index_sensor_->publish_state(f.uv_index);
  if (this->illuminance_sensor_ != nullptr)
    this->illuminance_sensor_->publish_state(f.lux);
  if (this->battery_low_binary_sensor_ != nullptr)
    this->battery_low_binary_sensor_->publish_state(f.battery_low);
  if (this->station_id_text_sensor_ != nullptr) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%04X", f.id);
    this->station_id_text_sensor_->publish_state(buf);
  }
  if (this->last_status_text_sensor_ != nullptr)
    this->last_status_text_sensor_->publish_state(::vevor7in1::status_str(::vevor7in1::Status::kOk));
}

bool Vevor7in1Component::on_receive(remote_base::RemoteReceiveData data) {
  const remote_base::RawTimings &raw = data.get_raw_data();
  if (raw.empty())
    return false;

  const ::vevor7in1::Status status = this->assembler_.push(raw.data(), raw.size());

  if (status == ::vevor7in1::Status::kOk) {
    this->last_frame_ms_ = millis();
    this->has_frame_      = true;
    this->publish_frame_(this->assembler_.frame());
    return true;
  }

  // Préambule vu mais trame rejetée (somme de contrôle, compteur, en-tête) :
  // c'est l'information qui alimente la qualité du signal.
  if (status != ::vevor7in1::Status::kNoSync) {
    this->frames_invalid_++;
    if (this->last_status_text_sensor_ != nullptr)
      this->last_status_text_sensor_->publish_state(::vevor7in1::status_str(status));
  }
  return false;
}

void Vevor7in1Component::update() {
  const uint32_t now      = millis();
  const float    uptime_s = now / 1000.0f;

  // Watchdog : âge de la dernière trame valide. Aucune valeur publiée tant
  // qu'aucune trame n'a été décodée (on ne fabrique pas de données).
  if (this->frame_age_sensor_ != nullptr && this->has_frame_)
    this->frame_age_sensor_->publish_state((now - this->last_frame_ms_) / 1000.0f);

  // Qualité : trames valides rapportées aux trames attendues depuis le démarrage.
  if (this->signal_quality_sensor_ != nullptr && uptime_s > 0.0f) {
    const float expected = uptime_s / kNominalPeriodS;
    float       ratio    = 100.0f * this->assembler_.stats().frames_ok / expected;
    this->signal_quality_sensor_->publish_state(ratio > 100.0f ? 100.0f : ratio);
  }

  if (this->frames_valid_sensor_ != nullptr)
    this->frames_valid_sensor_->publish_state(this->assembler_.stats().frames_ok);
  if (this->frames_invalid_sensor_ != nullptr)
    this->frames_invalid_sensor_->publish_state(this->frames_invalid_);
  if (this->bit_period_sensor_ != nullptr)
    this->bit_period_sensor_->publish_state(this->assembler_.bit_us());
}

}  // namespace esphome::vevor_7in1
