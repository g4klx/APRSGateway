/*
*   Copyright (C) 2020,2023 by Jonathan Naylor G4KLX
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#if !defined(APRSGateway_H)
#define	APRSGateway_H

#include "APRSWriterThread.h"
#include "Timer.h"
#include "Conf.h"

#include <cstdio>
#include <string>
#include <vector>

#if !defined(_WIN32) && !defined(_WIN64)
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <WS2tcpip.h>
#endif

class CAPRSGateway
{
public:
	CAPRSGateway(const std::string& file);
	~CAPRSGateway();

	int run();

private:
	CConf              m_conf;
	CAPRSWriterThread* m_writer;

	void writeAPRS(const std::string& message);

	static void onAPRS(const unsigned char* message, unsigned int length);
};

#endif
