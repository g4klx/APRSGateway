/*
*   Copyright (C) 2016,2017,2018,2020,2022,2023 by Jonathan Naylor G4KLX
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
#include "MQTTConnection.h"
#include "StopWatch.h"
#include "TCPSocket.h"
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

// In Log.cpp
extern CMQTTConnection* m_mqtt;

static CAPRSGateway* gateway = NULL;

static bool m_killed = false;
static int  m_signal = 0;

#if !defined(_WIN32) && !defined(_WIN64)
static void sigHandler(int signum)
{
	m_killed = true;
	m_signal = signum;
}
#endif

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

#if !defined(_WIN32) && !defined(_WIN64)
	::signal(SIGINT,  sigHandler);
	::signal(SIGTERM, sigHandler);
	::signal(SIGHUP,  sigHandler);
#endif

	int ret = 0;

	do {
		m_signal = 0;

		gateway = new CAPRSGateway(std::string(iniFile));
		ret = gateway->run();

		delete gateway;

		if (m_signal == 2)
			::LogInfo("APRSGateway-%s exited on receipt of SIGINT", VERSION);

		if (m_signal == 15)
			::LogInfo("APRSGateway-%s exited on receipt of SIGTERM", VERSION);

		if (m_signal == 1)
			::LogInfo("APRSGateway-%s restarted on receipt of SIGHUP", VERSION);

	} while (m_signal == 1);

	::LogFinalise();

	return ret;
}

CAPRSGateway::CAPRSGateway(const std::string& file) :
m_conf(file),
m_writer(NULL)
{
}

CAPRSGateway::~CAPRSGateway()
{
}

int CAPRSGateway::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "APRSGateway: cannot read the .ini file\n");
		return 1;
	}

#if !defined(_WIN32) && !defined(_WIN64)
	bool m_daemon = m_conf.getDaemon();
	if (m_daemon) {
		// Create new process
		pid_t pid = ::fork();
		if (pid == -1) {
			::fprintf(stderr, "Couldn't fork() , exiting\n");
			return -1;
		} else if (pid != 0) {
			exit(EXIT_SUCCESS);
		}

		// Create new session and process group
		if (::setsid() == -1) {
			::fprintf(stderr, "Couldn't setsid(), exiting\n");
			return -1;
		}

		// Set the working directory to the root directory
		if (::chdir("/") == -1) {
			::fprintf(stderr, "Couldn't cd /, exiting\n");
			return -1;
		}

		// If we are currently root...
		if (getuid() == 0) {
			struct passwd* user = ::getpwnam("mmdvm");
			if (user == NULL) {
				::fprintf(stderr, "Could not get the mmdvm user, exiting\n");
				return -1;
			}

			uid_t mmdvm_uid = user->pw_uid;
			gid_t mmdvm_gid = user->pw_gid;

			// Set user and group ID's to mmdvm:mmdvm
			if (setgid(mmdvm_gid) != 0) {
				::fprintf(stderr, "Could not set mmdvm GID, exiting\n");
				return -1;
			}

			if (setuid(mmdvm_uid) != 0) {
				::fprintf(stderr, "Could not set mmdvm UID, exiting\n");
				return -1;
			}

			// Double check it worked (AKA Paranoia) 
			if (setuid(0) != -1) {
				::fprintf(stderr, "It's possible to regain root - something is wrong!, exiting\n");
				return -1;
			}
		}
	}
#endif

        ::LogInitialise(m_conf.getLogDisplayLevel(), m_conf.getLogMQTTLevel());

#if !defined(_WIN32) && !defined(_WIN64)
	if (m_daemon) {
		::close(STDIN_FILENO);
		::close(STDOUT_FILENO);
		::close(STDERR_FILENO);
	}
#endif
	m_writer = new CAPRSWriterThread(m_conf.getCallsign(), m_conf.getAPRSPassword(), m_conf.getAPRSServer(), m_conf.getAPRSPort(), VERSION, m_conf.getDebug());
	ret = m_writer->start();
	if (!ret) {
		delete m_writer;
		return -1;
	}

	std::vector<std::pair<std::string, void (*)(const unsigned char*, unsigned int)>> subscriptions;
	subscriptions.push_back(std::make_pair("aprs", CAPRSGateway::onAPRS));

	m_mqtt = new CMQTTConnection(m_conf.getMQTTAddress(), m_conf.getMQTTPort(), "aprs-gateway", subscriptions, m_conf.getMQTTKeepalive());
	ret = m_mqtt->open();
	if (!ret) {
		m_writer->stop();
		delete m_writer;
		return -1;
	}

	CStopWatch stopWatch;
	stopWatch.start();

	LogMessage("APRSGateway-%s is starting", VERSION);
 	LogMessage("Built %s %s (GitID #%.7s)", __TIME__, __DATE__, gitversion);

	for (;;) {
		unsigned int ms = stopWatch.elapsed();
		stopWatch.start();

		m_writer->clock(ms);

		if (ms < 20U)
			CThread::sleep(20U);
	}

	m_writer->stop();
	delete m_writer;

	return 0;
}

void CAPRSGateway::writeAPRS(const std::string& message)
{
	assert(m_writer != NULL);

	m_writer->write(message);
}

void CAPRSGateway::onAPRS(const unsigned char* message, unsigned int length)
{
	assert(gateway != NULL);
	assert(message != NULL);

	gateway->writeAPRS(std::string((char*)message, length));
}

