#include "core/file_processor.hpp"
#include "core/media_processing_orchestrator.hpp"
#include "core/thread_pool_manager.hpp"
#include "core/scan_thread_pool_manager.hpp"
#include "core/database_connection_pool.hpp"
#include "core/file_scanner.hpp"
#include "core/transcoding_manager.hpp"
#include "core/decoder/media_decoder.hpp"
#include "web/route_handlers.hpp"
#include "auth/auth.hpp"
#include "logging/logger.hpp"
#include "database/database_manager.hpp"
#include "config_observer.hpp"
#include "server_config.hpp"
#include "poco_config_adapter.hpp"
#include "core/simple_scheduler.hpp"
#include "web/openapi_docs.hpp"
#include "core/status.hpp"
#include "core/singleton_manager.hpp"
#include "core/duplicate_linker.hpp"
#include "core/resource_monitor.hpp"
#include "core/crash_recovery.hpp"
#include "core/logger_observer.hpp"
#include "core/server_config_observer.hpp"
#include "core/scan_config_observer.hpp"
#include "core/threading_config_observer.hpp"
#include "core/database_config_observer.hpp"
#include "core/file_type_config_observer.hpp"
#include "core/video_processing_config_observer.hpp"
#include "core/cache_config_observer.hpp"
#include "core/processing_config_observer.hpp"
#include "core/dedup_mode_config_observer.hpp"
#include "core/http_server_manager.hpp"
#include <httplib.h>
#include <iostream>
#include <memory>
#include <signal.h>
#include <atomic>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/mutex.h>
#include <tbb/task_arena.h>
#include "core/shutdown_manager.hpp"

// Global variables for HTTP server pointer (optional)
static httplib::Server *g_server = nullptr;

