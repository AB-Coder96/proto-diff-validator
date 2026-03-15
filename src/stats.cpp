#include "zetapacket/stats.h"

#include <cmath>
#include <sstream>

namespace zetapacket {

static inline uint32_t log2_bucket(uint64_t v) {
  if (v == 0) {
    return 0;
  }
  return 63u - static_cast<uint32_t>(__builtin_clzll(v));
}

Log2Histogram::Log2Histogram(uint32_t max_bucket)
    : max_bucket_(max_bucket), buckets_(max_bucket + 1, 0) {}

void Log2Histogram::add(uint64_t value_ns) {
  uint32_t bucket = log2_bucket(value_ns);
  if (bucket > max_bucket_) {
    bucket = max_bucket_;
  }
  buckets_[bucket] += 1;
  count_ += 1;
}

uint64_t Log2Histogram::quantile(double q) const {
  if (count_ == 0) {
    return 0;
  }
  if (q <= 0.0) {
    q = 0.0;
  }
  if (q >= 1.0) {
    q = 1.0;
  }

  uint64_t target = static_cast<uint64_t>(std::ceil(q * static_cast<double>(count_)));
  if (target == 0) {
    target = 1;
  }

  uint64_t running = 0;
  for (uint32_t i = 0; i <= max_bucket_; ++i) {
    running += buckets_[i];
    if (running >= target) {
      if (i >= 63) {
        return UINT64_MAX;
      }
      return 1ull << (i + 1);
    }
  }
  return 1ull << (max_bucket_ + 1);
}

std::string Log2Histogram::debug_summary() const {
  std::ostringstream oss;
  oss << "count=" << count_;
  return oss.str();
}

double RunStats::seconds() const {
  if (end_ns <= start_ns) {
    return 0.0;
  }
  return double(end_ns - start_ns) / 1e9;
}

double RunStats::measured_pps() const {
  const double s = seconds();
  if (s <= 0.0) {
    return 0.0;
  }
  return double(rx_pkts) / s;
}

}  // namespace zetapacket
