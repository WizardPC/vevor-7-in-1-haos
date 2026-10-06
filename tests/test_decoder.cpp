// ---------------------------------------------------------------------------
// Tests hôte du décodeur Vevor 7-in-1 — aucun matériel requis.
//
// La trame de référence est un exemple RÉEL documenté dans l'en-tête de
// rtl_433/src/devices/vevor_7in1.c. Sa somme de contrôle est recalculée par le
// test (0xE0) : ce n'est pas une valeur inventée.
//
// Compilation : tests/run_tests.sh
// ---------------------------------------------------------------------------

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "../components/vevor_7in1/vevor_frame.h"
#include "../components/vevor_7in1/vevor_pcm.h"

using namespace vevor7in1;

static int g_failures = 0;
static int g_checks   = 0;

static void check(bool cond, const char *label) {
  g_checks++;
  if (!cond) {
    g_failures++;
    std::printf("  \xE2\x9C\x97 %s\n", label);
  } else {
    std::printf("  \xE2\x9C\x93 %s\n", label);
  }
}

static void check_near(float got, float want, float tol, const char *label) {
  g_checks++;
  if (std::fabs(got - want) > tol) {
    g_failures++;
    std::printf("  \xE2\x9C\x97 %s : obtenu %.4f, attendu %.4f\n", label, got, want);
  } else {
    std::printf("  \xE2\x9C\x93 %s (%.4f)\n", label, got);
  }
}

// Trame de référence (21 octets), extraite du commentaire d'en-tête de vevor_7in1.c.
static const uint8_t kRef[kPayloadLen] = {0xaa, 0x00, 0xf8, 0xf7, 0x9d, 0x02, 0xe3,
                                          0x32, 0x01, 0x0e, 0x03, 0x02, 0x0b, 0x01,
                                          0x38, 0x02, 0x39, 0x7a, 0x86, 0xe0, 0x87};

// Fabrique la suite de durées (µs) telle qu'un récepteur la verrait :
// préambule (40 bits) + trame (33 octets, 12 derniers à zéro), en NRZ.
// `edge_noise_us` modélise la gigue d'horodatage des fronts (±N µs), pas une
// erreur proportionnelle : c'est ce que produit un vrai récepteur.
static std::vector<int32_t> build_runs(float bit_us, bool invert, float edge_noise_us) {
  std::vector<uint8_t> bits;
  for (size_t i = 0; i < kPreambleBits; i++)
    bits.push_back((kPreamble[i / 8] >> (7 - (i % 8))) & 1);

  uint8_t msg[33] = {0};
  std::memcpy(msg, kRef, kPayloadLen);
  for (size_t i = 0; i < kMessageBits; i++)
    bits.push_back((msg[i / 8] >> (7 - (i % 8))) & 1);

  if (invert)
    for (uint8_t &b : bits)
      b ^= 1;

  std::vector<int32_t> runs;
  uint32_t             rng = 12345u;
  size_t               i   = 0;
  while (i < bits.size()) {
    size_t j = i;
    while (j < bits.size() && bits[j] == bits[i])
      j++;
    float w = static_cast<float>(j - i) * bit_us;
    if (edge_noise_us > 0.0f) {
      rng = rng * 1103515245u + 12345u;
      const float f = ((static_cast<float>((rng >> 16) % 2001u) / 1000.0f) - 1.0f) * edge_noise_us;
      w += f;
    }
    int32_t d = static_cast<int32_t>(std::lround(w));
    runs.push_back(d < 1 ? 1 : d);
    i = j;
  }
  return runs;
}

static void expect_reference_frame(const Frame &f, const char *label) {
  std::printf("[%s]\n", label);
  check(f.channel == 0, "canal = 0");
  check(f.id == 0xf8f7, "id = 0xf8f7");
  check(f.battery_low, "batterie faible (0x9d)");
  check_near(f.temperature_c, 23.9f, 0.05f, "temperature_C");
  check(f.humidity_pct == 50, "humidite = 50 %");
  check_near(f.wind_avg_kmh, 13.0f / 8.333f, 0.01f, "vent moyen km/h");
  // b[10] (rafale) ne subit PAS l'offset -1 dans rtl_433 : 0x03 / 1.25 = 2.4
  check_near(f.wind_gust_kmh, 3.0f / 1.25f, 0.01f, "rafale km/h");
  check(f.wind_dir_deg == 266, "direction = 266 deg");
  check_near(f.rain_mm, 55.0f * 0.233f, 0.01f, "pluie mm");
  check(f.uv_index == 1, "UV = 1");
  check(f.lux == 14457, "luminosite = 14457 lx");
  check(f.tx_counter == 0x86, "compteur TX = 0x86");
}

