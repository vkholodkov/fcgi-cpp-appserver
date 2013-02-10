
#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <string>
#include <sstream>

class Logger {
public:
    Logger();
    ~Logger();

    void log_info(const std::string&);
    void log_error(const std::string&);
    void log_warn(const std::string&);
    void log_debug(const std::string&);
};

void init_logging();

extern Logger theLog;

#define LogInfo(x) do { std::ostringstream o; o << x; theLog.log_info(o.str()); } while(0);
#define LogWarn(x) do { std::ostringstream o; o << x; theLog.log_warn(o.str()); } while(0);
#define LogError(x) do { std::ostringstream o; o << x; theLog.log_error(o.str()); } while(0);
#define LogDebug(x) do { std::ostringstream o; o << x; theLog.log_debug(o.str()); } while(0);

#ifdef SQLDEBUG
#define LogSQL(x) do { std::ostringstream o; o << "SQL: " << x; theLog.log_info(o.str()); } while(0);
#else
#define LogSQL(x) do { } while(0);
#endif

#endif
