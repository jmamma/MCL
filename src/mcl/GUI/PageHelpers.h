#pragma once

#include <stdint.h>

namespace gui {

constexpr uint8_t kNoSelection = 0xFF;

struct ListIteratorConfig {
  uint8_t start_index;
  uint8_t total_items;
  uint8_t visible_rows;
  uint8_t highlight_row;
};

inline uint8_t clamp_visible_row(uint8_t cur_row, int8_t diff,
                                 uint8_t visible_rows) {
  if (visible_rows == 0) {
    return 0;
  }
  int16_t next = static_cast<int16_t>(cur_row) + diff;
  int16_t max_row = static_cast<int16_t>(visible_rows) - 1;
  if (next < 0) {
    next = 0;
  } else if (next > max_row) {
    next = max_row;
  }
  return static_cast<uint8_t>(next);
}

template <typename MenuT, typename EncoderT>
inline void sync_menu_value_encoder(MenuT *menu, EncoderT &value_encoder,
                                    uint8_t item_index) {
  const uint8_t range = menu->get_option_range(item_index);
  value_encoder.max = range > 0 ? range - 1 : 0;
  value_encoder.min = menu->get_option_min(item_index);
  if (auto *dest = menu->get_dest_variable(item_index)) {
    value_encoder.setValue(*dest);
  } else {
    value_encoder.setValue(value_encoder.min);
  }
}

template <typename Callback>
inline void for_each_visible_row(const ListIteratorConfig &cfg,
                                 Callback &&callback) {
  if (cfg.visible_rows == 0 || cfg.start_index >= cfg.total_items) {
    return;
  }
  uint8_t rows = cfg.visible_rows;
  const uint8_t remaining = cfg.total_items - cfg.start_index;
  if (rows > remaining) {
    rows = remaining;
  }
  for (uint8_t row = 0; row < rows; ++row) {
    const bool selected =
        (cfg.highlight_row != kNoSelection && row == cfg.highlight_row);
    callback(cfg.start_index + row, row, selected);
  }
}

} // namespace gui
