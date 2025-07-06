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

- `POST /auth/login`
  - Request body: `{"username": "string", "password": "string"}`
  - Response: `{"token": "string"}`

### Status Operations

All status endpoints require a valid JWT token in the `Authorization` header:
`Authorization: Bearer <token>`

- `GET /auth/status`
  - Response: `{"status": boolean}`

### Duplicate Finding Operations

All duplicate finding endpoints require a valid JWT token in the `Authorization` header:
`Authorization: Bearer <token>`

- `POST /duplicates/find`
  - Request body: `{"directory": "string"}`
  - Response: `{"message": "string"}`
  - Scans directory recursively and prints found files to console

## Example Usage

1. Get a token:

```bash
curl -X POST http://localhost:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username": "testuser", "password": "testpass"}'
```

2. Check service status:

```bash
curl -X GET http://localhost:8080/auth/status \
  -H "Authorization: Bearer <token>"
```

3. Find duplicates in a directory:

```bash
curl -X POST http://localhost:8080/duplicates/find \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{"directory": "/path/to/scan"}'
```

## Security Notes

### Current Development State (NOT PRODUCTION READY)

- **Hardcoded credentials**: username: "testuser", password: "testpass"
- **Hardcoded JWT secret**: Tokens persist across server restarts
- **No token expiration**: JWT tokens never expire
- **No password hashing**: Passwords stored in plain text
- **No rate limiting**: No protection against brute force attacks

### Production Requirements

- Use a secure secret key for JWT signing from environment variables
- Store the secret key in environment variables
- Implement proper user authentication against a database
- Add JWT token expiration (24 hours recommended)
- Hash passwords with bcrypt/argon2
- Use HTTPS in production
- Implement rate limiting and other security measures
- Add audit logging for authentication events
- Consider token blacklisting/revocation mechanisms
