# Tests CMakeLists.txt - Comprehensive test organization
# All tests are now organized in the tests directory

# ============================================================================
# UNIT TESTS (Google Test Framework)
# ============================================================================

# Add test source files
set(TEST_SOURCES
    ../config/tests/poco_config_manager_test.cpp
    ../config/tests/poco_config_adapter_test.cpp
    ../config/tests/test_config_persistence.cpp
    test_server_config_observer.cpp
    auth_test.cpp
    database_manager_test.cpp
    file_processor_test.cpp
    file_utils_test.cpp
    media_processing_orchestrator_test.cpp
    media_processor_test.cpp
    status_test.cpp
    test_env_setup.cpp
)

# Add source files for dedup_tests
set(DEDUP_TESTS_SOURCES
    ../config/src/poco_config_manager.cpp
    ../config/src/poco_config_adapter.cpp
    ../src/core/server_config_observer.cpp
    ../src/media_processor.cpp
    ../src/file_utils.cpp
    ../src/auth.cpp
    ../src/duplicate_linker.cpp
    ../src/core/memory_pool.cpp
    ../src/database/database_manager.cpp
    ../src/file_processor.cpp
    ../src/file_scanner.cpp
    ../src/database/db_performance_logger.cpp
    ../src/mount_manager.cpp
    ../src/transcoding_manager.cpp
    ../src/media_processing_orchestrator.cpp
    ../src/thread_pool_manager.cpp
    ../src/simple_scheduler.cpp
    ../src/singleton_manager.cpp
    ../src/cache/decoder_cache.cpp
    ../src/decoder/media_decoder.cpp
    ${TEST_SOURCES}
)

# Main unit test executable
add_executable(dedup_tests
    ${DEDUP_TESTS_SOURCES}
)

