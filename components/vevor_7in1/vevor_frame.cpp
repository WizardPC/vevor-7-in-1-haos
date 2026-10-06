#include "vevor_frame.h"

namespace vevor7in1 {

const char *status_str(Status status) {
  switch (status) {
    case Status::kOk:          return "ok";
    case Status::kNoSync:      return "no_sync";
    case Status::kTooShort:    return "too_short";
    case Status::kBadHeader:   return "bad_header";
    case Status::kBadChecksum: return "bad_checksum";
    case Status::kBadCounter:  return "bad_counter";
  }
  return "?";
}

uint8_t sum_u8(const uint8_t *data, size_t len) {
  uint16_t sum = 0;
  for (size_t i = 0; i < len; i++)
    sum += data[i];
  return static_cast<uint8_t>(sum & 0xFF);
}

Status decode_payload(const uint8_t payload[kPayloadLen], Frame *out) {
  uint8_t b[kPayloadLen];
  for (size_t i = 0; i < kPayloadLen; i++)
    b[i] = payload[i];

  // 1) Intégrité : équivalent de DECODE_FAIL_MIC côté rtl_433.
  if (sum_u8(b, 19) != b[19])
    return Status::kBadChecksum;

  // 2) Le second compteur suit le premier, rebouclage 0xFF -> 0x00 compris.
  if (b[20] != static_cast<uint8_t>(b[18] + 1))
    return Status::kBadCounter;

  // 3) En-tête : type de capteur 0 (b[0] = 0xAA) et canal 0 (b[1] = 0x00).
  if (b[0] != 0xAA || b[1] != 0x00)
    return Status::kBadHeader;

  // 4) Les valeurs multi-octets arrivent avec un décalage de 1 appliqué
  //    octet par octet (le rebouclage 0x00 -> 0xFF est volontaire).
  b[8]  -= 1;
  b[9]  -= 1;
  b[11] -= 1;
  b[12] -= 1;
  b[13] -= 1;
  b[14] -= 1;
  b[16] -= 1;
  b[17] -= 1;

  const uint16_t temp_raw = (static_cast<uint16_t>(b[5]) << 8) | b[6];
  const uint16_t wind_raw = (static_cast<uint16_t>(b[8]) << 8) | b[9];
  const uint16_t dir_raw  = ((b[11] & 0x0F) << 8) | b[12];
  const uint16_t rain_raw = (static_cast<uint16_t>(b[13]) << 8) | b[14];
  uint32_t       lux      = (static_cast<uint32_t>(b[16]) << 8) | b[17];
  if (lux & 0x8000u)                     // bit 15 = multiplicateur x10
    lux = (lux & 0x7FFFu) * 10u;

  Frame f;
  f.channel       = b[1] & 0x0F;
  f.id            = (static_cast<uint16_t>(b[2]) << 8) | b[3];
  f.battery_low   = ((b[4] & 0x80) >> 7) != 0;
  f.temperature_c = (static_cast<int>(temp_raw) - 500) * 0.1f;
  f.humidity_pct  = b[7];
  f.wind_avg_kmh  = wind_raw / 8.333f;
  f.wind_gust_kmh = b[10] / 1.25f;
  f.wind_dir_deg  = dir_raw;
  f.rain_mm       = rain_raw * 0.233f;
  f.uv_index      = static_cast<int>(b[15] & 0x1F) - 1;
  f.lux           = lux;
  f.tx_counter    = b[18];
  f.checksum      = b[19];
  for (size_t i = 0; i < kPayloadLen; i++)
    f.raw[i] = payload[i];

  *out = f;
  return Status::kOk;
}

}  // namespace vevor7in1
