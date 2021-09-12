#pragma once

#include "core.h"

enum pmdEDUEventTypes {
  PMD_EDU_EVENT_NONE = 0,
  PMD_EDU_EVENT_TERM,   // terminate EDU
  PMD_EDU_EVENT_RESUME, // resume a edu, the data is startEDU's argv
  PMD_EDU_EVENT_ACTIVE,
  PMD_EDU_EVENT_DEACTIVE,
  PMD_EDU_EVENT_MSG,
  PMD_EDU_EVENT_TIMEOUT,   // timeout
  PMD_EDU_EVENT_LOCKWAKEUP // transaction lock wake up
};

class pmdEDUEvent {
public:
  pmdEDUEvent()
      : _eventType(PMD_EDU_EVENT_NONE), _release(false), _Data(nullptr) {}

  pmdEDUEvent(pmdEDUEventTypes type)
      : _eventType(type), _release(false), _Data(nullptr) {}

  pmdEDUEvent(pmdEDUEventTypes type, bool release, void *data)
      : _eventType(type), _release(release), _Data(data) {}

  void reset() {
    _eventType = PMD_EDU_EVENT_NONE;
    _release = false;
    _Data = nullptr;
  }

  pmdEDUEventTypes _eventType;
  bool _release;
  void *_Data;
};
