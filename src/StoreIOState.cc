
/*
 * DEBUG: section 20    Swap Dir base object
 * AUTHOR: Robert Collins
 *
 * SQUID Web Proxy Cache          http://www.squid-cache.org/
 * ----------------------------------------------------------
 *
 *  Squid is the result of efforts by numerous individuals from
 *  the Internet community; see the CONTRIBUTORS file for full
 *  details.   Many organizations have provided support for Squid's
 *  development; see the SPONSORS file for full details.  Squid is
 *  Copyrighted (C) 2001 by the Regents of the University of
 *  California; see the COPYRIGHT file for full details.  Squid
 *  incorporates software developed and/or copyrighted by other
 *  sources; see the CREDITS file for full details.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 *
 */

#include "squid.h"
#include "Debug.h"
#include "defines.h"
#include "StoreIOState.h"

void *
StoreIOState::operator new (size_t amount)
{
    assert(0);
    return (void *)1;
}

void
StoreIOState::operator delete (void *address) {assert (0);}

StoreIOState::StoreIOState() :
        swap_dirn(-1), swap_filen(-1), e(NULL), mode(O_BINARY),
        offset_(0), file_callback(NULL), callback(NULL), callback_data(NULL)
{
    read.callback = NULL;
    read.callback_data = NULL;
    flags.closing = false;
}

off_t
StoreIOState::offset() const
{
    return offset_;
}

StoreIOState::~StoreIOState()
{
    debugs(20,3, "StoreIOState::~StoreIOState: " << this);

    if (read.callback_data)
        cbdataReferenceDone(read.callback_data);

    if (callback_data)
        cbdataReferenceDone(callback_data);
}