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

#include <fcntl.h>

// 获取 CPU 温度
float getCpuTemperature() {
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    if (!file.is_open()) {
        std::cerr << "Failed to open temperature file." << std::endl;
        return -1.0f;
    }

    float temp;
    file >> temp;
    file.close();
    return temp / 1000.0f;  // 转换为摄氏度
}

// 获取 CPU 占用率
float getCpuUsage() {
    static long prevIdle = 0, prevTotal = 0;

    std::ifstream file("/proc/stat");
    if (!file.is_open()) {
        std::cerr << "Failed to open /proc/stat file." << std::endl;
        return -1.0f;
    }

    std::string line;
    std::getline(file, line);
    file.close();

    std::istringstream iss(line);
    std::string cpu;
    long user, nice, system, idle, iowait, irq, softirq, steal;
    iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    long total = user + nice + system + idle + iowait + irq + softirq + steal;
    long idleTime = idle + iowait;
    
    long totalDelta = total - prevTotal;
    long idleDelta = idleTime - prevIdle;

    float cpuUsage = 100.0f * (totalDelta - idleDelta) / totalDelta;
    prevTotal = total;
    prevIdle = idleTime;

    return cpuUsage;
}

float getMemoryUsage() {
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) {
        std::cerr << "Failed to open /proc/meminfo file." << std::endl;
        return -1.0f;
    }

    std::string line;
    long totalMemory = 0, availableMemory = 0;
    while (std::getline(file, line)) {
        if (line.find("MemTotal:") == 0) {
            std::istringstream iss(line);
            std::string key;
            iss >> key >> totalMemory;
        } else if (line.find("MemAvailable:") == 0) {
            std::istringstream iss(line);
            std::string key;
            iss >> key >> availableMemory;
        }
    }
    file.close();

    if (totalMemory == 0) {
        return -1.0f;
    }

    // Memory Usage = (Total Memory - Available Memory) / Total Memory * 100
    float usedMemory = (totalMemory - availableMemory) / static_cast<float>(totalMemory);
    return usedMemory * 100.0f;  // 返回百分比
}



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
	spdlog::flush_every(std::chrono::seconds(1));
	spdlog::info(" ");
	spdlog::info(" ");
	spdlog::info("                                      |------------------------------------------------------|");
	spdlog::info("                                      |---------内存在用监控系统开始运行!----------|");
	spdlog::info("                                      |------------------------------------------------------|");
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
            if (processList[i].first/1024 > 1000)
            {
                spdlog::info( "id:{2:d}, Process:{0:s}, Memory Usage:{1:d}  MB", processList[i].second,processList[i].first/1024, i);
            }            
        }

        float cpuTemp = getCpuTemperature();
        float cpuUsage = getCpuUsage();
        float memUsage = getMemoryUsage();

        if (cpuTemp >= 0.0f) {
            std::cout << "CPU Temperature: " << cpuTemp << " °C" << std::endl;
        }

        if (cpuUsage >= 0.0f) {
            std::cout << "CPU Usage: " << cpuUsage << " %" << std::endl;
        }

        if (memUsage >= 0.0f) {
            std::cout << "Memory Usage: " << memUsage << " %" << std::endl;
        }
        spdlog::info( "------CPU Temperature: {0:f}°C, CPU Usage: {1:f}%, Memory Usage:{2:f}%", cpuTemp,cpuUsage, memUsage);
         spdlog::info("---------------------------------");
        std::cout << "---------------------------------" << std::endl;
        sleep(1);
    }

    return 0;
}
