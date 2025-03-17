/*
 *   Copyright (C) 2010-2014,2016,2020,2022,2025 by Jonathan Naylor G4KLX
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

#include "APRSWriterThread.h"
#include "Utils.h"
#include "Log.h"

#include <algorithm>
#include <functional>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cassert>

// #define	DUMP_TX

const unsigned int CALLSIGN_LENGTH = 8U;

const unsigned int APRS_TIMEOUT = 10U;

CAPRSWriterThread::CAPRSWriterThread(const std::string& callsign, const std::string& password, const std::string& address, unsigned short port, const std::string& version, bool debug) :
CThread(),
m_username(callsign),
m_password(password),
m_debug(debug),
m_socket(address, port),
m_queue(2000U, "APRS Queue"),
m_exit(false),
m_connected(false),
m_reconnectTimer(1000U),
m_tries(1U),
m_aprsReadCallback(nullptr),
m_version(version)
{
	assert(!callsign.empty());
	assert(!password.empty());
	assert(!address.empty());
	assert(port > 0U);

	m_username.resize(CALLSIGN_LENGTH, ' ');
	m_username.erase(std::find_if(m_username.rbegin(), m_username.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), m_username.end());
	std::transform(m_username.begin(), m_username.end(), m_username.begin(), ::toupper);
}

CAPRSWriterThread::~CAPRSWriterThread()
{
	m_username.clear();
}

bool CAPRSWriterThread::start()
{
	run();

	return true;
}

void CAPRSWriterThread::entry()
{
	LogMessage("Starting the APRS Writer thread");

	m_connected = connect();
	if (!m_connected) {
		LogError("Connect attempt to the APRS server has failed");
		startReconnectionTimer();
	}

	try {
		while (!m_exit) {
			if (!m_connected) {
				sleep(100U);
				if (m_reconnectTimer.isRunning() && m_reconnectTimer.hasExpired()) {
					m_reconnectTimer.stop();

					m_connected = connect();
					if (!m_connected) {
						LogError("Reconnect attempt to the APRS server has failed");
						startReconnectionTimer();
					}
				}
			}

			if (m_connected) {
				m_tries = 0U;

				if (!m_queue.isEmpty()){
					unsigned int length = 0U;
					m_queue.getData((unsigned char*)&length, sizeof(unsigned int));

					unsigned char p[FRAME_BUFFER_SIZE];
					m_queue.getData(p, length);

					if (m_debug)
						CUtils::dump(1U, "APRS message", p, length);

					bool ret = m_socket.write(p, length);
					if (!ret) {
						m_connected = false;
						m_socket.close();
						LogError("Connection to the APRS thread has failed");
						startReconnectionTimer();
					}
				}

				if (m_connected) {
					std::string line;
					int length = m_socket.readLine(line, APRS_TIMEOUT);

					if (length < 0) {
						m_connected = false;
						m_socket.close();
						LogError("Error when reading from the APRS server");
						startReconnectionTimer();
					}

					if (length > 0 && line.at(0U) != '#'//check if we have something and if that something is an APRS frame
						&& m_aprsReadCallback != nullptr) { //do we have someone wanting an APRS Frame?
						// LogMessage("Received APRS Frame : %s", line.c_str());
						m_aprsReadCallback(line);
					}
				}
			}
		}

		if (m_connected)
			m_socket.close();

		while (!m_queue.isEmpty()) {
			unsigned char p;
			m_queue.getData(&p, 1U);
		}
	}
	catch (std::exception& e) {
		LogError("Exception raised in the APRS Writer thread - \"%s\"", e.what());
	}
	catch (...) {
		LogError("Unknown exception raised in the APRS Writer thread");
	}

	LogMessage("Stopping the APRS Writer thread");
}

void CAPRSWriterThread::setReadAPRSCallback(ReadAPRSFrameCallback cb)
{
	m_aprsReadCallback = cb;
}

bool CAPRSWriterThread::write(const unsigned char* data, unsigned int length)
{
	assert(data != nullptr);

	if (!m_connected)
		return false;

	unsigned int free = m_queue.freeSpace();
	if (free < (length + sizeof(unsigned int)))
		return false;

	bool ret = m_queue.addData((unsigned char*)&length, sizeof(unsigned int));
	if (!ret)
		return false;

	return m_queue.addData(data, length);
}

bool CAPRSWriterThread::isConnected() const
{
	return m_connected;
}

void CAPRSWriterThread::stop()
{
	m_exit = true;

	wait();
}

void CAPRSWriterThread::clock(unsigned int ms)
{
	m_reconnectTimer.clock(ms);
}

bool CAPRSWriterThread::connect()
{
	bool ret = m_socket.open();
	if (!ret)
		return false;

	//wait for lgin banner
	int length;
	std::string serverResponse;
	length = m_socket.readLine(serverResponse, APRS_TIMEOUT);
	if (length == 0) {
		LogError("No reply from the APRS server after %u seconds", APRS_TIMEOUT);
		m_socket.close();
		return false;
	}
	if (length < 0) {
		LogError("Error when reading from the APRS server");
		m_socket.close();
		return false;
	}

	LogMessage("Received login banner : %s", CUtils::rtrim(serverResponse).c_str());

	char connectString[200U];
	::sprintf(connectString, "user %s pass %s vers APRSGateway %s\n", m_username.c_str(), m_password.c_str(), m_version.c_str());

	ret = m_socket.writeLine(std::string(connectString));
	if (!ret) {
		m_socket.close();
		return false;
	}

	length = m_socket.readLine(serverResponse, APRS_TIMEOUT);
	if (length == 0) {
		LogError("No reply from the APRS server after %u seconds", APRS_TIMEOUT);
		m_socket.close();
		return false;
	}
	if (length < 0) {
		LogError("Error when reading from the APRS server");
		m_socket.close();
		return false;
	}

	LogMessage("Response from APRS server: %s", CUtils::rtrim(serverResponse).c_str());

	LogMessage("Connected to the APRS server");

	return true;
}

void CAPRSWriterThread::startReconnectionTimer()
{
	// Clamp at a ten minutes reconnect time
	m_tries++;
	if (m_tries > 10U)
		m_tries = 10U;

	LogMessage("Will attempt to reconnect in %u minutes", m_tries);

	m_reconnectTimer.setTimeout(m_tries * 60U);
	m_reconnectTimer.start();
}
