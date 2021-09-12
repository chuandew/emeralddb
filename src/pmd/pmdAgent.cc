#include "bson.h"
#include "pd.h"
#include "pmdEDU.h"
#include "pmdEDUMgr.h"
#include "pmd.h"

using namespace bson;

#define ossRoundUpToMultipleX(x,y) (((x)+((y)-1))-(((x)+((y)-1))%(y)))
#define PMD_AGENT_RECIEVE_BUFFER_SZ 4096
#define EDB_PAGE_SIZE               4096

static int pmdProcessAgentRequest(char *pReceiveBuffer,
    int packetSize,
    char **ppResultBuffer,
    int *pResultBufferSize,
    bool *disconnect,
    pmdEDUCB *cb){

}