int main(int argc, char *argv[])
{
    // Initialize coordinated signal handling FIRST
    ShutdownManager::getInstance().installSignalHandlers();

    // Initialize singleton manager with PID file
    SingletonManager::initialize("dedup_server.pid");
    auto &singleton_manager = SingletonManager::getInstance();

    // Check for command line arguments
    bool force_shutdown = false;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--shutdown" || arg == "-s")
        {
            force_shutdown = true;
        }
        else if (arg == "--help" || arg == "-h")
        {
            std::cout << "Dedup Server - Single Instance Manager" << std::endl;
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --shutdown, -s    Shutdown existing instance and start new one" << std::endl;
            std::cout << "  --help, -h        Show this help message" << std::endl;
            return 0;
        }
    }

    // Check if another instance is running
    if (singleton_manager.isAnotherInstanceRunning())
    {
        if (force_shutdown)
        {
            Logger::info("Existing instance detected. Attempting to shutdown...");
            if (singleton_manager.shutdownExistingInstance())
            {
                Logger::info("Existing instance shutdown successful.");
                sleep(1); // Give it time to fully shutdown
            }
            else
            {
                Logger::error("Failed to shutdown existing instance.");
                return 1;
            }
        }
        else
        {
            std::cout << "Error: Another instance is already running!" << std::endl;
            std::cout << "Use --shutdown or -s to force shutdown the existing instance." << std::endl;
            std::cout << "Use --help or -h for more options." << std::endl;
            return 1;
        }
    }

    // Initialize crash recovery early so we can capture stack traces on faults
    CrashRecovery::initialize();

    // Try to create PID file (this will fail if another instance is running)
    if (!singleton_manager.createPidFile())
    {
        Logger::error("Failed to create PID file. Another instance may be running.");
        return 1;
    }

    Logger::info("Starting dedup server (PID: " + std::to_string(getpid()) + ")...");

    // Initialize configuration manager
    auto &config_manager = PocoConfigAdapter::getInstance();

    // Initialize logger with configured log level
    Logger::init(config_manager.getLogLevel());

    // Create and register configuration observers
    auto logger_observer = std::make_unique<LoggerObserver>();
    auto server_config_observer = std::make_unique<ServerConfigObserver>();
    auto scan_config_observer = std::make_unique<ScanConfigObserver>();
    auto threading_config_observer = std::make_unique<ThreadingConfigObserver>();
    auto database_config_observer = std::make_unique<DatabaseConfigObserver>();
    auto file_type_config_observer = std::make_unique<FileTypeConfigObserver>();
    auto video_processing_config_observer = std::make_unique<VideoProcessingConfigObserver>();
    auto cache_config_observer = std::make_unique<CacheConfigObserver>();
    auto processing_config_observer = std::make_unique<ProcessingConfigObserver>();
    auto dedup_mode_config_observer = std::make_unique<DedupModeConfigObserver>();

    // Subscribe observers to configuration changes
    config_manager.subscribe(logger_observer.get());
    config_manager.subscribe(server_config_observer.get());
    config_manager.subscribe(scan_config_observer.get());
    config_manager.subscribe(threading_config_observer.get());
    config_manager.subscribe(database_config_observer.get());
    config_manager.subscribe(file_type_config_observer.get());
    config_manager.subscribe(video_processing_config_observer.get());
    config_manager.subscribe(cache_config_observer.get());
    config_manager.subscribe(processing_config_observer.get());
    config_manager.subscribe(dedup_mode_config_observer.get());

    // Initialize and start the simple scheduler
    auto &scheduler = SimpleScheduler::getInstance();

    // Subscribe the scheduler to configuration changes for real-time interval updates
    config_manager.subscribe(&scheduler);
    Logger::info("SimpleScheduler subscribed to configuration changes for real-time scan and processing interval updates");

    // Start watching configuration for runtime changes
    config_manager.startWatching("config/config.json", 2);

    // Initialize safety mechanisms for external library calls
    Logger::info("Initializing safety mechanisms...");

    // Basic safety mechanisms initialized (crash recovery enabled)
    Logger::info("Basic safety mechanisms initialized (crash recovery active)");

    // Initialize thread pool manager with configured thread count
    ThreadPoolManager::initialize(config_manager.getMaxProcessingThreads());

    // Initialize database connection pool with configured database thread count
    auto &db_connection_pool = DatabaseConnectionPool::getInstance();
    db_connection_pool.initialize(config_manager.getDatabaseThreads());
    Logger::info("Database connection pool initialized with " + std::to_string(config_manager.getDatabaseThreads()) + " connections");

    // Initialize scan thread pool manager with configured scan thread count
    auto &scan_thread_manager = ScanThreadPoolManager::getInstance();
    scan_thread_manager.initialize(config_manager.getMaxScanThreads());
    Logger::info("Scan thread pool manager initialized with " + std::to_string(config_manager.getMaxScanThreads()) + " threads");

    // At the start of main, initialize the DatabaseManager singleton with default db path
    auto &db_manager = DatabaseManager::getInstance("scan_results.db");

    // Reset all processing flags from -1 (in progress) to 0 (not processed) on startup
    // This ensures a clean state when the server restarts
    Logger::info("Resetting processing flags on startup...");
    auto reset_result = db_manager.resetAllProcessingFlagsOnStartup();
    if (!reset_result.success)
    {
        Logger::warn("Warning: Failed to reset processing flags: " + reset_result.error_message);
    }
    else
    {
        Logger::info("Successfully reset all processing flags on startup");
    }

    // Initialize transcoding manager
    auto &transcoding_manager = TranscodingManager::getInstance();
    transcoding_manager.setDatabaseManager(&db_manager);
    transcoding_manager.initialize("./cache", config_manager.getMaxProcessingThreads());

    // Reset all transcoding job statuses from 1 (in progress) to 0 (queued) on startup
    // This ensures a clean state when the server restarts
    Logger::info("Resetting transcoding job statuses on startup...");
    transcoding_manager.resetTranscodingJobStatusesOnStartup();

    // Restore transcoding queue from database on startup
    transcoding_manager.restoreQueueFromDatabase();

    // Start transcoding after queue restoration
    transcoding_manager.startTranscoding();

    // Start duplicate linker async process (wake on schedule and when new results land)
    DuplicateLinker::getInstance().start(db_manager, config_manager.getProcessingIntervalSeconds());

    // Subscribe the duplicate linker to configuration changes for real-time processing interval updates
    config_manager.subscribe(&DuplicateLinker::getInstance());
    Logger::info("DuplicateLinker subscribed to configuration changes for real-time processing interval updates");

    // Start the scheduler first to ensure it's ready
    scheduler.start();

    // Perform immediate scan on startup in a separate thread to avoid blocking
    Logger::info("Starting immediate scan on server startup in background thread...");
    std::thread startup_scan_thread([&]()
                                    {
        try
        {
            // Small delay to ensure server is fully started
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Get all stored scan paths from database
            auto &db_manager = DatabaseManager::getInstance();
            auto scan_paths = db_manager.getUserInputs("scan_path");

            if (scan_paths.empty())
            {
                Logger::warn("No scan paths configured, using default: /tmp");
                scan_paths.push_back("/tmp");
            }

            Logger::info("Found " + std::to_string(scan_paths.size()) + " scan paths for immediate scan");

            // Get configured scan thread limit
            auto &config_manager = PocoConfigAdapter::getInstance();
            int max_scan_threads = config_manager.getMaxScanThreads();

            Logger::info("Starting immediate parallel scan with " + std::to_string(max_scan_threads) + " threads for " +
                         std::to_string(scan_paths.size()) + " scan paths");

            // Thread-safe counters for progress tracking
            std::atomic<size_t> total_files_stored{0};
            std::atomic<size_t> successful_scans{0};
            std::atomic<size_t> failed_scans{0};

            // Thread-safe mutex for database operations to prevent race conditions
            tbb::mutex db_mutex;

            // Process scan paths in parallel using TBB with round-robin distribution
            tbb::parallel_for(tbb::blocked_range<size_t>(0, scan_paths.size()),
                              [&](const tbb::blocked_range<size_t> &range)
                              {
                // Round-robin distribution: each thread processes every Nth path
                // where N is the number of threads
                int max_scan_threads = config_manager.getMaxScanThreads();

                // Get thread ID for round-robin distribution
                size_t thread_id = tbb::this_task_arena::current_thread_index();
                if (thread_id >= max_scan_threads)
                {
                    thread_id = thread_id % max_scan_threads;
                }

                // Process paths assigned to this thread in round-robin fashion
                for (size_t i = thread_id; i < scan_paths.size(); i += max_scan_threads)
                {
                    const auto &scan_path = scan_paths[i];

                    try
                    {
                        Logger::info("Thread " + std::to_string(thread_id) +
                                     " scanning directory: " + scan_path);

                        // Create scanner instance for this thread
                        FileScanner scanner("scan_results.db");

                        // Lock database operations to prevent race conditions
                        tbb::mutex::scoped_lock lock(db_mutex);

                        // Perform the scan
                        size_t files_stored = scanner.scanDirectory(scan_path, true);

                        // Update counters
                        total_files_stored += files_stored;
                        successful_scans++;

                        Logger::info("Thread " + std::to_string(thread_id) +
                                     " completed scan for " + scan_path +
                                     " - Files stored: " + std::to_string(files_stored));
                    }
                    catch (const std::exception &e)
                    {
                        failed_scans++;
                        Logger::error("Thread " + std::to_string(thread_id) +
                                      " error scanning directory " + scan_path + ": " + std::string(e.what()));
                    }
                }
            });

            // Log final statistics for immediate scan
            Logger::info("Immediate startup scan completed - Total files stored: " + std::to_string(total_files_stored.load()) +
                         ", Successful scans: " + std::to_string(successful_scans.load()) +
                         ", Failed scans: " + std::to_string(failed_scans.load()));

            Logger::info("All immediate startup scans completed - Total files stored: " + std::to_string(total_files_stored.load()));

            // If files were found, trigger immediate processing
            if (total_files_stored.load() > 0)
            {
                Logger::info("Files found during startup scan, triggering immediate processing...");
                
                // Ensure TranscodingManager is fully ready before starting TBB processing
                Logger::info("Waiting for TranscodingManager to be fully ready...");
                std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // Wait 2 seconds
                
                try
                {
                    ThreadPoolManager::processAllScannedFilesAsync(
                        config_manager.getMaxProcessingThreads(),
                        // on_event callback
                        [](const FileProcessingEvent &event)
                        {
                            if (event.success)
                            {
                                Logger::info("Startup processing: " + event.file_path +
                                             " (format: " + event.artifact_format +
                                             ", confidence: " + std::to_string(event.artifact_confidence) + ")");
                            }
                            else
                            {
                                Logger::warn("Startup processing failed: " + event.file_path +
                                             " - " + event.error_message);
                            }
                        },
                        // on_error callback
                        [](const std::exception &e)
                        {
                            Logger::error("Startup processing error: " + std::string(e.what()));
                        },
                        // on_complete callback
                        []()
                        {
                            Logger::info("Startup processing completed");
                        });
                }
                catch (const std::exception &e)
                {
                    Logger::error("Error in startup processing: " + std::string(e.what()));
                }
            }
            else
            {
                Logger::info("No files found during startup scan, skipping immediate processing");
            }
        }
        catch (const std::exception &e)
        {
            Logger::error("Error in immediate startup scan: " + std::string(e.what()));
        } });

    // Detach the thread so it runs independently
    startup_scan_thread.detach();

    // Set up scan callback - scan all stored directories
    scheduler.setScanCallback([]()
                              {
        Logger::info("Executing scheduled scan operation");
        try {
            // Get all stored scan paths from database
            auto &db_manager = DatabaseManager::getInstance();
            auto scan_paths = db_manager.getUserInputs("scan_path");
            
            if (scan_paths.empty()) {
                Logger::warn("No scan paths configured, using default: /tmp");
                scan_paths.push_back("/tmp");
            }
            
            Logger::info("Found " + std::to_string(scan_paths.size()) + " scan paths to process");
            
            // Get configured scan thread limit
            auto &config_manager = PocoConfigAdapter::getInstance();
            int max_scan_threads = config_manager.getMaxScanThreads();
            
            Logger::info("Starting parallel scan with " + std::to_string(max_scan_threads) + " threads for " + 
                        std::to_string(scan_paths.size()) + " scan paths");
            
            // Thread-safe counters for progress tracking
            std::atomic<size_t> total_files_stored{0};
            std::atomic<size_t> successful_scans{0};
            std::atomic<size_t> failed_scans{0};
            
            // Thread-safe mutex for database operations to prevent race conditions
            tbb::mutex db_mutex;
            
            // Process scan paths in parallel using TBB with round-robin distribution
            // This ensures equal prioritization when there are more paths than threads
            tbb::parallel_for(tbb::blocked_range<size_t>(0, scan_paths.size()),
                [&](const tbb::blocked_range<size_t>& range) {
                    // Round-robin distribution: each thread processes every Nth path
                    // where N is the number of threads
                    auto& config_manager = PocoConfigAdapter::getInstance();
                    int max_scan_threads = config_manager.getMaxScanThreads();
                    
                    // Get thread ID for round-robin distribution
                    size_t thread_id = tbb::this_task_arena::current_thread_index();
                    if (thread_id >= max_scan_threads) {
                        thread_id = thread_id % max_scan_threads;
                    }
                    
                    // Process paths assigned to this thread in round-robin fashion
                    for (size_t i = thread_id; i < scan_paths.size(); i += max_scan_threads) {
                        const auto& scan_path = scan_paths[i];
                        
                        try {
                            Logger::info("Thread " + std::to_string(thread_id) + 
                                       " scanning directory: " + scan_path);
                            
                            // Create scanner instance for this thread
                            FileScanner scanner("scan_results.db");
                            
                            // Lock database operations to prevent race conditions
                            tbb::mutex::scoped_lock lock(db_mutex);
                            
                            // Perform the scan
                            size_t files_stored = scanner.scanDirectory(scan_path, true);
                            
                            // Update counters
                            total_files_stored += files_stored;
                            successful_scans++;
                            
                            Logger::info("Thread " + std::to_string(thread_id) + 
                                       " completed scan for " + scan_path + 
                                       " - Files stored: " + std::to_string(files_stored));
                                           
                        } catch (const std::exception &e) {
                            failed_scans++;
                            Logger::error("Thread " + std::to_string(thread_id) + 
                                         " error scanning directory " + scan_path + ": " + std::string(e.what()));
                        }
                    }
                });
            
            // Log final statistics
            Logger::info("Parallel scanning completed - Total files stored: " + std::to_string(total_files_stored.load()) + 
                        ", Successful scans: " + std::to_string(successful_scans.load()) + 
                        ", Failed scans: " + std::to_string(failed_scans.load()));
            
            Logger::info("All scheduled scans completed - Total files stored: " + std::to_string(total_files_stored.load()));
        } catch (const std::exception &e) {
            Logger::error("Error in scheduled scan: " + std::string(e.what()));
        } });

    // Set up processing callback - process files that need processing
    scheduler.setProcessingCallback([&db_manager]()
                                    {
        Logger::info("Executing scheduled processing operation");
        try {
            // Process files that need processing using ThreadPoolManager
            auto &config_manager = PocoConfigAdapter::getInstance();
            ThreadPoolManager::processAllScannedFilesAsync(
                config_manager.getMaxProcessingThreads(),
                // on_event callback
                [](const FileProcessingEvent &event) {
                    if (event.success) {
                        Logger::info("Processed file: " + event.file_path + 
                                   " (format: " + event.artifact_format + 
                                   ", confidence: " + std::to_string(event.artifact_confidence) + ")");
                    } else {
                        Logger::warn("Failed to process file: " + event.file_path + 
                                   " - " + event.error_message);
                    }
                },
                // on_error callback
                [](const std::exception &e) {
                    Logger::error("Processing error: " + std::string(e.what()));
                },
                // on_complete callback
                []() {
                    Logger::info("Scheduled processing completed");
                });
        } catch (const std::exception &e) {
            Logger::error("Error in scheduled processing: " + std::string(e.what()));
        } });

    // Initialize and start the HTTP server manager
    auto &http_server_manager = HttpServerManager::getInstance();

    // Subscribe the HTTP server manager to configuration changes for real-time server updates
    config_manager.subscribe(&http_server_manager);
    Logger::info("HttpServerManager subscribed to configuration changes for real-time server configuration updates");

    // Start the HTTP server
    http_server_manager.start(config_manager.getServerHost(), config_manager.getServerPort());

    // Set global pointer for signal handling (for backward compatibility)
    g_server = http_server_manager.getServer();

    // Wait for shutdown signal via centralized manager
    ShutdownManager::getInstance().waitForShutdown();

    Logger::info("Shutdown requested, cleaning up...");

    // Cleanup sequence
    g_server = nullptr; // Clear global pointer

    // Stop all components in reverse order
    // Ensure DuplicateLinker worker thread is stopped and joined to avoid
    // std::terminate during static destruction
    DuplicateLinker::getInstance().stop();
    // Stop transcoding manager threads before tearing down DB and pools
    TranscodingManager::getInstance().shutdown();
    scheduler.stop();

    // Shutdown managers
    DatabaseManager::shutdown();
    ThreadPoolManager::shutdown();
    ScanThreadPoolManager::getInstance().shutdown();
    DatabaseConnectionPool::getInstance().shutdown();

    // Stop the HTTP server manager
    http_server_manager.stop();

    // Safety mechanisms shutdown
    Logger::info("Safety mechanisms shutdown complete");

    // Cleanup singleton manager (PID file, etc.)
    SingletonManager::cleanup();
    Logger::info("Server shutdown complete");
    return 0;
}