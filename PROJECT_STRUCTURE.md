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
│   │   └── server_config.hpp   # Server configuration constants
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
│   └── auth.cpp               # Authentication implementation
├── tests/                     # Unit tests
│   ├── auth_test.cpp          # Authentication tests
│   ├── status_test.cpp        # Status tests
│   └── CMakeLists.txt         # Test build configuration
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

### 📝 **Logging Module** (`include/logging/`)

- **logger.hpp**: spdlog-based logging wrapper with consistent API

### 🌐 **Web Module** (`include/web/`)

- **route_handlers.hpp**: HTTP endpoint handlers and request processing
- **openapi_docs.hpp**: OpenAPI/Swagger documentation generation

### 📚 **Third-Party Module** (`include/third_party/`)

- External libraries and dependencies
- Kept separate to avoid conflicts with system libraries

## Benefits of This Organization

1. **Clear Separation of Concerns**: Each module has a specific responsibility
2. **Easy Navigation**: Developers can quickly find relevant code
3. **Scalability**: Easy to add new modules or expand existing ones
4. **Maintainability**: Related functionality is grouped together
5. **Dependency Management**: Clear dependencies between modules

## Include Paths

The project uses relative include paths that reflect the module structure:

```cpp
// Core functionality
#include "core/status.hpp"
#include "core/server_config.hpp"

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
