#ifndef GRID_CHAIN_H__
#define GRID_CHAIN_H__

class GridChain {

public:
  uint8_t row;
  uint8_t loops;

  GridChain(uint8_t active_ = 0, uint8_t row_ = 0, uint8_t col_ = 0,
            uint8_t loops_ = 0) {}

};

#endif /* GRID_CHAIN_H__ */
