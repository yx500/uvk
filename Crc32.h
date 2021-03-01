#pragma once

#include <stdint.h>

namespace Vx {

template<uint32_t Polynomial = 0xEDB88320>
class Crc32 {
  class Table {
    uint32_t table[256]; //
  public:
    Table() {
      uint32_t polynomial = Polynomial;
      for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (size_t j = 0; j < 8; j++) {
          if (c & 1) {
            c = polynomial ^ (c >> 1);
          } else {
            c >>= 1;
          }
        }
        this->table[i] = c;
      }
    }

    uint32_t operator[](size_t index) const {
      return this->table[index];
    }
  };

  static Table table; //
private:
  uint32_t crc32; //
public:
  constexpr Crc32(const void *data, size_t length, uint32_t crc32 = 0) :
      crc32(calculate(data, length, crc32)) {
  }

  constexpr operator uint32_t() const {
    return this->crc32;
  }

  static constexpr uint32_t calculate(const void *data, size_t length, uint32_t crc32 = 0) {
    crc32 ^= 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
      crc32 = table[(crc32 ^ ((const uint8_t*) data)[i]) & 0xFF] ^ (crc32 >> 8);
    }
    return crc32 ^ 0xFFFFFFFF;
  }
};

template<uint32_t Polynomial>
typename Crc32<Polynomial>::Table Crc32<Polynomial>::table;

}
