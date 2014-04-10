#include "squid.h"
#include "SquidTime.h"

#define STUB_API "time.cc"
#include "STUB.h"

struct timeval current_time;
double current_dtime;
time_t squid_curtime = 0;

time_t getCurrentTime(void) STUB_RETVAL(0)
int tvSubMsec(struct timeval, struct timeval) STUB_RETVAL(0)
const char * Time::FormatStrf(time_t ) STUB_RETVAL("")
const char * Time::FormatHttpd(time_t ) STUB_RETVAL("")

void TimeEngine::tick() STUB
TimeEngine::~TimeEngine() STUB

