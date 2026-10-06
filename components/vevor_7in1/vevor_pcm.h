#pragma once
// ---------------------------------------------------------------------------
// Réassemblage NRZ/PCM : convertit une suite de durées d'impulsions (µs) en un
// flux de bits, y cherche le préambule Vevor puis en extrait la charge utile.
//
// Portage de `pulse_slicer_pcm()` (rtl_433, src/pulse_slicer.c, l. 216-231)
// pour le cas NRZ où short_width == long_width, qui est celui du Vevor
// (short = long = 87 µs) :
//   - un mark  (niveau haut) de durée d  => round(d / T) bits à 1
//   - un space (niveau bas)  de durée d  => round(d / T) bits à 0
// T (période d'un bit) est affinée sur le préambule, comme le fait rtl_433 dès
// 12 bits de préambule en NRZ — nécessaire ici car le CC1101 et l'émetteur
// n'ont pas exactement la même horloge.
//
// La polarité (niveau haut = 1 ou 0) et le niveau du premier run ne sont pas
// connus au démarrage : les 4 combinaisons sont essayées à chaque trame.
// ---------------------------------------------------------------------------

#include <cstddef>
#include <cstdint>
#include <vector>

#include "vevor_frame.h"

namespace vevor7in1 {

class PcmAssembler {
 public:
  struct Stats {
    uint32_t captures{0};     // lots d'impulsions reçus
    uint32_t sync_found{0};   // préambules détectés (trames ou erreurs)
    uint32_t frames_ok{0};    // trames décodées et validées
    uint32_t err_checksum{0};
    uint32_t err_counter{0};
    uint32_t err_header{0};
    uint32_t err_too_short{0};
    // Sonde du front RF : ce que le récepteur livre réellement.
    uint32_t last_runs{0};    // impulsions du dernier lot reçu
    int32_t  last_max_us{0};  // impulsion la plus longue du dernier lot
    uint64_t total_runs{0};   // impulsions cumulées depuis le démarrage
  };

  // Ajoute un lot de durées en µs. Les signes sont ignorés : seule l'alternance
  // mark/space compte (les composants ESPHome fournissent des valeurs signées).
  // Renvoie kOk si une trame a été validée pendant cet appel.
  Status push(const int32_t *runs, size_t count);

  void reset();

  const Frame &frame() const { return frame_; }
  const Stats &stats() const { return stats_; }
  float       bit_us() const { return bit_us_; }

 private:
  static constexpr int32_t kIdleUs = 9000;   // reset_limit rtl_433
  static constexpr size_t  kMaxRuns = 2048;  // borne mémoire (une trame << 300 runs)
  static constexpr size_t  kMaxBits = 8192;  // garde-fou sur le flux binaire

  void   refine_bit_period();
  size_t to_bits(bool start_high, bool high_is_one);
  Status extract(size_t bit_count, Frame *out) const;
  Status finalize();

  std::vector<int32_t> runs_;
  std::vector<uint8_t> bits_;
  Frame                frame_{};
  Stats                stats_{};
  float                bit_us_{kBitUsNominal};
};

}  // namespace vevor7in1
