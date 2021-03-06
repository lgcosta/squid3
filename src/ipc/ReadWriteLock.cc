/*
 * DEBUG: section 54    Interprocess Communication
 */

#include "squid.h"
#include "ipc/ReadWriteLock.h"
#include "Store.h"

bool
Ipc::ReadWriteLock::lockShared()
{
    ++readLevel; // this locks "new" writers out
    if (!writeLevel || appending) { // nobody is writing, or sharing is OK
        ++readers;
        return true;
    }
    --readLevel;
    return false;
}

bool
Ipc::ReadWriteLock::lockExclusive()
{
    if (!writeLevel++) { // we are the first writer + lock "new" readers out
        if (!readLevel) { // no old readers and nobody is becoming one
            writing = true;
            return true;
        }
    }
    --writeLevel;
    return false;
}

void
Ipc::ReadWriteLock::unlockShared()
{
    assert(readers > 0);
    --readers;
    --readLevel;
}

void
Ipc::ReadWriteLock::unlockExclusive()
{
    assert(writing);
    appending = false;
    writing = false;
    --writeLevel;
}

void
Ipc::ReadWriteLock::switchExclusiveToShared()
{
    assert(writing);
    ++readLevel; // must be done before we release exclusive control
    ++readers;
    unlockExclusive();
}

void
Ipc::ReadWriteLock::startAppending()
{
    assert(writing);
    appending = true;
}

void
Ipc::ReadWriteLock::updateStats(ReadWriteLockStats &stats) const
{
    if (readers) {
        ++stats.readable;
        stats.readers += readers;
    } else if (writing) {
        ++stats.writeable;
        ++stats.writers;
        stats.appenders += appending;
    } else {
        ++stats.idle;
    }
    ++stats.count;
}

/* Ipc::ReadWriteLockStats */

Ipc::ReadWriteLockStats::ReadWriteLockStats()
{
    memset(this, 0, sizeof(*this));
}

void
Ipc::ReadWriteLockStats::dump(StoreEntry &e) const
{
    storeAppendPrintf(&e, "Available locks: %9d\n", count);

    if (!count)
        return;

    storeAppendPrintf(&e, "Reading: %9d %6.2f%%\n",
                      readable, (100.0 * readable / count));
    storeAppendPrintf(&e, "Writing: %9d %6.2f%%\n",
                      writeable, (100.0 * writeable / count));
    storeAppendPrintf(&e, "Idle:    %9d %6.2f%%\n",
                      idle, (100.0 * idle / count));

    if (readers || writers) {
        const int locked = readers + writers;
        storeAppendPrintf(&e, "Readers:         %9d %6.2f%%\n",
                          readers, (100.0 * readers / locked));
        const double appPerc = writers ? (100.0 * appenders / writers) : 0.0;
        storeAppendPrintf(&e, "Writers:         %9d %6.2f%% including Appenders: %9d %6.2f%%\n",
                          writers, (100.0 * writers / locked),
                          appenders, appPerc);
    }
}
