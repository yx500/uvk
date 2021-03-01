#pragma once

#include <numeric>
#include <memory>
#include "Crc32.h"


#pragma pack(1)
struct cmd_DO
{
  uint8_t   magic[2];
  uint16_t  flag;     //0- установка/снятие бита, 1-выставить паттрн байта ...  и тд
  uint16_t  number;   //номер по порядку бита или байта, в зависимости от флага
  uint8_t   data[4];  //on_off or pattern;
  uint32_t  id;
  uint32_t  crc;
};
#pragma pack()


class message_DO
    : public cmd_DO
{
public:
  message_DO() : cmd_DO() {}
  message_DO(void* p) : cmd_DO( *reinterpret_cast<cmd_DO*>(p) ) {}

  const uint8_t* begin() const { return this->magic;  }
  uint8_t* begin() {  return this->magic; }
  const uint8_t* end() const {  return this->magic + sizeof(cmd_DO); }
  uint8_t* end() {  return this->magic + sizeof(cmd_DO); }
  size_t size() const {  return sizeof(cmd_DO); }


  using Crc = Vx::Crc32<0x4C11DB7>;

  bool is_valid() const
  {
     return
        magic[0]=='D'
        && magic[1]=='O'
        && crc == Crc::calculate( magic, sizeof(cmd_DO) - sizeof(cmd_DO::crc) );
  }

  void commit() {
    magic[0]='D';
    magic[1]='O';
    crc = Crc::calculate( magic, sizeof(cmd_DO) - sizeof(cmd_DO::crc) );
  }

};

