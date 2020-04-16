#pragma once

#include <numeric>
#include <memory>
//#include <boost/shared_ptr.hpp>
//#include <boost/enable_shared_from_this.hpp>
#include "Crc32.h"


#pragma pack(push, 1)
struct tu_cmd
{
  uint8_t   magic[2] { 'T', 'U'};
  uint16_t  number;   //номер бита
  uint8_t   on_off;   //взвести или сбросить
  uint32_t  id;       //айди команды, или счетчик
  uint8_t   flag;     //резерв/приоритет
  uint32_t  crc;
};
#pragma pack(pop)

using Crc = Vx::Crc32<0x4C11DB7>;

class do_message
    : public tu_cmd
//    , public boost::enable_shared_from_this<do_message>
{
public:
  do_message() : tu_cmd() {}
  do_message(void* p) : tu_cmd( *reinterpret_cast<tu_cmd*>(p) ) {}

  const uint8_t* begin() const { return this->magic;  }
  uint8_t* begin() {  return this->magic; }
  const uint8_t* end() const {  return this->magic + sizeof(tu_cmd); }
  uint8_t* end() {  return this->magic + sizeof(tu_cmd); }
  size_t size() const {  return sizeof(tu_cmd); }


  bool is_valid() const
  {
     return
        magic[0]=='T'
        && magic[1]=='U'
        && crc == Crc::calculate( magic, sizeof(tu_cmd) - sizeof(tu_cmd::crc) );
  }

  void commit() {
    magic[0]='T';
    magic[1]='U';
    crc = Crc::calculate( magic, sizeof(tu_cmd) - sizeof(tu_cmd::crc) );
  }

//  typedef std::shared_ptr<do_message> do_message_ptr;
//  static do_message_ptr create(){
//    return std::make_shared<do_message>();
//  }

};

