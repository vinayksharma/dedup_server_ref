cmake_minimum_required(VERSION 3.10)
project(dedup_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

# TODO: DEDUP MODES - Libraries for different deduplication modes
# FAST MODE: OpenCV (dHash) + FFmpeg
# BALANCED MODE: libvips + OpenCV (pHash) + FFmpeg  
# QUALITY MODE: ONNX Runtime + CNN Embeddings (ResNet/CLIP) + FFmpeg

# OpenCV - Required for FAST and BALANCED modes
find_package(OpenCV REQUIRED)

# FFmpeg - Required for video processing in all modes
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
)

# libvips - Required for BALANCED mode (image processing)
# TODO: Install with: brew install vips (macOS) or apt-get install libvips-dev (Ubuntu)
find_package(PkgConfig REQUIRED)
pkg_check_modules(VIPS IMPORTED_TARGET libvips)

# ONNX Runtime - Required for QUALITY mode (CNN embeddings)
# TODO: Install with: brew install onnxruntime (macOS) or download from ONNX website
find_package(PkgConfig REQUIRED)
pkg_check_modules(ONNXRUNTIME IMPORTED_TARGET onnxruntime)

# Add include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/auth
    ${CMAKE_SOURCE_DIR}/include/web
    ${CMAKE_SOURCE_DIR}/include/logging
    ${CMAKE_SOURCE_DIR}/include/third_party
    ${OPENSSL_INCLUDE_DIR}
)

# Add source files
set(SOURCES
    src/main.cpp
    src/auth.cpp
    src/file_utils.cpp
)

# Add header files
set(HEADERS
    include/auth/auth.hpp
    include/auth/auth_middleware.hpp
    include/core/status.hpp
    include/core/file_utils.hpp
    include/core/dedup_modes.hpp
    include/logging/logger.hpp
    include/web/openapi_docs.hpp
    include/web/route_handlers.hpp
    include/core/server_config.hpp
)

# Create executable
add_executable(dedup_server ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(dedup_server
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    pthread
    # TODO: DEDUP MODES - Link libraries for different modes
    # FAST MODE: OpenCV + FFmpeg
    # BALANCED MODE: libvips + OpenCV + FFmpeg
    # QUALITY MODE: ONNX Runtime + FFmpeg
    ${OpenCV_LIBS}
    PkgConfig::FFMPEG
    $<$<BOOL:${VIPS_FOUND}>:PkgConfig::VIPS>
    $<$<BOOL:${ONNXRUNTIME_FOUND}>:PkgConfig::ONNXRUNTIME>
)

# Add Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

# Add httplib
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.1
)
FetchContent_MakeAvailable(httplib)

# Add spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# Tests
enable_testing()
add_subdirectory(tests) 