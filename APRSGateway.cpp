/*
*   Copyright (C) 2016,2017,2018,2020,2022 by Jonathan Naylor G4KLX
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

#include "APRSGateway.h"
#include "APRSWriterThread.h"
#include "StopWatch.h"
#include "UDPSocket.h"
#include "Version.h"
#include "Thread.h"
#include "Timer.h"
#include "Utils.h"
#include "Log.h"
#include "GitVersion.h"

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
const char* DEFAULT_INI_FILE = "APRSGateway.ini";
#else
const char* DEFAULT_INI_FILE = "/etc/APRSGateway.ini";
#endif

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <ctime>
#include <cstring>

int main(int argc, char** argv)
{
	const char* iniFile = DEFAULT_INI_FILE;
	if (argc > 1) {
		for (int currentArg = 1; currentArg < argc; ++currentArg) {
			std::string arg = argv[currentArg];
			if ((arg == "-v") || (arg == "--version")) {
				::fprintf(stdout, "APRSGateway version %s git #%.7s\n", VERSION, gitversion);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: APRSGateway [-v|--version] [filename]\n");
				return 1;
			} else {
				iniFile = argv[currentArg];
			}
		}
	}

	CAPRSGateway* gateway = new CAPRSGateway(std::string(iniFile));
	gateway->run();
	delete gateway;

	return 0;
}

CAPRSGateway::CAPRSGateway(const std::string& file) :
m_conf(file)
{
	CUDPSocket::startup();
}

CAPRSGateway::~CAPRSGateway()
{
	CUDPSocket::shutdown();
}

void CAPRSGateway::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "APRSGateway: cannot read the .ini file\n");
		return;
	}

#if !defined(_WIN32) && !defined(_WIN64)
	bool m_daemon = m_conf.getDaemon();
	if (m_daemon) {
		// Create new process
		pid_t pid = ::fork();
		if (pid == -1) {
			::fprintf(stderr, "Couldn't fork() , exiting\n");
			return;
		} else if (pid != 0) {
			exit(EXIT_SUCCESS);
		}

		// Create new session and process group
		if (::setsid() == -1) {
			::fprintf(stderr, "Couldn't setsid(), exiting\n");
			return;
		}

		// Set the working directory to the root directory
		if (::chdir("/") == -1) {
			::fprintf(stderr, "Couldn't cd /, exiting\n");
			return;
		}

		// If we are currently root...
		if (getuid() == 0) {
			struct passwd* user = ::getpwnam("mmdvm");
			if (user == NULL) {
				::fprintf(stderr, "Could not get the mmdvm user, exiting\n");
				return;
			}

			uid_t mmdvm_uid = user->pw_uid;
			gid_t mmdvm_gid = user->pw_gid;

			// Set user and group ID's to mmdvm:mmdvm
			if (setgid(mmdvm_gid) != 0) {
				::fprintf(stderr, "Could not set mmdvm GID, exiting\n");
				return;
			}

			if (setuid(mmdvm_uid) != 0) {
				::fprintf(stderr, "Could not set mmdvm UID, exiting\n");
				return;
			}

			// Double check it worked (AKA Paranoia) 
			if (setuid(0) != -1) {
				::fprintf(stderr, "It's possible to regain root - something is wrong!, exiting\n");
				return;
			}
		}
	}
#endif

#if !defined(_WIN32) && !defined(_WIN64)
        ret = ::LogInitialise(m_daemon, m_conf.getLogFilePath(), m_conf.getLogFileRoot(), m_conf.getLogFileLevel(), m_conf.getLogDisplayLevel(), m_conf.getLogFileRotate());
#else
        ret = ::LogInitialise(false, m_conf.getLogFilePath(), m_conf.getLogFileRoot(), m_conf.getLogFileLevel(), m_conf.getLogDisplayLevel(), m_conf.getLogFileRotate());
#endif
	if (!ret) {
		::fprintf(stderr, "APRSGateway: unable to open the log file\n");
		return;
	}

#if !defined(_WIN32) && !defined(_WIN64)
	if (m_daemon) {
		::close(STDIN_FILENO);
		::close(STDOUT_FILENO);
		::close(STDERR_FILENO);
	}
#endif

	CAPRSWriterThread* writer = new CAPRSWriterThread(m_conf.getCallsign(), m_conf.getAPRSPassword(), m_conf.getAPRSServer(), m_conf.getAPRSPort(), VERSION, m_conf.getDebug());
	ret = writer->start();
	if (!ret) {
		delete writer;
		return;
	}

	CUDPSocket aprsSocket(m_conf.getNetworkAddress(), m_conf.getNetworkPort());
	ret = aprsSocket.open();
	if (!ret)
		return;

	CStopWatch stopWatch;
	stopWatch.start();

	LogMessage("APRSGateway-%s is starting", VERSION);
 	LogMessage("Built %s %s (GitID #%.7s)", __TIME__, __DATE__, gitversion);

	for (;;) {
		unsigned char buffer[FRAME_BUFFER_SIZE];
		sockaddr_storage addr;
		unsigned int addrLen;

		// From a gateway to aprs.fi
		unsigned int len = aprsSocket.read(buffer, FRAME_BUFFER_SIZE, addr, addrLen);
		if (len > 0U)
			writer->write(buffer, len);

		unsigned int ms = stopWatch.elapsed();
		stopWatch.start();

		writer->clock(ms);

		if (ms < 20U)
			CThread::sleep(20U);
	}

	aprsSocket.close();

	writer->stop();
	delete writer;

	::LogFinalise();
}
