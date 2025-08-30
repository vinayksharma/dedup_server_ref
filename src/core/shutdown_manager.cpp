#include "core/shutdown_manager.hpp"
#include "logging/logger.hpp"
#include <atomic>
#include <csignal>
#include <unistd.h>

volatile sig_atomic_t ShutdownManager::signal_flag_ = 0;
volatile sig_atomic_t ShutdownManager::signal_num_ = 0;

ShutdownManager &ShutdownManager::getInstance()
{
    static ShutdownManager instance;
    return instance;
}

ShutdownManager::~ShutdownManager()
{
    stopWatcher();
}

void ShutdownManager::installSignalHandlers()
{
    // Reset to defaults to avoid conflicts then install our handler
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);

    signal(SIGINT, &ShutdownManager::handleSignal);
    signal(SIGTERM, &ShutdownManager::handleSignal);
    signal(SIGQUIT, &ShutdownManager::handleSignal);

    startWatcher();
    Logger::info("ShutdownManager: signal handlers installed");
}

void ShutdownManager::handleSignal(int sig) noexcept
{
    // Set the static signal flags - these are safe to access from signal handler
    signal_num_ = sig;
    signal_flag_ = 1;
}

void ShutdownManager::startWatcher()
{
    if (watcher_running_.exchange(true))
    {
        return;
    }
    watcher_ = std::thread([this]()
                           {
        for (;;)
        {
            if (!watcher_running_.load())
            {
                break;
            }

            if (signal_flag_)
            {
                // Capture and clear asap
                int sig = signal_num_;
                signal_flag_ = 0;
                last_signal_.store(sig);
                requestShutdown("Signal received", sig);
            }

            if (shutdown_requested_.load())
            {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        } });
}

void ShutdownManager::stopWatcher()
{
    if (!watcher_running_.exchange(false))
    {
        return;
    }
    if (watcher_.joinable())
    {
        watcher_.join();
    }
}

void ShutdownManager::requestShutdown(const std::string &reason, int signal_number) noexcept
{
    if (shutdown_in_progress_.exchange(true))
    {
        // Already in progress; fast exit if we are in a signal path
        return;
    }

    last_signal_.store(signal_number);
    {
        std::lock_guard<std::mutex> lk(mutex_);
        reason_ = reason;
        shutdown_requested_.store(true);
    }
    cv_.notify_all();

    // Hint: Stop watcher loop so it can exit
    watcher_running_.store(false);

    if (signal_number != 0)
    {
        Logger::info("ShutdownManager: received signal " + std::to_string(signal_number) + ", initiating graceful shutdown");
    }
    else
    {
        Logger::info("ShutdownManager: programmatic shutdown requested - " + reason);
    }
}

void ShutdownManager::waitForShutdown()
{
    std::unique_lock<std::mutex> lk(mutex_);
    cv_.wait(lk, [this]
             { return shutdown_requested_.load(); });
}

std::string ShutdownManager::getReason() const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return reason_;
}

void ShutdownManager::reset() noexcept
{
    // Stop any existing watcher
    stopWatcher();

    // Reset all state
    shutdown_requested_.store(false);
    shutdown_in_progress_.store(false);
    last_signal_.store(0);
    watcher_running_.store(false);

    // Clear static signal flags
    signal_flag_ = 0;
    signal_num_ = 0;

    // Clear reason
    {
        std::lock_guard<std::mutex> lk(mutex_);
        reason_.clear();
    }
}
