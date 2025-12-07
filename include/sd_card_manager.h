/*
 * Encoder3D CNC Controller - SD Card Manager
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 * 
 * Optional SD card file operations for G-code storage
 * Singleton pattern - lazy initialization
 */

#ifndef SD_CARD_MANAGER_H
#define SD_CARD_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "config.h"

// Forward declaration
class GCodeParser;

// File execution state machine
enum FileExecutionState {
    FILE_IDLE,
    FILE_OPENING,
    FILE_READING,
    FILE_EXECUTING,
    FILE_PAUSED,
    FILE_COMPLETE,
    FILE_ERROR
};

class SDCardManager {
private:
    static SDCardManager* instance;
    bool card_initialized;
    GCodeParser* gcode_parser;
    
    // File execution state
    File current_file;
    FileExecutionState execution_state;
    String current_filename;
    unsigned long file_size;
    unsigned long bytes_read;
    unsigned long lines_executed;
    unsigned long last_line_time;
    
    // Line buffer for chunked reading
    String line_buffer;
    static const size_t READ_CHUNK_SIZE = 128;
    
    // Timing control (non-blocking)
    unsigned long last_update;
    static const unsigned long UPDATE_INTERVAL = 10; // ms between line sends
    
    // Private constructor for singleton
    SDCardManager();
    
public:
    ~SDCardManager();
    
    // Singleton access
    static SDCardManager* getInstance();
    static void destroyInstance();
    
    // Set G-code parser reference
    void setGCodeParser(GCodeParser* parser);
    
    // Initialization (lazy - called when first accessed)
    bool begin();
    
    // Non-blocking update (call in main loop)
    void update();
    
    // File operations
    bool startFile(String filename);
    void pauseExecution();
    void resumeExecution();
    void stopExecution();
    
    // File management
    bool fileExists(String filename);
    bool deleteFile(String filename);
    bool listFiles(String path = "/");
    size_t getFileSize(String filename);
    
    // Status
    bool isExecuting() { return execution_state == FILE_EXECUTING || execution_state == FILE_READING; }
    bool isPaused() { return execution_state == FILE_PAUSED; }
    bool isInitialized() { return card_initialized; }
    float getProgress();
    String getCurrentFile() { return current_filename; }
    unsigned long getLinesExecuted() { return lines_executed; }
    
    // Write file (for uploads)
    bool writeFile(String filename, const uint8_t* data, size_t len, bool append = false);
    File openFile(String filename, const char* mode);
    void closeFile(File& file);
    
private:
    void processNextLine();
    bool readNextChunk();
    String extractLine();
};

#endif // SD_CARD_MANAGER_H
