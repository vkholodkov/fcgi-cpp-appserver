
#include <syslog.h>

#include "logger.h"

Logger theLog;

Logger::Logger()
{
    openlog("pm_backend_fcgi", LOG_NDELAY, LOG_DAEMON);
}

Logger::~Logger()
{
    closelog();
}

void Logger::log_info(const std::string &msg)
{
    syslog(LOG_INFO, "%s", msg.c_str());
}

void Logger::log_error(const std::string &msg)
{
    syslog(LOG_ERR, "%s", msg.c_str());
}

void Logger::log_warn(const std::string &msg)
{
    syslog(LOG_WARNING, "%s", msg.c_str());
}

void Logger::log_debug(const std::string &msg)
{
    syslog(LOG_DEBUG, "%s", msg.c_str());
}
