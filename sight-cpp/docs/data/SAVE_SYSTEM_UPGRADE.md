# Save System Upgrade - Organized Folder Structure

## Overview

The save system has been completely reorganized to provide better organization, easier management, and improved user experience. The new system uses a structured folder hierarchy with separate save types and comprehensive metadata tracking.

## New Folder Structure

```
saves/
├── games/          # Game state saves
├── avatars/        # Avatar saves  
├── designs/        # Design system saves
├── backups/        # Automatic backups
├── logs/           # Save logs and metadata
├── formations/     # Formation saves (legacy)
├── persons/        # Person saves (legacy)
└── objects/        # Object saves (legacy)
```

## Save Types

The system now supports different save types for better organization:

- **GAME**: Game state saves (zones, camera, objects, etc.)
- **AVATAR**: Avatar saves (character data, body parts, stats)
- **DESIGN**: Design system saves (layers, history, tools)
- **BACKUP**: Automatic backups of existing saves
- **CUSTOM**: Custom saves for specific purposes

## Key Features

### 1. Organized Storage
- Each save type has its own folder
- Automatic folder creation
- Clean separation of concerns

### 2. Enhanced Logging
- Separate log files for each save type
- Automatic log maintenance
- Stale file cleanup

### 3. Metadata Tracking
- File creation time
- File size
- Custom labels
- Save type information

### 4. Backup System
- Automatic backup creation
- Backup management
- Restore capabilities

### 5. Cleanup Tools
- Automatic cleanup of old saves
- Configurable retention policies
- Manual cleanup options

## API Changes

### SaveSystem Namespace

```cpp
// Save types
enum class SaveType {
    GAME,       // Game state saves
    AVATAR,     // Avatar saves
    DESIGN,     // Design system saves
    BACKUP,     // Automatic backups
    CUSTOM      // Custom saves
};

// Core functions
std::string ensureSaveFolder();
std::string ensureSaveTypeFolder(SaveType type);
std::string makeFilename(const std::string& customLabel = "", SaveType type = SaveType::GAME);
void addToLog(const std::string& filepath, SaveType type = SaveType::GAME);
std::vector<std::string> listFiles(SaveType type = SaveType::GAME);
std::string writeJson(const nlohmann::json& j, const std::string& customLabel = "", SaveType type = SaveType::GAME);

// Advanced features
std::string createBackup(const std::string& originalFile, SaveType type = SaveType::GAME);
void cleanupOldSaves(SaveType type = SaveType::GAME, int keepCount = 10);
std::vector<SaveMetadata> getSaveMetadata(SaveType type = SaveType::GAME);
```

### SaveMetadata Structure

```cpp
struct SaveMetadata {
    std::string filename;
    std::string fullPath;
    std::time_t creationTime;
    size_t fileSize;
    SaveType type;
    std::string customLabel;
};
```

## UI Improvements

### New Save Windows

1. **Save Window**: Quick save with custom naming
2. **Load Window**: Enhanced load dialog with metadata display
3. **Save Manager**: Comprehensive save management with tabs

### UI Features

- File size display
- Creation time display
- Custom label support
- Save type organization
- Cleanup tools
- Backup management

## Migration

### Automatic Migration

The migration script (`migrate_saves.cpp`) automatically:

1. Creates new folder structure
2. Moves existing saves to appropriate folders
3. Updates log files
4. Preserves all existing data

### Manual Migration

If needed, you can manually migrate by:

1. Creating the new folder structure
2. Moving files to appropriate folders
3. Updating log files

## Usage Examples

### Saving Game State

```cpp
// Quick save with timestamp
SaveSystem::writeJson(gameData, "", SaveSystem::SaveType::GAME);

// Save with custom name
SaveSystem::writeJson(gameData, "My Awesome Save", SaveSystem::SaveType::GAME);
```

### Saving Avatar Data

```cpp
// Save avatar state
SaveSystem::writeJson(avatarData, avatarName, SaveSystem::SaveType::AVATAR);
```

### Loading Saves

```cpp
// Get list of game saves
auto gameSaves = SaveSystem::listFiles(SaveSystem::SaveType::GAME);

// Get metadata for display
auto metadata = SaveSystem::getSaveMetadata(SaveSystem::SaveType::GAME);
```

### Cleanup

```cpp
// Keep only 10 most recent saves
SaveSystem::cleanupOldSaves(SaveSystem::SaveType::GAME, 10);
```

## Benefits

1. **Better Organization**: Clear separation of different save types
2. **Easier Management**: Dedicated tools for save management
3. **Improved Performance**: Faster file operations with organized structure
4. **Enhanced UI**: Better user experience with metadata display
5. **Automatic Maintenance**: Built-in cleanup and backup features
6. **Future-Proof**: Extensible system for new save types

## Backward Compatibility

- All existing saves are preserved during migration
- Old save format is still supported
- Gradual migration path available
- No data loss during upgrade

## File Formats

All saves use JSON format for:

- Human readability
- Easy debugging
- Cross-platform compatibility
- Extensibility

## Log Files

Each save type has its own log file:

- `game_save_log.txt`: Game saves
- `avatar_save_log.txt`: Avatar saves
- `design_save_log.txt`: Design saves
- `backup_save_log.txt`: Backup files
- `custom_save_log.txt`: Custom saves

## Error Handling

The system includes comprehensive error handling:

- Folder creation failures
- File write errors
- Log maintenance issues
- Backup failures
- Cleanup errors

All errors are logged with descriptive messages for debugging.

## Performance Considerations

- Efficient file operations
- Minimal memory usage
- Fast metadata generation
- Optimized cleanup algorithms
- Background maintenance tasks

## Future Enhancements

Potential future improvements:

1. Cloud save integration
2. Save compression
3. Incremental saves
4. Save encryption
5. Multi-user support
6. Save sharing features 