/*
 *   Copyright (C) 2015,2016,2017,2018,2020 by Jonathan Naylor G4KLX
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

#include "Conf.h"
#include "Log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

const int BUFFER_SIZE = 500;

enum SECTION {
  SECTION_NONE,
  SECTION_GENERAL,
  SECTION_LOG,
  SECTION_APRS_FI,
  SECTION_NETWORK
};

CConf::CConf(const std::string& file) :
m_file(file),
m_callsign(),
m_debug(false),
m_daemon(false),
m_logFilePath(),
m_logFileRoot(),
m_aprsServer(),
m_aprsPort(0U),
m_aprsPassword(),
m_networkAddress("127.0.0.1"),
m_networkPort(0U)
{
}

CConf::~CConf()
{
}

bool CConf::read()
{
  FILE* fp = ::fopen(m_file.c_str(), "rt");
  if (fp == NULL) {
    ::fprintf(stderr, "Couldn't open the .ini file - %s\n", m_file.c_str());
    return false;
  }

  SECTION section = SECTION_NONE;

  char buffer[BUFFER_SIZE];
  while (::fgets(buffer, BUFFER_SIZE, fp) != NULL) {
	  if (buffer[0U] == '#')
		  continue;

	  if (buffer[0U] == '[') {
		  if (::strncmp(buffer, "[General]", 9U) == 0)
			  section = SECTION_GENERAL;
		  else if (::strncmp(buffer, "[Log]", 5U) == 0)
			  section = SECTION_LOG;
		  else if (::strncmp(buffer, "[aprs.fi]", 9U) == 0)
			  section = SECTION_APRS_FI;
		  else if (::strncmp(buffer, "[Network]", 9U) == 0)
			  section = SECTION_NETWORK;
		  else
			  section = SECTION_NONE;

		  continue;
	  }

	  char* key = ::strtok(buffer, " \t=\r\n");
	  if (key == NULL)
		  continue;

	  char* value = ::strtok(NULL, "\r\n");
	  if (section == SECTION_GENERAL) {
		  if (::strcmp(key, "Callsign") == 0) {
			  // Convert the callsign to upper case
			  for (unsigned int i = 0U; value[i] != 0; i++)
				  value[i] = ::toupper(value[i]);
			  m_callsign = value;
		  } else if (::strcmp(key, "Debug") == 0)
			  m_debug = ::atoi(value) == 1;
		  else if (::strcmp(key, "Daemon") == 0)
			  m_daemon = ::atoi(value) == 1;
	  } else if (section == SECTION_LOG) {
		  if (::strcmp(key, "FilePath") == 0)
			  m_logFilePath = value;
		  else if (::strcmp(key, "FileRoot") == 0)
			  m_logFileRoot = value;
	  } else if (section == SECTION_APRS_FI) {
		  if (::strcmp(key, "Server") == 0)
			  m_aprsServer = value;
		  else if (::strcmp(key, "Port") == 0)
			  m_aprsPort = (unsigned int)::atoi(value);
		  else if (::strcmp(key, "Password") == 0)
			  m_aprsPassword = value;
	  } else if (section == SECTION_NETWORK) {
		  if (::strcmp(key, "Address") == 0)
			  m_networkAddress = value;
		  else if (::strcmp(key, "Port") == 0)
			  m_networkPort = (unsigned int)::atoi(value);
	  }
  }

  ::fclose(fp);

  return true;
}

std::string CConf::getCallsign() const
{
	return m_callsign;
}

bool CConf::getDebug() const
{
	return m_debug;
}

bool CConf::getDaemon() const
{
	return m_daemon;
}

std::string CConf::getLogFilePath() const
{
  return m_logFilePath;
}

std::string CConf::getAPRSServer() const
{
	return m_aprsServer;
}

unsigned int CConf::getAPRSPort() const
{
	return m_aprsPort;
}

std::string CConf::getAPRSPassword() const
{
	return m_aprsPassword;
}

std::string CConf::getLogFileRoot() const
{
  return m_logFileRoot;
}

std::string CConf::getNetworkAddress() const
{
	return m_networkAddress;
}

unsigned int CConf::getNetworkPort() const
{
	return m_networkPort;
}
