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
  bool         getDebug() const;
  bool         getDaemon() const;

  // The APRS-IS section
  std::string  getAPRSServer() const;
  unsigned short getAPRSPort() const;
  std::string  getAPRSPassword() const;

  // The Log section
  unsigned int getLogDisplayLevel() const;
  unsigned int getLogFileLevel() const;
  std::string  getLogFilePath() const;
  std::string  getLogFileRoot() const;
  bool         getLogFileRotate() const;

  // The Network section
  std::string  getNetworkAddress() const;
  unsigned short getNetworkPort() const;
  
private:
  std::string  m_file;
  std::string  m_callsign;
  bool         m_debug;
  bool         m_daemon;

  unsigned int m_logDisplayLevel;
  unsigned int m_logFileLevel;
  std::string  m_logFilePath;
  std::string  m_logFileRoot;
  bool         m_logFileRotate;

  std::string  m_aprsServer;
  unsigned short m_aprsPort;
  std::string  m_aprsPassword;

  std::string  m_networkAddress;
  unsigned short m_networkPort;
};

#endif
