#pragma once
// ---------------------------------------------------------------------------
// Vevor 7-in-1 (YT60231, 868 MHz) : décodage de la charge utile.
//
// Portage C++ du décodeur rtl_433 `src/devices/vevor_7in1.c`
// (auteur amont : Bruno OCTAU / ProfBoc75) — licence GPL-2.0. La redistribution
// de ce fichier impose de conserver la GPL-2.0 du projet amont.
//
// Ce fichier ne dépend PAS d'ESPHome : il se compile et se teste sur PC
// (voir tests/test_decoder.cpp).
// ---------------------------------------------------------------------------

#include <cstddef>
#include <cstdint>

namespace vevor7in1 {

// Mot de synchronisation recherché dans le flux de bits : 5 octets = 40 bits.
constexpr uint8_t kPreamble[]   = {0xAA, 0xAA, 0xCA, 0xCA, 0x54};
constexpr size_t  kPreambleLen  = 5;
constexpr size_t  kPreambleBits = kPreambleLen * 8;

// La trame dure 33 octets (264 bits) ; seuls les 21 premiers sont exploités.
constexpr size_t kPayloadLen  = 21;
constexpr size_t kMessageBits = 264;

// 2-FSK, ~11.26 kbaud => 88.8 us par bit.
// rtl_433 déclare short_width = long_width = 87 us : c'est la même échelle.
// Amorce seulement : l'assembleur affine la mesure sur le préambule.
constexpr float kBitUsNominal = 88.8f;

enum class Status : uint8_t {
  kOk = 0,
  kNoSync,       // préambule absent du flux
  kTooShort,     // préambule trouvé mais moins de 21 octets derrière
  kBadHeader,    // b[0] != 0xAA ou b[1] != 0x00
  kBadChecksum,  // somme(b[0..18]) & 0xff != b[19]
  kBadCounter,   // b[20] != b[18] + 1 (compteur de TX hors séquence)
};

const char *status_str(Status status);

// Grandeurs publiées, dans l'ordre du décodeur rtl_433.
struct Frame {
  uint8_t  channel{0};
  uint16_t id{0};
  bool     battery_low{false};
  float    temperature_c{0.0f};
  uint8_t  humidity_pct{0};
  float    wind_avg_kmh{0.0f};
  float    wind_gust_kmh{0.0f};
  uint16_t wind_dir_deg{0};
  float    rain_mm{0.0f};      // cumul depuis la mise sous tension de la station
  int      uv_index{0};
  uint32_t lux{0};
  uint8_t  tx_counter{0};      // b[18] : incrémenté à chaque émission
  uint8_t  checksum{0};
  uint8_t  raw[kPayloadLen]{}; // charge utile telle que reçue (offsets non retirés)
};

// Somme modulo 256 (rtl_433 : add_bytes()).
uint8_t sum_u8(const uint8_t *data, size_t len);

// Décode une charge utile de 21 octets, préambule déjà retiré.
Status decode_payload(const uint8_t payload[kPayloadLen], Frame *out);

}  // namespace vevor7in1
