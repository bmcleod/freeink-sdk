#include "Wudada370Driver.h"

#include <BoardConfig.h>

#include "../lut/Wudada370Luts.h"

namespace freeink {

uint32_t Wudada370Driver::spiHz() const { return BoardConfig::ACTIVE.displaySpiHz; }

void Wudada370Driver::initialize(EpdBus& bus) {
  // OEM 0x4207935c. Use the recovered voltages/booster settings, not Murphy's.
  bus.reset(10);
  bus.waitBusy("WUDADA reset");
  bus.cmd(0x00);
  bus.data(0xFF);
  bus.data(0x01);
  bus.cmd(0x01);
  bus.data(0x03);
  bus.data(0x10);
  bus.data(0x3F);
  bus.data(0x3F);
  bus.data(0x03);
  bus.cmd(0x06);
  bus.data(0x17);
  bus.data(0x37);
  bus.data(0x3D);
  bus.cmd(0x30);
  bus.data(0x09);
  bus.cmd(0x82);
  bus.data(0x07);
  bus.cmd(0x61);
  bus.data(0xF0);
  bus.data(0x01);
  bus.data(0xA0);
  bus.cmd(0x50);
  bus.data(0xD7);
}

void Wudada370Driver::loadLut(EpdBus& bus) {
  bus.cmdData(0x20, WUDADA_LUT_20, sizeof(WUDADA_LUT_20));
  bus.cmdData(0x21, WUDADA_LUT_21, sizeof(WUDADA_LUT_21));
  bus.cmdData(0x22, WUDADA_LUT_22, sizeof(WUDADA_LUT_22));
  bus.cmdData(0x23, WUDADA_LUT_23, sizeof(WUDADA_LUT_23));
  bus.cmdData(0x24, WUDADA_LUT_24, sizeof(WUDADA_LUT_24));
}

#if WUDADA_ENABLE_FAST_REFRESH
void Wudada370Driver::loadFastLut(EpdBus& bus) {
  bus.cmdData(0x20, WUDADA_LUT_FAST_20, sizeof(WUDADA_LUT_FAST_20));
  bus.cmdData(0x21, WUDADA_LUT_FAST_21, sizeof(WUDADA_LUT_FAST_21));
  bus.cmdData(0x22, WUDADA_LUT_FAST_22, sizeof(WUDADA_LUT_FAST_22));
  bus.cmdData(0x23, WUDADA_LUT_FAST_23, sizeof(WUDADA_LUT_FAST_23));
  bus.cmdData(0x24, WUDADA_LUT_FAST_24, sizeof(WUDADA_LUT_FAST_24));
}
#endif

void Wudada370Driver::begin(EpdBus& bus) {
  initialize(bus);
  loadLut(bus);
}

void Wudada370Driver::writePlane(EpdBus& bus, uint8_t command, const uint8_t* fb) {
  // CrossPoint's landscape facade -> controller-native 240x416 RAM.
  // Rotation direction still needs verification on the owner's unit.
  bus.cmd(command);
  bus.beginTxn();
  uint8_t row[30];
  for (uint16_t y = 0; y < 416; ++y) {
    for (auto& byte : row) byte = 0;
    for (uint16_t x = 0; x < 240; ++x) {
      const uint8_t bit = (fb[(239 - x) * 52 + (y >> 3)] >> (7 - (y & 7))) & 1;
      row[x >> 3] |= bit << (7 - (x & 7));
    }
    bus.rawWriteBytes(row, sizeof(row));
  }
  bus.endTxn();
}

void Wudada370Driver::triggerRefresh(EpdBus& bus, bool turnOff, bool /*fast*/) {
  // OEM 0x420794a4: WUDADA triggers refresh with 17/A5, not 12.
  bus.cmd(0x17);
  bus.data(0xA5);
  delay(10);
  bus.waitBusy("WUDADA full refresh");
  if (turnOff) deepSleep(bus);
}

void Wudada370Driver::display(EpdBus& bus, const uint8_t* fb, const uint8_t* /*prev*/, RefreshMode /*mode*/,
                              bool turnOff) {
  // Fast DU does not paint on this panel (WW/BB only). Every Full/Half/Fast
  // request uses the proven GC path: reset+init, mode-1 LUTs 20..24, NEW frame
  // on both 0x10 and 0x13, window, 17/A5. No skip-reset, no prev/new differential.
  initialize(bus);
  loadLut(bus);

  bus.cmd(0x91);
  const uint8_t window[] = {0, 239, 0, 0, 1, 159, 1};
  bus.cmdData(0x90, window, sizeof(window));
  writePlane(bus, 0x10, fb);
  writePlane(bus, 0x13, fb);
  bus.cmd(0x92);
  triggerRefresh(bus, turnOff, false);
}

void Wudada370Driver::deepSleep(EpdBus& bus) {
  // OEM 0x42079198.
  bus.cmd(0x07);
  bus.data(0xA5);
}

PanelDriver& wudada370Driver() {
  static Wudada370Driver driver;
  return driver;
}
}  // namespace freeink
