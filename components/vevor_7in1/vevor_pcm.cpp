#include "vevor_pcm.h"

#include <cmath>

namespace vevor7in1 {
namespace {
// rtl_433 : min_count = 12 « bits » de préambule lorsque short_width == long_width.
constexpr size_t kMinPreambleRuns = 12;
constexpr float  kWidthTolerance  = 0.35f;  // ±35 % autour de la période de bit
}  // namespace

void PcmAssembler::reset() {
  runs_.clear();
  bit_us_ = kBitUsNominal;
}

// Estimation de la période de bit : on cherche la plus longue suite de runs
// consécutifs valant ~1 bit, puis on en prend la moyenne.
void PcmAssembler::refine_bit_period() {
  float  sum = 0.0f, best_sum = 0.0f;
  size_t count = 0, best = 0;

  for (int32_t raw : runs_) {
    const float r = static_cast<float>(raw);
    if (std::fabs(r - bit_us_) <= bit_us_ * kWidthTolerance) {
      sum += r;
      count++;
    } else {
      if (count > best) { best = count; best_sum = sum; }
      sum = 0.0f;
      count = 0;
    }
  }
  if (count > best) { best = count; best_sum = sum; }

  if (best >= kMinPreambleRuns)
    bit_us_ = best_sum / static_cast<float>(best);
}

size_t PcmAssembler::to_bits(bool start_high, bool high_is_one) {
  size_t n     = 0;
  bool   level = start_high;

  for (int32_t raw : runs_) {
    size_t m = static_cast<size_t>(std::lround(static_cast<float>(raw) / bit_us_));
    if (m < 1)
      m = 1;
    const uint8_t bit = (level == high_is_one) ? 1 : 0;
    for (size_t k = 0; k < m && n < kMaxBits; k++)
      bits_[n++] = bit;
    level = !level;
  }
  return n;
}

Status PcmAssembler::extract(size_t bit_count, Frame *out) const {
  for (size_t start = 0; start + kPreambleBits <= bit_count; start++) {
    bool match = true;
    for (size_t i = 0; i < kPreambleBits; i++) {
      const uint8_t want = (kPreamble[i / 8] >> (7 - (i % 8))) & 1;
      if (bits_[start + i] != want) { match = false; break; }
    }
    if (!match)
      continue;

    const size_t payload_start = start + kPreambleBits;
    if (payload_start + kPayloadLen * 8 > bit_count)
      return Status::kTooShort;

    uint8_t payload[kPayloadLen] = {0};
    for (size_t i = 0; i < kPayloadLen * 8; i++)
      payload[i / 8] = static_cast<uint8_t>((payload[i / 8] << 1) | bits_[payload_start + i]);

    return decode_payload(payload, out);
  }
  return Status::kNoSync;
}

Status PcmAssembler::finalize() {
  if (runs_.empty())
    return Status::kNoSync;

  refine_bit_period();

  Status best         = Status::kNoSync;
  bool   sync_seen    = false;

  for (int sh = 0; sh < 2 && !sync_seen; sh++) {
    for (int pol = 0; pol < 2 && !sync_seen; pol++) {
      const size_t n = to_bits(sh == 1, pol == 1);
      Frame        f;
      const Status s = extract(n, &f);
      if (s == Status::kOk) {
        frame_ = f;
        stats_.frames_ok++;
        stats_.sync_found++;
        runs_.clear();
        return Status::kOk;
      }
      if (s != Status::kNoSync) {
        sync_seen = true;
        best      = s;
      }
    }
  }

  if (sync_seen) {
    stats_.sync_found++;
    switch (best) {
      case Status::kBadChecksum: stats_.err_checksum++; break;
      case Status::kBadCounter:  stats_.err_counter++;  break;
      case Status::kBadHeader:   stats_.err_header++;   break;
      case Status::kTooShort:    stats_.err_too_short++; break;
      default: break;
    }
    runs_.clear();
  }
  return best;
}

Status PcmAssembler::push(const int32_t *runs, size_t count) {
  stats_.captures++;
  Status last = Status::kNoSync;

  if (bits_.size() < kMaxBits)
    bits_.resize(kMaxBits);

  for (size_t i = 0; i < count; i++) {
    const int32_t d = runs[i] < 0 ? -runs[i] : runs[i];
    if (d <= 0)
      continue;

    if (d > kIdleUs) {  // silence : frontière de trame
      const Status s = finalize();
      if (s != Status::kNoSync)
        last = s;
      continue;
    }

    runs_.push_back(d);
    if (runs_.size() > kMaxRuns)
      runs_.erase(runs_.begin(), runs_.begin() + (runs_.size() - kMaxRuns));
  }

  const Status s = finalize();
  if (s != Status::kNoSync)
    last = s;
  return last;
}

}  // namespace vevor7in1
