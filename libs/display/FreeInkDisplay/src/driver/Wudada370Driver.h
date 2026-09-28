#pragma once
#include "PanelDriver.h"

#ifndef WUDADA_ENABLE_FAST_REFRESH
#define WUDADA_ENABLE_FAST_REFRESH 0
#endif

namespace freeink {
// UC81xx command-family panel recovered from WUDADA stock firmware.
// Exact controller part number is not yet verified on the physical panel.
class Wudada370Driver : public PanelDriver {
 public:
  uint32_t spiHz() const override;
  BusyPolarity busyPolarity() const override { return BusyPolarity::ActiveLow; }
  PanelGeometry geometry() const override { return {416, 240, 52, 12480}; }
  void begin(EpdBus& bus) override;
  void deepSleep(EpdBus& bus) override;
  void display(EpdBus& bus, const uint8_t* fb, const uint8_t* prev, RefreshMode mode, bool turnOff) override;

 private:
  void initialize(EpdBus& bus);
  void loadLut(EpdBus& bus);
#if WUDADA_ENABLE_FAST_REFRESH
  void loadFastLut(EpdBus& bus);
#endif
  void writePlane(EpdBus& bus, uint8_t command, const uint8_t* fb);
  void triggerRefresh(EpdBus& bus, bool turnOff, bool fast = false);
};
PanelDriver& wudada370Driver();
}  // namespace freeink
