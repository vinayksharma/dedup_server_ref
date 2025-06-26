# Project Structure

This document describes the organized structure of the dedup-server project.

## Directory Organization

```
dedup-server/
├── include/
│   ├── auth/                    # Authentication & Authorization
│   │   ├── auth.hpp            # JWT token generation and verification
│   │   └── auth_middleware.hpp # HTTP authentication middleware
│   ├── core/                   # Core functionality
│   │   ├── status.hpp          # System status checking
│   │   ├── server_config.hpp   # Server configuration constants
│   │   └── file_utils.hpp      # File system utilities with reactive streams
│   ├── logging/                # Logging functionality
│   │   └── logger.hpp          # spdlog-based logging wrapper
│   ├── web/                    # Web/HTTP functionality
│   │   ├── openapi_docs.hpp    # OpenAPI/Swagger documentation
│   │   └── route_handlers.hpp  # HTTP route handlers
│   └── third_party/            # Third-party libraries
│       ├── httplib.h           # HTTP server library
│       ├── jwt-cpp/            # JWT library
│       └── picojson/           # JSON library
├── src/
│   ├── main.cpp               # Application entry point
│   ├── auth.cpp               # Authentication implementation
│   └── file_utils.cpp         # File utilities implementation
├── tests/                     # Unit tests
│   ├── auth_test.cpp          # Authentication tests
│   ├── status_test.cpp        # Status tests
│   ├── file_utils_test.cpp    # File utilities tests
│   └── CMakeLists.txt         # Test build configuration
├── examples/                  # Example code
│   └── file_utils_example.cpp # FileUtils usage example
├── build/                     # Build artifacts (generated)
├── CMakeLists.txt             # Main build configuration
└── README.md                  # Project documentation
```

## Module Responsibilities

### 🔐 **Auth Module** (`include/auth/`)

- **auth.hpp**: JWT token generation, verification, and user management
- **auth_middleware.hpp**: HTTP request authentication middleware

### 🏗️ **Core Module** (`include/core/`)

- **status.hpp**: System health and status checking functionality
- **server_config.hpp**: Server configuration constants and settings
- **file_utils.hpp**: File system utilities with reactive stream support

### 📝 **Logging Module** (`include/logging/`)

- **logger.hpp**: spdlog-based logging wrapper with consistent API

### 🌐 **Web Module** (`include/web/`)

- **route_handlers.hpp**: HTTP endpoint handlers and request processing
- **openapi_docs.hpp**: OpenAPI/Swagger documentation generation

### 📚 **Third-Party Module** (`include/third_party/`)

- External libraries and dependencies
- Kept separate to avoid conflicts with system libraries

## FileUtils - Reactive File System Operations

The `FileUtils` class provides reactive file system operations using a custom observable pattern:

### Features

- **Reactive Streams**: Uses a custom `SimpleObservable` implementation
- **Recursive Scanning**: Support for both recursive and non-recursive directory scanning
- **Error Handling**: Proper error propagation through the observable chain
- **Thread Safety**: Safe for use in multi-threaded environments

### Usage Example

```cpp
#include "core/file_utils.hpp"

// List files non-recursively
auto observable = FileUtils::listFilesAsObservable("/path/to/dir", false);
observable.subscribe(
    [](const std::string& file_path) {
        std::cout << "Found: " << file_path << std::endl;
    },
    [](const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    },
    []() {
        std::cout << "Scan completed." << std::endl;
    }
);

// List files recursively
auto recursive_observable = FileUtils::listFilesAsObservable("/path/to/dir", true);
recursive_observable.subscribe(
    [](const std::string& file_path) {
        std::cout << "Found: " << file_path << std::endl;
    }
);
```

### SimpleObservable Implementation

The custom `SimpleObservable` provides:

- **onNext**: Called for each file found
- **onError**: Called when an error occurs
- **onComplete**: Called when scanning is finished
- **Flexible Subscription**: Support for different callback combinations

## Benefits of This Organization

1. **Clear Separation of Concerns**: Each module has a specific responsibility
2. **Easy Navigation**: Developers can quickly find relevant code
3. **Scalability**: Easy to add new modules or expand existing ones
4. **Maintainability**: Related functionality is grouped together
5. **Dependency Management**: Clear dependencies between modules
6. **Reactive Programming**: Modern reactive patterns for file operations

## Include Paths

The project uses relative include paths that reflect the module structure:

```cpp
// Core functionality
#include "core/status.hpp"
#include "core/server_config.hpp"
#include "core/file_utils.hpp"

// Authentication
#include "auth/auth.hpp"
#include "auth/auth_middleware.hpp"

// Web functionality
#include "web/route_handlers.hpp"
#include "web/openapi_docs.hpp"

// Logging
#include "logging/logger.hpp"
```

## Adding New Code

When adding new functionality:

1. **Identify the appropriate module** based on functionality
2. **Use consistent naming** (e.g., `feature.hpp` for headers)
3. **Update include paths** to reflect the new structure
4. **Update CMakeLists.txt** if adding new files
5. **Add tests** in the appropriate test file
6. **Create examples** if the functionality is complex

## Build System

The CMakeLists.txt file includes all necessary include directories:

```cmake
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/auth
    ${CMAKE_SOURCE_DIR}/include/web
    ${CMAKE_SOURCE_DIR}/include/logging
    ${CMAKE_SOURCE_DIR}/include/third_party
    ${OPENSSL_INCLUDE_DIR}
)
```
