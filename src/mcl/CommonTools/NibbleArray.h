#pragma once

template<size_t N>
class NibbleArray {
public:
	///  storage layout
	///  B0          B1          B2 ...
	///  b7  ...  b0 b7  ...  b0
	///  [nib1 nib0] [nib3 nib2] ...
  uint8_t data[N/2];

	uint8_t get(uint8_t index) {
		uint8_t e = data[index >> 1];
		if (index & 1) {
			e >>= 4;
		}
		return e & 0x0F;
	}

	void set(uint8_t index, uint8_t value) {
		uint8_t i = index >> 1;
		uint8_t e = data[i];
		uint8_t mask = 0xF0;
		if (index & 1) {
			mask = 0xF;
			value <<= 4;
		} 	
		data[i] = (e & mask) | value;
	}

	void clear() {
		memset(data, 0, N / 2);
	}

private:
	__attribute__((noinline)) void shift(uint8_t len, int8_t delta) {
		if (len == 0) return;
		uint8_t i = delta > 0 ? len - 1 : 0;
		uint8_t saved = get(i);
		for(uint8_t count = len - 1; count > 0; --count) {
			uint8_t next = i - delta;
			set(i, get(next));
			i = next;
		}
		set(i, saved);
	}

public:
	void shift_left(uint8_t len) {
		shift(len, -1);
	}

	void shift_right(uint8_t len) {
		shift(len, 1);
	}

	void reverse(uint8_t len) {
		if (len == 0) return;
		for(uint8_t i = 0, j = len - 1; i < j; ++i, --j) {
			uint8_t tmp = get(i);
			set(i, get(j));
			set(j, tmp);
		}
	}
};
