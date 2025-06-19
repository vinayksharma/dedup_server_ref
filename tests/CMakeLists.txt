add_executable(dedup_tests
    status_test.cpp
    auth_test.cpp
    ../src/status.cpp
    ../src/auth.cpp
)

target_include_directories(dedup_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dedup_tests PRIVATE
    GTest::gtest_main
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

include(GoogleTest)
gtest_discover_tests(dedup_tests) 