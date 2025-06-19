# Dedup Service

A secure dedup service that provides status checking through a REST API with JWT authentication.

## Features

- Service status checking
- JWT-based authentication
- RESTful API endpoints
- Unit tests
- Modern C++ (C++17)

## Dependencies

- CMake (3.10 or higher)
- C++17 compatible compiler
- OpenSSL
- nlohmann_json
- Google Test (automatically downloaded by CMake)
- cpp-httplib (automatically downloaded by CMake)
- jwt-cpp (automatically downloaded by CMake)

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Running Tests

```bash
cd build
ctest
```

## Running the Server

```bash
cd build
./dedup_server
```

The server will start on `localhost:8080`.

## API Endpoints

### Authentication

- `POST /login`
  - Request body: `{"username": "string", "password": "string"}`
  - Response: `{"token": "string"}`

### Status Operations

All status endpoints require a valid JWT token in the `Authorization` header:
`Authorization: Bearer <token>`

- `GET /status`
  - Response: `{"status": boolean}`

## Example Usage

1. Get a token:

```bash
curl -X POST http://localhost:8080/login \
  -H "Content-Type: application/json" \
  -d '{"username": "testuser", "password": "testpass"}'
```

2. Check service status:

```bash
curl -X GET http://localhost:8080/status \
  -H "Authorization: Bearer <token>"
```

## Security Notes

- In production, use a secure secret key for JWT signing
- Store the secret key in environment variables
- Implement proper user authentication against a database
- Use HTTPS in production
- Consider rate limiting and other security measures
