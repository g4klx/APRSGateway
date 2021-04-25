/*
 *   Copyright (C) 2010,2011,2012,2016,2020 by Jonathan Naylor G4KLX
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

#ifndef	APRSWriterThread_H
#define	APRSWriterThread_H

#include "TCPSocket.h"
#include "RingBuffer.h"
#include "Timer.h"
#include "Thread.h"

#include <string>

const unsigned int FRAME_BUFFER_SIZE = 300U;

typedef void (*ReadAPRSFrameCallback)(const std::string&);

class CAPRSWriterThread : public CThread {
public:
	CAPRSWriterThread(const std::string& callsign, const std::string& password, const std::string& address, unsigned short port, bool debug);
	CAPRSWriterThread(const std::string& callsign, const std::string& password, const std::string& address, unsigned short port, const std::string& filter, const std::string& clientName, bool debug);
	virtual ~CAPRSWriterThread();

	virtual bool start();

	virtual bool isConnected() const;

	virtual bool write(const unsigned char* data, unsigned int length);

	virtual void entry();

	virtual void stop();

	void setReadAPRSCallback(ReadAPRSFrameCallback cb);

	void clock(unsigned int ms);

private:
	std::string                m_username;
	std::string                m_password;
	bool                       m_debug;
	CTCPSocket                 m_socket;
	CRingBuffer<unsigned char> m_queue;
	bool                       m_exit;
	bool                       m_connected;
	CTimer                     m_reconnectTimer;
	unsigned int               m_tries;
	ReadAPRSFrameCallback      m_aprsReadCallback;
	std::string                m_filter;
	std::string                m_clientName;

	bool connect();
	void startReconnectionTimer();
};

#endif
