#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include "vevor_frame.h"
#include "vevor_pcm.h"

namespace esphome::vevor_7in1 {

// Composant central : reçoit les timings bruts du remote_receiver (sortie GDO0
// du CC1101), les confie au décodeur puis publie les entités. Il porte aussi les
// deux diagnostics demandés : âge de la dernière trame valide (watchdog, cadencé
// à 1 s) et taux de trames conformes.
class Vevor7in1Component : public PollingComponent, public remote_base::RemoteReceiverListener {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;  // cadence fixée par le YAML (1 s) : watchdog + qualité

  // Appelé par le remote_receiver pour chaque lot de timings capturés.
  bool on_receive(remote_base::RemoteReceiveData data) override;

  // Données de la station.
  SUB_SENSOR(temperature)
  SUB_SENSOR(humidity)
  SUB_SENSOR(wind_speed)
  SUB_SENSOR(wind_gust)
  SUB_SENSOR(wind_direction)
  SUB_SENSOR(rain_total)
  SUB_SENSOR(uv_index)
  SUB_SENSOR(illuminance)
  SUB_BINARY_SENSOR(battery_low)
  SUB_TEXT_SENSOR(station_id)

  // Diagnostic.
  SUB_SENSOR(frame_age)        // secondes depuis la dernière trame valide
  SUB_SENSOR(signal_quality)   // % de trames conformes / trames attendues
  SUB_SENSOR(frames_valid)
  SUB_SENSOR(frames_invalid)
  SUB_SENSOR(bit_period)       // période de bit mesurée, µs (mise au point)
  SUB_SENSOR(captures)         // lots d'impulsions livrés par le récepteur
  SUB_SENSOR(last_pulses)      // impulsions du dernier lot
  SUB_SENSOR(last_max_us)      // impulsion la plus longue du dernier lot, µs
  SUB_TEXT_SENSOR(last_status)

 protected:
  void publish_frame_(const ::vevor7in1::Frame &frame);

  ::vevor7in1::PcmAssembler assembler_{};
  uint32_t                  frames_invalid_{0};
  uint32_t                  last_frame_ms_{0};
  bool                      has_frame_{false};
};

}  // namespace esphome::vevor_7in1