int main() {
  std::printf("=== 1. Somme de controle de la trame de reference ===\n");
  check(sum_u8(kRef, 19) == kRef[19], "somme(b[0..18]) & 0xff == b[19] (0xE0)");

  std::printf("\n=== 2. decode_payload() sur la trame de reference ===\n");
  {
    Frame  f;
    Status s = decode_payload(kRef, &f);
    check(s == Status::kOk, "statut = ok");
    expect_reference_frame(f, "champs");
  }

  std::printf("\n=== 3. Chaine complete : impulsions -> bits -> trame ===\n");
  {
    PcmAssembler asm_;
    auto         runs = build_runs(kBitUsNominal, false, 0.0f);
    Status       s    = asm_.push(runs.data(), runs.size());
    check(s == Status::kOk, "statut = ok");
    check(asm_.stats().frames_ok == 1, "1 trame valide comptee");
    check_near(asm_.bit_us(), kBitUsNominal, 1.0f, "periode de bit mesuree");
    expect_reference_frame(asm_.frame(), "champs");
  }

  std::printf("\n=== 4. Polarite inversee (mark = 0) ===\n");
  {
    PcmAssembler asm_;
    auto         runs = build_runs(kBitUsNominal, true, 0.0f);
    check(asm_.push(runs.data(), runs.size()) == Status::kOk, "statut = ok malgre l'inversion");
  }

  std::printf("\n=== 5. Horloge emettrice decalee (80 us au lieu de 88.8) + bruit de front +/-2 us ===\n");
  {
    PcmAssembler asm_;
    auto         runs = build_runs(80.0f, false, 2.0f);
    Status       s    = asm_.push(runs.data(), runs.size());
    if (s != Status::kOk) {
      std::printf("    diagnostic: statut=%s, runs=%zu, sync=%u, csum=%u, ctr=%u, hdr=%u, court=%u\n",
                  status_str(s), runs.size(), asm_.stats().sync_found, asm_.stats().err_checksum,
                  asm_.stats().err_counter, asm_.stats().err_header, asm_.stats().err_too_short);
    }
    check(s == Status::kOk, "statut = ok");
    check_near(asm_.bit_us(), 80.0f, 4.0f, "periode de bit re-estimee");
    check(asm_.frame().id == 0xf8f7, "meme trame decodee");
    check(asm_.frame().temperature_c > 23.8f && asm_.frame().temperature_c < 24.0f, "temperature intacte");
  }

  std::printf("\n=== 6. Silence > 9 ms en tete de capture ===\n");
  {
    PcmAssembler asm_;
    auto         runs = build_runs(kBitUsNominal, false, 0.0f);
    runs.insert(runs.begin(), 12000);  // au-dela du reset_limit rtl_433
    check(asm_.push(runs.data(), runs.size()) == Status::kOk, "statut = ok");
  }

  std::printf("\n=== 7. Trammes corrompues rejetees ===\n");
  {
    uint8_t bad[kPayloadLen];
    std::memcpy(bad, kRef, kPayloadLen);
    bad[7] ^= 0x01;  // humidite alteree -> somme invalide
    Frame f;
    check(decode_payload(bad, &f) == Status::kBadChecksum, "somme fausse -> kBadChecksum");

    std::memcpy(bad, kRef, kPayloadLen);
    bad[0] = 0xAB;  // en-tete invalide, somme recalculee pour isoler le test
    bad[19] = sum_u8(bad, 19);
    check(decode_payload(bad, &f) == Status::kBadHeader, "en-tete invalide -> kBadHeader");

    std::memcpy(bad, kRef, kPayloadLen);
    bad[20] = 0x11;  // compteur desynchronise
    check(decode_payload(bad, &f) == Status::kBadCounter, "compteur incoherent -> kBadCounter");
  }

  std::printf("\n=== 8. Bruit pur : aucune fausse trame ===\n");
  {
    PcmAssembler        asm_;
    std::vector<int32_t> noise;
    uint32_t             rng = 7u;
    for (int i = 0; i < 400; i++) {
      rng = rng * 1103515245u + 12345u;
      noise.push_back(static_cast<int32_t>(20 + (rng >> 16) % 400));
    }
    check(asm_.push(noise.data(), noise.size()) != Status::kOk, "aucune trame valide sur du bruit");
    check(asm_.stats().frames_ok == 0, "compteur de trames valides inchange");
  }

  std::printf("\n---------------------------------------------\n");
  std::printf("%d verification(s), %d echec(s)\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
