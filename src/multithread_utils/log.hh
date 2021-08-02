#ifndef LOG_HH
#define LOG_HH

#include <string>
#include <sstream>
#include <mutex>

///Simple threadsafe logger
class Log {
public:
    void operator() (std::string msg);
    void operator() (std::stringstream msg);
    void operator() (const char* msg);
private:
    std::mutex _mtx;
};

extern Log logger;

#endif
