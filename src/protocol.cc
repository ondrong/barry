///
/// \file	protocol.cc
///		USB Blackberry bulk protocol API
///

/*
    Copyright (C) 2005-2007, Net Direct Inc. (http://www.netdirect.ca/)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the GNU General Public License in the COPYING file at the
    root directory of this project for more details.
*/

#include "protocol.h"
#include "protostructs.h"
#include "data.h"
#include "endian.h"
#include "error.h"
#include "debug.h"

#include <sstream>

namespace Barry { namespace Protocol {

void CheckSize(const Data &packet, size_t requiredsize)
{
	const Packet *p = (const Packet *) packet.GetData();

// FIXME - NOTE - can't rely on GetSize() when using libusb-stable
//	if( btohs(p->size) != packet.GetSize() || packet.GetSize() < requiredsize )

	if( packet.GetSize() < requiredsize )
	{
		std::ostringstream oss;
		oss << "Bad packet size. Packet: " << btohs(p->size)
		    << ". DataSize(): " << packet.GetSize()
		    << ". Required size: " << requiredsize;
		eout(oss.str());
		eout(packet);
		throw BError(oss.str());
	}
}

unsigned int GetSize(const Data &packet)
{
	CheckSize(packet, 4);
	uint16_t size = *((uint16_t *)&packet.GetData()[2]);
	return btohs(size);
}

}} // namespace Barry::Protocol

