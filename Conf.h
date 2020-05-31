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

#if !defined(CONF_H)
#define	CONF_H

#include <string>

class CConf
{
public:
  CConf(const std::string& file);
  ~CConf();

  bool read();

  // The General section
  std::string  getCallsign() const;
  std::string  getSuffix() const;
  bool         getDebug() const;
  bool         getDaemon() const;

  // The aprs.fi section
  std::string  getAPRSServer() const;
  unsigned int getAPRSPort() const;
  std::string  getAPRSPassword() const;
  std::string  getAPRSDescription() const;

  // The Log section
  std::string  getLogFilePath() const;
  std::string  getLogFileRoot() const;

  // The Network section
  std::string  getNetworkAddress() const;
  unsigned int getNetworkPort() const;
  bool         getNetworkDebug() const;
  
private:
  std::string  m_file;
  std::string  m_callsign;
  std::string  m_suffix;
  bool         m_debug;
  bool         m_daemon;

  std::string  m_logFilePath;
  std::string  m_logFileRoot;

  std::string  m_aprsServer;
  unsigned int m_aprsPort;
  std::string  m_aprsPassword;
  std::string  m_aprsDescription;

  std::string  m_networkAddress;
  unsigned int m_networkPort;
  bool         m_networkDebug;
};

#endif