target_include_directories(dedup_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/config/include
    $<TARGET_PROPERTY:spdlog::spdlog,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(dedup_tests PRIVATE
    GTest::gtest_main
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    PkgConfig::SQLITE3
    PkgConfig::OpenCV
    ${FFMPEG_LIBRARIES}
    PkgConfig::LIBRAW
    yaml-cpp
    TBB::tbb
    Poco::Util
    Poco::JSON
    Poco::Foundation
)

include(GoogleTest)
gtest_discover_tests(dedup_tests)

# Media Processing Orchestrator unit test
add_executable(media_processing_orchestrator_test
    media_processing_orchestrator_test.cpp
    ../src/database/database_manager.cpp
    ../src/media_processing_orchestrator.cpp
    ../src/media_processor.cpp
    ../src/core/memory_pool.cpp
    ../src/file_utils.cpp
    ../src/database/db_performance_logger.cpp
    ../src/mount_manager.cpp
    ../src/duplicate_linker.cpp
    ../src/transcoding_manager.cpp
    ../config/src/poco_config_adapter.cpp
    ../config/src/poco_config_manager.cpp
)

target_link_libraries(media_processing_orchestrator_test
    PRIVATE
    gtest
    gtest_main
    pthread
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    PkgConfig::SQLITE3
    PkgConfig::OpenCV
    ${FFMPEG_LIBRARIES}
    PkgConfig::LIBRAW
    yaml-cpp
    TBB::tbb
    Poco::Util
    Poco::JSON
    Poco::Foundation
)

add_test(NAME MediaProcessingOrchestratorTest COMMAND media_processing_orchestrator_test)

# ============================================================================
# INTEGRATION TESTS (Standalone executables)
# ============================================================================

# Cache management tests
add_executable(cache_size_test
    integration/cache_size_test.cpp
    ../src/transcoding_manager.cpp
    ../src/media_processor.cpp
    ../src/core/memory_pool.cpp
    ../src/mount_manager.cpp
    ../src/duplicate_linker.cpp
    ../src/database/database_manager.cpp
    ../src/file_processor.cpp
    ../src/file_utils.cpp
    ../src/database/db_performance_logger.cpp
    ../config/src/poco_config_adapter.cpp
    ../config/src/poco_config_manager.cpp
)

target_link_libraries(cache_size_test PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    PkgConfig::SQLITE3
    PkgConfig::OpenCV
    ${FFMPEG_LIBRARIES}
    PkgConfig::LIBRAW
    yaml-cpp
    TBB::tbb
    Poco::Util
    Poco::JSON
    Poco::Foundation
)

add_executable(smart_cache_cleanup_test
    integration/smart_cache_cleanup_test.cpp
    ../src/transcoding_manager.cpp
    ../src/media_processor.cpp
    ../src/core/memory_pool.cpp
    ../src/mount_manager.cpp
    ../src/duplicate_linker.cpp
    ../src/database/database_manager.cpp
    ../src/file_utils.cpp
    ../src/database/db_performance_logger.cpp
    ../config/src/poco_config_adapter.cpp
    ../config/src/poco_config_manager.cpp
)

target_link_libraries(smart_cache_cleanup_test PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    PkgConfig::SQLITE3
    PkgConfig::OpenCV
    ${FFMPEG_LIBRARIES}
    PkgConfig::LIBRAW
    yaml-cpp
    TBB::tbb
    Poco::Util
    Poco::JSON
    Poco::Foundation
)

# File processing tests
add_executable(raw_file_test
    integration/raw_file_test.cpp
    ../src/transcoding_manager.cpp
    ../src/media_processor.cpp
    ../src/core/memory_pool.cpp
    ../src/mount_manager.cpp
    ../src/duplicate_linker.cpp
    ../src/database/database_manager.cpp
    ../src/file_utils.cpp
    ../src/database/db_performance_logger.cpp
    ../config/src/poco_config_adapter.cpp
    ../config/src/poco_config_manager.cpp
)

target_link_libraries(raw_file_test PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    PkgConfig::SQLITE3
    PkgConfig::OpenCV
    ${FFMPEG_LIBRARIES}
    PkgConfig::LIBRAW
    yaml-cpp
    TBB::tbb
    Poco::Util
    Poco::JSON
    Poco::Foundation
)

add_executable(file_type_config_test
    ../config/tests/file_type_config_test.cpp
    ../config/src/poco_config_adapter.cpp
    ../config/src/poco_config_manager.cpp
)

target_link_libraries(file_type_config_test PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    PkgConfig::SQLITE3
    PkgConfig::OpenCV
    ${FFMPEG_LIBRARIES}
    PkgConfig::LIBRAW
    yaml-cpp
    TBB::tbb
    Poco::Util
    Poco::JSON
    Poco::Foundation
)

# Media processing tests
add_executable(media_processor_example
    integration/media_processor_example.cpp
    ../src/media_processor.cpp
    ../src/core/memory_pool.cpp
    ../src/mount_manager.cpp
    ../config/src/poco_config_adapter.cpp
    ../config/src/poco_config_manager.cpp
)

target_link_libraries(media_processor_example PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    PkgConfig::SQLITE3
    PkgConfig::OpenCV
    ${FFMPEG_LIBRARIES}
    PkgConfig::LIBRAW
    yaml-cpp
    TBB::tbb
    Poco::Util
    Poco::JSON
    Poco::Foundation
)

add_executable(file_processor_example
    integration/file_processor_example.cpp
    ../src/file_processor.cpp
    ../src/transcoding_manager.cpp
    ../src/media_processor.cpp
    ../src/core/memory_pool.cpp
    ../src/mount_manager.cpp
    ../src/duplicate_linker.cpp
    ../src/database/database_manager.cpp
    ../src/database/db_performance_logger.cpp
    ../src/file_utils.cpp
    ../config/src/poco_config_adapter.cpp
    ../config/src/poco_config_manager.cpp
)

target_link_libraries(file_processor_example PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    PkgConfig::SQLITE3
    PkgConfig::OpenCV
    ${FFMPEG_LIBRARIES}
    PkgConfig::LIBRAW
    yaml-cpp
    TBB::tbb
    Poco::Util
    Poco::JSON
    Poco::Foundation
)

# Utility tests
add_executable(file_utils_example
    integration/file_utils_example.cpp
    ../src/file_utils.cpp
)

add_executable(debug_config
    ../config/tests/debug_config.cpp
    ../config/src/poco_config_adapter.cpp
    ../config/src/poco_config_manager.cpp
)

target_link_libraries(debug_config PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    PkgConfig::SQLITE3
    PkgConfig::OpenCV
    ${FFMPEG_LIBRARIES}
    PkgConfig::LIBRAW
    yaml-cpp
    TBB::tbb
    Poco::Util
    Poco::JSON
    Poco::Foundation
)

add_executable(test_mode_change
    integration/test_mode_change.cpp
    ../config/src/poco_config_adapter.cpp
    ../config/src/poco_config_manager.cpp
)

target_link_libraries(test_mode_change PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    PkgConfig::SQLITE3
    PkgConfig::OpenCV
    ${FFMPEG_LIBRARIES}
    PkgConfig::LIBRAW
    yaml-cpp
    TBB::tbb
    Poco::Util
    Poco::JSON
    Poco::Foundation
)

add_executable(migrate_to_relative_paths
    integration/migrate_to_relative_paths.cpp
    ../src/file_utils.cpp
    ../src/mount_manager.cpp
)

# ============================================================================
# LIBRAW AND FORMAT TESTS
# ============================================================================

# LibRaw probe tool
add_executable(libraw_probe
    libraw/libraw_probe.cpp
)

# Raw to JPEG conversion test
add_executable(raw_to_jpeg
    libraw/raw_to_jpeg.cpp
)

# ============================================================================
# COMMON TARGET CONFIGURATIONS
# ============================================================================

# Common include directories for all integration tests
target_include_directories(cache_size_test PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/config/include)
target_include_directories(smart_cache_cleanup_test PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/config/include)
target_include_directories(raw_file_test PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/config/include)
target_include_directories(file_type_config_test PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/config/include)
target_include_directories(media_processor_example PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/config/include)
target_include_directories(file_processor_example PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/config/include)
target_include_directories(file_utils_example PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/config/include)
target_include_directories(debug_config PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/config/include)
target_include_directories(test_mode_change PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/config/include)
target_include_directories(migrate_to_relative_paths PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/config/include)
target_include_directories(libraw_probe PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/config/include)
target_include_directories(raw_to_jpeg PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/config/include)

# Common link libraries for integration tests
target_link_libraries(cache_size_test
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    pthread
    PkgConfig::SQLITE3
    TBB::tbb
    PkgConfig::OpenCV
    ${FFMPEG_LIBRARIES}
    $<$<BOOL:${VIPS_FOUND}>:PkgConfig::VIPS>
    $<$<BOOL:${ONNXRUNTIME_FOUND}>:PkgConfig::ONNXRUNTIME>
    PkgConfig::LIBRAW
    yaml-cpp
)

target_link_libraries(smart_cache_cleanup_test
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    pthread
    PkgConfig::SQLITE3
    TBB::tbb
    PkgConfig::OpenCV
    ${FFMPEG_LIBRARIES}
    $<$<BOOL:${VIPS_FOUND}>:PkgConfig::VIPS>
    $<$<BOOL:${ONNXRUNTIME_FOUND}>:PkgConfig::ONNXRUNTIME>
    PkgConfig::LIBRAW
    yaml-cpp
)

target_link_libraries(raw_file_test
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    pthread
    PkgConfig::SQLITE3
    TBB::tbb
    PkgConfig::OpenCV
    ${FFMPEG_LIBRARIES}
    $<$<BOOL:${VIPS_FOUND}>:PkgConfig::VIPS>
    $<$<BOOL:${ONNXRUNTIME_FOUND}>:PkgConfig::ONNXRUNTIME>
    PkgConfig::LIBRAW
    yaml-cpp
)

target_link_libraries(file_type_config_test
    PRIVATE
    spdlog::spdlog
    yaml-cpp
    PkgConfig::SQLITE3
    pthread
)

target_link_libraries(media_processor_example
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    pthread
    PkgConfig::SQLITE3
    TBB::tbb
    PkgConfig::OpenCV
    ${FFMPEG_LIBRARIES}
    $<$<BOOL:${VIPS_FOUND}>:PkgConfig::VIPS>
    $<$<BOOL:${ONNXRUNTIME_FOUND}>:PkgConfig::ONNXRUNTIME>
    yaml-cpp
)

target_link_libraries(file_processor_example
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    pthread
    PkgConfig::SQLITE3
    TBB::tbb
    PkgConfig::OpenCV
    ${FFMPEG_LIBRARIES}
    $<$<BOOL:${VIPS_FOUND}>:PkgConfig::VIPS>
    $<$<BOOL:${ONNXRUNTIME_FOUND}>:PkgConfig::ONNXRUNTIME>
    PkgConfig::LIBRAW
    yaml-cpp
)

target_link_libraries(file_utils_example
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    spdlog::spdlog
    pthread
)

target_link_libraries(debug_config
    PRIVATE
    spdlog::spdlog
    yaml-cpp
    PkgConfig::SQLITE3
    pthread
)

target_link_libraries(test_mode_change
    PRIVATE
    spdlog::spdlog
    yaml-cpp
    PkgConfig::SQLITE3
    pthread
)

target_link_libraries(migrate_to_relative_paths
    PRIVATE
    spdlog::spdlog
    yaml-cpp
    PkgConfig::SQLITE3
    pthread
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_link_libraries(libraw_probe
    PRIVATE
    PkgConfig::LIBRAW
)

target_link_libraries(raw_to_jpeg
    PRIVATE
    PkgConfig::LIBRAW
    PkgConfig::OpenCV
) 