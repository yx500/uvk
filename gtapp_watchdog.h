#pragma once

#include <QSharedMemory>
#include <iomanip>
#include <iostream>
#include <optional>

class gtapp_watchdog {

  QSharedMemory shmem;
  int *tick = nullptr;
  int last;

public:
  gtapp_watchdog(const char *name) : shmem( QString::fromLatin1(name) +"_gtapp_watchdog" ) {};
  ~gtapp_watchdog() {
    if (shmem.isAttached())
      shmem.detach();
  }

  const std::string key() const { return shmem.key().toStdString(); }

  void gav() {
    if (!shmem.isAttached()) {
      if (!shmem.create(256)) {
        std::cerr << shmem.errorString().toStdString() << std::endl;
        throw std::runtime_error(shmem.errorString().toStdString());
      }
      tick = reinterpret_cast<int *>(shmem.data());
      *tick = 0;
    }
    shmem.lock();
    (*tick) += 1;
    shmem.unlock();
  }

  std::optional<bool> checkout() {
    if (!shmem.isAttached()) {
      if( shmem.attach(QSharedMemory::AccessMode::ReadOnly) ){
        tick = reinterpret_cast<int *>(shmem.data());
        shmem.lock();
        last = *tick;
        shmem.unlock();
        return true;
      }
      else return std::nullopt;
    }

    shmem.lock();
    int cc = *tick;
    shmem.unlock();

    if (cc==last)
      return false;

    last = cc;
    return true;
  }
};
