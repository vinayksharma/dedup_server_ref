#include "core/singleton_manager.hpp"
#include "logging/logger.hpp"
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>

// Static member variables
std::string SingletonManager::pid_file_path;
std::ofstream SingletonManager::pid_file;
bool SingletonManager::is_running = false;

SingletonManager::SingletonManager()
{
    Logger::info("SingletonManager constructor called");
}

SingletonManager::~SingletonManager()
{
    removePidFile();
    Logger::info("SingletonManager destructor called");
}

SingletonManager &SingletonManager::getInstance()
{
    static std::unique_ptr<SingletonManager> instance;
    if (!instance)
    {
        instance = std::make_unique<SingletonManager>();
    }
    return *instance;
}

void SingletonManager::initialize(const std::string &pid_file_path_param)
{
    pid_file_path = pid_file_path_param;
    Logger::info("SingletonManager initialized with PID file: " + pid_file_path);
}

bool SingletonManager::isAnotherInstanceRunning()
{
    if (pid_file_path.empty())
    {
        return false;
    }

    // Check if PID file exists
    std::ifstream file(pid_file_path);
    if (!file.is_open())
    {
        return false;
    }

    // Read PID from file
    pid_t pid;
    file >> pid;
    file.close();

    if (pid <= 0)
    {
        // Invalid PID, remove the file
        unlink(pid_file_path.c_str());
        return false;
    }

    // Check if process is actually running
    if (kill(pid, 0) != 0)
    {
        // Process is not running, remove stale PID file
        unlink(pid_file_path.c_str());
        return false;
    }

    // Check if it's our own PID (shouldn't happen, but just in case)
    if (pid == getpid())
    {
        return false;
    }

    return true;
}

bool SingletonManager::createPidFile()
{
    if (pid_file_path.empty())
    {
        return false;
    }

    // Try to create PID file with exclusive access
    int fd = open(pid_file_path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd == -1)
    {
        // File already exists or cannot be created
        return false;
    }

    // Write current PID to file
    pid_t current_pid = getpid();
    std::string pid_str = std::to_string(current_pid) + "\n";
    write(fd, pid_str.c_str(), pid_str.length());
    close(fd);

    // Open the file for the ofstream to use
    pid_file.open(pid_file_path, std::ios::out | std::ios::app);
    if (!pid_file.is_open())
    {
        unlink(pid_file_path.c_str());
        return false;
    }

    is_running = true;

    // NOTE: Signal handlers are now managed by main.cpp for coordinated shutdown
    // No signal handlers set up here to avoid conflicts

    return true;
}

void SingletonManager::removePidFile()
{
    if (is_running && pid_file.is_open())
    {
        pid_file.close();
        unlink(pid_file_path.c_str());
        is_running = false;
    }
}

bool SingletonManager::shutdownExistingInstance()
{
    if (!isAnotherInstanceRunning())
    {
        return true; // No instance running
    }

    pid_t existing_pid = getPidFromFile();
    if (existing_pid <= 0)
    {
        // Invalid PID in file, remove the stale PID file
        Logger::info("Invalid PID in file, removing stale PID file...");
        unlink(pid_file_path.c_str());
        return true;
    }

    // Check if process is actually running
    if (kill(existing_pid, 0) != 0)
    {
        // Process is not running, remove stale PID file
        Logger::info("Process " + std::to_string(existing_pid) + " is not running, removing stale PID file...");
        unlink(pid_file_path.c_str());
        return true;
    }

    // Send SIGTERM to existing process
    if (kill(existing_pid, SIGTERM) == 0)
    {
        Logger::info("Sent shutdown signal to existing instance (PID: " + std::to_string(existing_pid) + ")");

        // Wait a bit for graceful shutdown
        sleep(2);

        // Check if process is still running
        if (kill(existing_pid, 0) == 0)
        {
            Logger::info("Existing instance still running, sending SIGKILL...");
            kill(existing_pid, SIGKILL);
            sleep(1);
        }

        return true;
    }

    return false;
}

bool SingletonManager::isPidFileValid()
{
    if (pid_file_path.empty())
    {
        return false;
    }

    std::ifstream file(pid_file_path);
    if (!file.is_open())
    {
        return false;
    }

    pid_t pid;
    file >> pid;
    file.close();

    if (pid <= 0)
    {
        return false;
    }

    // Check if process is still running
    return (kill(pid, 0) == 0);
}

pid_t SingletonManager::getPidFromFile()
{
    if (pid_file_path.empty())
    {
        return -1;
    }

    std::ifstream file(pid_file_path);
    if (!file.is_open())
    {
        return -1;
    }

    pid_t pid;
    file >> pid;
    file.close();

    return pid;
}

void SingletonManager::signalHandler(int signal)
{
    Logger::info("Received signal " + std::to_string(signal) + ", shutting down gracefully...");

    // CRITICAL: Clean up PID file immediately to prevent stale PID issues
    try
    {
        Logger::info("Cleaning up PID file due to signal " + std::to_string(signal));
        SingletonManager::getInstance().removePidFile();
        Logger::info("PID file cleanup completed");
    }
    catch (...)
    {
        // Ignore any exceptions during cleanup
        Logger::warn("Error during PID file cleanup in signal handler");
    }

    // Flush any pending output
    std::cout.flush();
    std::cerr.flush();

    // Exit cleanly - this ensures PID file is removed even if main() cleanup doesn't run
    Logger::info("Signal handler cleanup complete, exiting...");
    _exit(0);
}

void SingletonManager::cleanup()
{
    // Clean up PID file if still running
    if (is_running)
    {
        removePidFile();
    }
}
