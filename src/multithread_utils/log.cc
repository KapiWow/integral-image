#include <iostream>

#include <multithread_utils/log.hh>

Log logger;

void Log::operator() (std::string msg) {
    std::unique_lock<std::mutex> lock{_mtx};
    std::cout << msg << std::endl;
}

void Log::operator() (std::stringstream msg) {
    operator()(msg.str());
}

void Log::operator() (const char* msg) {
    operator()(std::string(msg));
}
