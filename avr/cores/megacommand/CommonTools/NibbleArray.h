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

	void shift_left(uint8_t len) {
		if (len == 0) return;
		NibbleArray<N> that = *this;
		--len;
		for(uint8_t i = 0; i < len; ++i) {
			set(i, that.get(i+1));
		}
		set(len, that.get(0));
	}

	void shift_right(uint8_t len) {
		if (len == 0) return;
		NibbleArray<N> that = *this;
		for(uint8_t i = 1; i < len; ++i) {
			set(i, that.get(i-1));
		}
		set(0, that.get(len-1));
	}

	void reverse(uint8_t len) {
		if (len == 0) return;
		NibbleArray<N> that = *this;
		for(uint8_t i = 0; i < len; ++i) {
			set(i, that.get(len-1-i));
		}
	}
};

