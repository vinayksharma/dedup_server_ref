# Database SQL Scripts

This directory contains all SQL scripts used by the dedup-server database system.

## Script Files

### `create_tables.sql`

Contains all table creation statements:

- `media_processing_results` - Stores processing results for each file and mode
- `scanned_files` - Stores information about scanned files
- `user_inputs` - Stores user input data
- `cache_map` - Maps source files to transcoded files
- `flags` - Stores system flags and state

### `create_triggers.sql`

Contains all trigger creation statements:

- Triggers for `scanned_files` table to update flags when data changes

### `create_indexes.sql`

Contains all index creation statements for performance optimization:

- Indexes on `cache_map` status and creation time
- Indexes on `scanned_files` file path and processing flags
- Indexes on `media_processing_results` file path
- Indexes on `flags` name

### Schema Management

The schema is now complete in the table definitions. No ALTER TABLE statements are needed since we're not in production yet and can change the schema directly.

### `init_database.sql`

Comprehensive initialization script that runs all other scripts in the correct order.

## Operation Scripts

These scripts contain SQL queries used in the operating logic of the server:

### `scanned_files_operations.sql`

Contains all SQL queries for scanned_files table operations:

- File metadata operations
- Processing status updates
- File existence checks
- Hash operations
- File links operations
- Processing status queries and resets

### `media_processing_operations.sql`

Contains all SQL queries for media_processing_results table operations:

- Processing results queries
- Processing status checks
- Statistics queries

### `cache_map_operations.sql`

Contains all SQL queries for cache_map table operations:

- Transcoding operations
- Cache map status updates
- Statistics queries

### `flags_and_user_inputs_operations.sql`

Contains all SQL queries for flags and user_inputs table operations:

- Flag operations
- User input operations
- Trigger operations

### `statistics_and_reporting.sql`

Contains all SQL queries for statistics, reporting, and complex operations:

- Basic statistics
- Transcoding statistics
- Database schema operations
- Dynamic field operations

## Usage

The scripts are automatically executed by the `DatabaseManager` class:

```cpp
// Initialize database using the comprehensive script
db_manager.executeScript(DatabaseScripts::INIT_DATABASE_SCRIPT);

// Or execute individual scripts as needed
db_manager.executeScript(DatabaseScripts::CREATE_TABLES_SCRIPT);
db_manager.executeScript(DatabaseScripts::CREATE_TRIGGERS_SCRIPT);
db_manager.executeScript(DatabaseScripts::CREATE_INDEXES_SCRIPT);

// Execute operation scripts for specific functionality
db_manager.executeScript(DatabaseScripts::SCANNED_FILES_OPERATIONS_SCRIPT);
db_manager.executeScript(DatabaseScripts::MEDIA_PROCESSING_OPERATIONS_SCRIPT);
db_manager.executeScript(DatabaseScripts::CACHE_MAP_OPERATIONS_SCRIPT);
db_manager.executeScript(DatabaseScripts::FLAGS_AND_USER_INPUTS_OPERATIONS_SCRIPT);
db_manager.executeScript(DatabaseScripts::STATISTICS_AND_REPORTING_SCRIPT);
```

## Benefits of This Structure

1. **Maintainability**: SQL statements are centralized and easier to modify
2. **Reusability**: Scripts can be run independently or together
3. **Version Control**: SQL changes are tracked in version control
4. **Testing**: Scripts can be tested independently
5. **Documentation**: SQL structure is clearly documented
6. **Consistency**: Ensures all database instances use the same schema
7. **Separation of Concerns**: Schema creation vs. operational queries are separated
8. **Code Organization**: SQL queries are organized by functionality and table
9. **Easier Debugging**: SQL queries can be reviewed and modified without touching C++ code
10. **Database Portability**: SQL scripts can be run on different database systems if needed

## Schema Evolution

When adding new tables, columns, or indexes:

1. Add the SQL to the appropriate script file (create_tables.sql for tables/columns, create_indexes.sql for indexes)
2. Update the `init_database.sql` script if needed
3. Test the script independently
4. Update this README if needed

## Adding New Operations

When adding new SQL queries for operating logic:

1. Add the SQL to the appropriate operation script file based on the table it operates on
2. Update the corresponding C++ code to reference the script instead of inline SQL
3. Test the new operation thoroughly
4. Update this README if needed
