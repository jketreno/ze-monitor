
#include "engine.h"
#include "helpers.h"

bool Engine::initializeEngine() {
  ze_result_t ret = zesEngineGetProperties(engine, &properties);
  if (ret != ZE_RESULT_SUCCESS)
    return false;
  ret = updateStats();
  return ret == ZE_RESULT_SUCCESS;
}

ze_result_t Engine::updateStats() {
  uint64_t lastActivetime = stats.activeTime;
  uint64_t lastTimestamp = stats.timestamp;
  ze_result_t ret;

  ret = zesEngineGetActivity(engine, &stats);
  if (ret != ZE_RESULT_SUCCESS)
    return ret;

  // If we have no previous sample (lastTimestamp == 0) treat this as the
  // initial reading and don't compute/validate utilization. Also perform
  // the computation in floating point to avoid integer truncation.
  if (lastTimestamp == 0) {
    utilization = 0;
  } else if (stats.timestamp > lastTimestamp) {
    utilization = 100.0 *
                  static_cast<double>(stats.activeTime - lastActivetime) /
                  static_cast<double>(stats.timestamp - lastTimestamp);

    // Only flag if we actually computed a utilization and it's outside
    // the valid range. This avoids spurious messages on the first sample.
    if (utilization > 100.0) {
      fprintf(stderr, "stats error: %lu %lu %lu %lu\n", stats.activeTime,
              lastActivetime, stats.timestamp, lastTimestamp);
    }
  } else {
    utilization = 0;
  }

  return ZE_RESULT_SUCCESS;
}
