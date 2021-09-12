#include "pmdEDU.h"
#include "pd.h"
#include "pmd.h"
#include "pmdEDUMgr.h"

static std::map<EDU_TYPES, std::string> mapEDUName;
static std::map<EDU_TYPES, EDU_TYPES> mapEDUTypeSys;

int registerEDUName(EDU_TYPES type, const char *name, bool system) {
  int rc = EDB_OK;
  auto it = mapEDUName.find(type);
  if (it != mapEDUName.end()) {
    PD_LOG(PDERROR, "EDU Type conflict[type:%d, %s<->%s]", (int)type,
           it->second.c_str(), name);
    rc = EDB_SYS;
    goto error;
  }
  mapEDUName[type] = std::string(name);
  if (system) {
    mapEDUTypeSys[type] = type;
  }
done:
  return rc;
error:
  goto done;
}

const char *getEDUName(EDU_TYPES type) {
  auto it = mapEDUName.find(type);
  if (it != mapEDUName.end()) {
    return it->second.c_str();
  }

  return "Unknown";
}

bool isSystemEDU(EDU_TYPES type) {
  auto it = mapEDUTypeSys.find(type);
  return it == mapEDUTypeSys.end() ? false : true;
}

pmdEDUCB::pmdEDUCB(pmdEDUMgr *mgr, EDU_TYPES type)
    : _type(type), _mgr(mgr), _status(PMD_EDU_CREATING), _id(0),
      _isForced(false), _isDisconnected(false) {}

struct _eduEntryInfo {
  EDU_TYPES type;
  int regResult;
  pmdEntryPoint entryFunc;
};

#define ON_EDUTYPE_TO_ENTRY1(type, system, entry, name)                        \
  { type, registerEDUName(type, name, system), entry }

pmdEntryPoint getEntryFuncByType(EDU_TYPES type) {
  pmdEntryPoint rt = nullptr;
  static const _eduEntryInfo entry[] = {
      // ON_EDUTYPE_TO_ENTRY1(EDU_TYPE_AGENT, false, pmdAgentEntryPoint, "Agent"),
      ON_EDUTYPE_TO_ENTRY1(EDU_TYPE_TCPLISTENDER, true,
                           pmdTcpListenerEntryPoint, "TCPListener"),
      ON_EDUTYPE_TO_ENTRY1(EDU_TYPE_MAXIMUM, false, nullptr, "Unknown")};

  static const unsigned int number = sizeof(entry) / sizeof(_eduEntryInfo);
  for (unsigned int index = 0; index < number; ++index) {
    if (entry[index].type == type) {
      rt = entry[index].entryFunc;
      goto done;
    }
  }

done:
  return rt;
}

int pmdRecv(char *pBuffer, int recvSize, ossSocket *sock, pmdEDUCB *cb) {
  int rc = EDB_OK;
  EDB_ASSERT(sock, "Socket is nullptr");
  EDB_ASSERT(cb, "cb is nullptr");

  while (true) {
    if (cb->isForced()) {
      rc = EDB_APP_FORCED;
      goto done;
    }

    rc = sock->recv(pBuffer, recvSize);
    if (EDB_TIMEOUT == rc) {
      continue;
    }
    goto done;
  }

done:
  return rc;
}

int pmdSend(const char *pBuffer, int sendSize, ossSocket *sock, pmdEDUCB *cb) {
  int rc = EDB_OK;
  EDB_ASSERT(sock, "Socket is nullptr");
  EDB_ASSERT(cb, "cb is nuullptr");

  while (true) {
    if (cb->isForced()) {
      rc = EDB_APP_FORCED;
      goto done;
    }
    rc = sock->send(pBuffer, sendSize);
    if (rc == EDB_TIMEOUT) {
      continue;
    }
    goto done;
  }
done:
  return rc;
}

int pmdEDUEntryPoint(EDU_TYPES type, pmdEDUCB *cb, void *arg) {
  int rc = EDB_OK;
  EDUID myEDUID = cb->getID();
  pmdEDUMgr *eduMgr = cb->getEDUMgr();
  pmdEDUEvent event;
  bool eduDestroyed = false;
  bool isForced = false;

  // main loop
  while (!eduDestroyed) {
    type = cb->getType();
    // wait for 1000 milliseconds for event
    if (!cb->waitEvent(event, 1000)) {
      if (cb->isForced()) {
        PD_LOG(PDEVENT, "EDU %lld is forced", myEDUID);
        isForced = true;
      } else {
        continue;
      }
    }

    if (!isForced && event._eventType == PMD_EDU_EVENT_RESUME) {
      // set EDU status to wait
      // eduMgr->waitEDU(myEDUID);
      // run the main function
      pmdEntryPoint entryFunc = getEntryFuncByType(type);
      if (!entryFunc) {
        PD_LOG(PDERROR, "EDU %lld type %d entry point func is nullptr", myEDUID,
               type);
        EDB_SHUTDOWN_DB;
        rc = EDB_SYS;
      } else {
        rc = entryFunc(cb, event._Data);
      }

      // sanity check
      if (EDB_IS_DB_UP) {
        // system EDU should never exit when db is still running
        if (isSystemEDU(cb->getType())) {
          PD_LOG(PDSEVERE, "System EDU: %lld, type %s exists with %d", myEDUID,
                 getEDUName(type), rc);
          EDB_SHUTDOWN_DB
        } else if (rc) {
          PD_LOG(PDWARNING, "EDU %lld, type %s, exist with %d", myEDUID,
                 getEDUName(type));
        }
      }
      //  eduMgr->waitEDU(myEDUID);
    } else if (!isForced && event._eventType != PMD_EDU_EVENT_TERM) {
      PD_LOG(PDERROR, "Receive the wrong event %d in EDU %lld, type %s",
             event._eventType, myEDUID, getEDUName(type));

      rc = EDB_SYS;
    } else if (isForced && event._eventType == PMD_EDU_EVENT_TERM &&
               cb->isForced()) {
      PD_LOG(PDEVENT, "EDU %lld, type %s is forced", myEDUID, type);
      isForced = true;
    }

    // release the event data
    if (!isForced && event._Data && event._release) {
      free(event._Data);
      event.reset();
    }

    // rc = eduMgr->returnEDU(myEDUID, isForced, &eduDestroyed);
    if (rc) {
      PD_LOG(PDERROR, "Invalid EDU Status for EDU: %lld, type %s", myEDUID,
             getEDUName(type));
    }

    PD_LOG(PDDEBUG, "Terminating thread for EDU: %lld, type %s ", myEDUID,
           getEDUName(type));
  }

  return 0;
}
