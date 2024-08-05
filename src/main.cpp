#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <fstream>
#include <dirent.h>
#include <map>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

// Function to get the memory usage of a process by its PID
long getProcessMemoryUsage(int pid) {
    std::string statusFilePath = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream statusFile(statusFilePath);
    std::string line;
    long memoryUsage = 0;

    if (statusFile.is_open()) {
        while (std::getline(statusFile, line)) {
            if (line.find("VmRSS:") != std::string::npos) {
                std::istringstream iss(line);
                std::string key;
                long value;
                std::string unit;
                iss >> key >> value >> unit;
                memoryUsage = value; // VmRSS value is in KB
                break;
            }
        }
        statusFile.close();
    }

    return memoryUsage;
}

// Function to get the name of a process by its PID
std::string getProcessName(int pid) {
    std::string commFilePath = "/proc/" + std::to_string(pid) + "/comm";
    std::ifstream commFile(commFilePath);
    std::string processName;

    if (commFile.is_open()) {
        std::getline(commFile, processName);
        commFile.close();
    }

    return processName;
}

int main() {

    auto logger = spdlog::rotating_logger_mt<spdlog::async_factory>("logger", "../log/log.txt", 1024*1024*5, 5);
	spdlog::set_default_logger(logger);
	spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e][%t][%p][%n:%l] %v");
	spdlog::flush_every(std::chrono::seconds(3));
	spdlog::info(" ");
	spdlog::info(" ");
	spdlog::info("                                      |-----------------------------------------------|");
	spdlog::info("                                      |---------内存在用监控系统开始运行!----------|");
	spdlog::info("                                      |-----------------------------------------------|");
	spdlog::info(" ");

    while (true) {
        DIR* procDir = opendir("/proc");
        struct dirent* entry;
        std::vector<std::pair<long, std::string>> processList;

        if (procDir == nullptr) {
            std::cerr << "Error: Unable to open /proc directory." << std::endl;
            return 1;
        }

        while ((entry = readdir(procDir)) != nullptr) {
            if (entry->d_type == DT_DIR) {
                int pid = atoi(entry->d_name);
                if (pid > 0) {
                    long memUsage = getProcessMemoryUsage(pid);
                    std::string processName = getProcessName(pid);
                    processList.push_back(std::make_pair(memUsage, processName));
                }
            }
        }

        closedir(procDir);

        std::sort(processList.rbegin(), processList.rend());

        std::cout << "Top 10 memory-consuming processes:" << std::endl;
        for (size_t i = 0; i < 10 && i < processList.size(); ++i) {
            std::cout << "Process: " << processList[i].second << ", Memory Usage: " << processList[i].first/1024 << " MB" << std::endl;
            spdlog::info( "Process:{0:s}, Memory Usage:{1:d}  MB", processList[i].second,processList[i].first/1024);
        }
         spdlog::info("---------------------------------");
        std::cout << "---------------------------------" << std::endl;
        sleep(1);
    }

    return 0;
}
