add_executable(dedup_tests
    status_test.cpp
    auth_test.cpp
    file_utils_test.cpp
    media_processor_test.cpp
    ../src/auth.cpp
    ../src/file_utils.cpp
    ../src/media_processor.cpp
)

target_include_directories(dedup_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    $<TARGET_PROPERTY:spdlog::spdlog,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(dedup_tests PRIVATE
    GTest::gtest_main
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

include(GoogleTest)
gtest_discover_tests(dedup_tests) 