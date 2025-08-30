#pragma once

#include <atomic>
#include <condition_variable>
#include <csignal>
#include <mutex>
#include <string>
#include <thread>

/**
 * Centralized shutdown manager.
 * - Installs async-signal-safe handlers for SIGINT/SIGTERM/SIGQUIT
 * - Exposes a single observable state for shutdown across the process
 * - Provides a blocking wait until shutdown is requested
 */
class ShutdownManager
{
public:
    static ShutdownManager &getInstance();

    // Install signal handlers and start internal watcher thread
    void installSignalHandlers();

    // Programmatically request shutdown (safe to call from any thread, not from a signal handler)
    void requestShutdown(const std::string &reason, int signal_number = 0) noexcept;

    // Query shutdown state
    bool isShutdownRequested() const noexcept { return shutdown_requested_.load(); }

    // Block until shutdown has been requested
    void waitForShutdown();

    // Metadata about shutdown
    int getSignalNumber() const noexcept { return last_signal_.load(); }
    std::string getReason() const;

    // Reset state for testing purposes
    void reset() noexcept;

private:
    ShutdownManager() = default;
    ~ShutdownManager();
    ShutdownManager(const ShutdownManager &) = delete;
    ShutdownManager &operator=(const ShutdownManager &) = delete;

    // Async-signal-safe handler (sets only sig_atomic_t flags)
    static void handleSignal(int sig) noexcept;

    // Background watcher to translate signal flags into a proper shutdown request
    void startWatcher();
    void stopWatcher();

    // Internal state
    std::atomic<bool> shutdown_requested_{false};
    std::atomic<bool> shutdown_in_progress_{false};
    std::atomic<int> last_signal_{0};
    std::string reason_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;

    // Watcher thread
    std::thread watcher_;
    std::atomic<bool> watcher_running_{false};

    // Async-signal-safe flags
    static volatile sig_atomic_t signal_flag_;
    static volatile sig_atomic_t signal_num_;
};
