/*
 * Encoder3D CNC Controller - SD Card Manager Implementation
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "sd_card_manager.h"
#include "gcode_parser.h"

// Static instance
SDCardManager* SDCardManager::instance = nullptr;

SDCardManager::SDCardManager() 
    : gcode_parser(nullptr), card_initialized(false), execution_state(FILE_IDLE),
      file_size(0), bytes_read(0), lines_executed(0), last_update(0), last_line_time(0) {
}

SDCardManager::~SDCardManager() {
    if (current_file) {
        current_file.close();
    }
    SD.end();
}

SDCardManager* SDCardManager::getInstance() {
    if (instance == nullptr) {
        instance = new SDCardManager();
        // Try to initialize immediately
        instance->begin();
    }
    return instance;
}

void SDCardManager::destroyInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

void SDCardManager::setGCodeParser(GCodeParser* parser) {
    gcode_parser = parser;
}

bool SDCardManager::begin() {
    if (!SD_ENABLED) {
        Serial.println("SD card disabled in config");
        return false;
    }
    
    // Initialize SPI with custom pins
    SPI.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    
    // Initialize SD card (non-blocking after this point)
    if (!SD.begin(SD_CS_PIN)) {
        if (Serial.availableForWrite() > 50) {
            Serial.println("SD card initialization failed!");
        }
        card_initialized = false;
        return false;
    }
    
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        if (Serial.availableForWrite() > 50) {
            Serial.println("No SD card attached");
        }
        card_initialized = false;
        return false;
    }
    
    card_initialized = true;
    
    if (Serial.availableForWrite() > 100) {
        Serial.print("SD Card Type: ");
        if (cardType == CARD_MMC) {
            Serial.println("MMC");
        } else if (cardType == CARD_SD) {
            Serial.println("SDSC");
        } else if (cardType == CARD_SDHC) {
            Serial.println("SDHC");
        } else {
            Serial.println("UNKNOWN");
        }
        
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        Serial.printf("SD Card Size: %lluMB\n", cardSize);
    }
    
    return true;
}

void SDCardManager::update() {
    if (!card_initialized || execution_state == FILE_IDLE) {
        return;
    }
    
    // Non-blocking timing control
    unsigned long now = millis();
    if (now - last_update < UPDATE_INTERVAL) {
        return;
    }
    last_update = now;
    
    switch (execution_state) {
        case FILE_READING:
        case FILE_EXECUTING:
            processNextLine();
            break;
            
        case FILE_PAUSED:
            // Do nothing while paused
            break;
            
        case FILE_COMPLETE:
        case FILE_ERROR:
            if (current_file) {
                current_file.close();
            }
            execution_state = FILE_IDLE;
            break;
            
        default:
            break;
    }
}

bool SDCardManager::startFile(String filename) {
    if (!card_initialized) {
        if (Serial.availableForWrite() > 50) {
            Serial.println("SD card not initialized");
        }
        return false;
    }
    
    // Stop any current execution
    stopExecution();
    
    // Open file
    current_file = SD.open(filename.c_str(), FILE_READ);
    if (!current_file) {
        if (Serial.availableForWrite() > 50) {
            Serial.println("Failed to open file: " + filename);
        }
        execution_state = FILE_ERROR;
        return false;
    }
    
    current_filename = filename;
    file_size = current_file.size();
    bytes_read = 0;
    lines_executed = 0;
    line_buffer = "";
    execution_state = FILE_READING;
    
    if (Serial.availableForWrite() > 50) {
        Serial.println("Starting file execution: " + filename);
        Serial.printf("File size: %lu bytes\n", file_size);
    }
    
    return true;
}

void SDCardManager::processNextLine() {
    if (execution_state == FILE_PAUSED || !current_file) {
        return;
    }
    
    // Read chunk if buffer is empty
    if (line_buffer.length() == 0 && current_file.available()) {
        if (!readNextChunk()) {
            execution_state = FILE_ERROR;
            return;
        }
    }
    
    // Extract and execute one line
    String line = extractLine();
    if (line.length() > 0) {
        // Execute the G-code line (non-blocking)
        gcode_parser->processLine(line);
        lines_executed++;
        last_line_time = millis();
    }
    
    // Check if file is complete
    if (!current_file.available() && line_buffer.length() == 0) {
        execution_state = FILE_COMPLETE;
        if (Serial.availableForWrite() > 50) {
            Serial.println("File execution complete");
            Serial.printf("Lines executed: %lu\n", lines_executed);
        }
    }
}

bool SDCardManager::readNextChunk() {
    if (!current_file.available()) {
        return false;
    }
    
    char chunk[READ_CHUNK_SIZE + 1];
    size_t bytes = current_file.readBytes(chunk, READ_CHUNK_SIZE);
    chunk[bytes] = '\0';
    
    bytes_read += bytes;
    line_buffer += String(chunk);
    
    return true;
}

String SDCardManager::extractLine() {
    int newline_pos = line_buffer.indexOf('\n');
    if (newline_pos == -1) {
        newline_pos = line_buffer.indexOf('\r');
    }
    
    if (newline_pos == -1) {
        // No complete line yet
        if (!current_file.available()) {
            // End of file - return what's left
            String line = line_buffer;
            line_buffer = "";
            return line;
        }
        return "";
    }
    
    String line = line_buffer.substring(0, newline_pos);
    line_buffer = line_buffer.substring(newline_pos + 1);
    
    // Clean up line
    line.trim();
    
    return line;
}

void SDCardManager::pauseExecution() {
    if (execution_state == FILE_READING || execution_state == FILE_EXECUTING) {
        execution_state = FILE_PAUSED;
        if (Serial.availableForWrite() > 50) {
            Serial.println("File execution paused");
        }
    }
}

void SDCardManager::resumeExecution() {
    if (execution_state == FILE_PAUSED) {
        execution_state = FILE_READING;
        if (Serial.availableForWrite() > 50) {
            Serial.println("File execution resumed");
        }
    }
}

void SDCardManager::stopExecution() {
    if (current_file) {
        current_file.close();
    }
    execution_state = FILE_IDLE;
    line_buffer = "";
    
    if (Serial.availableForWrite() > 50) {
        Serial.println("File execution stopped");
    }
}

bool SDCardManager::fileExists(String filename) {
    if (!card_initialized) return false;
    return SD.exists(filename.c_str());
}

bool SDCardManager::deleteFile(String filename) {
    if (!card_initialized) return false;
    return SD.remove(filename.c_str());
}

bool SDCardManager::listFiles(String path) {
    if (!card_initialized) return false;
    
    File root = SD.open(path.c_str());
    if (!root) {
        return false;
    }
    
    if (!root.isDirectory()) {
        root.close();
        return false;
    }
    
    if (Serial.availableForWrite() > 50) {
        Serial.println("Files in " + path + ":");
    }
    
    File file = root.openNextFile();
    while (file) {
        if (Serial.availableForWrite() > 80) {
            if (file.isDirectory()) {
                Serial.print("  DIR : ");
            } else {
                Serial.print("  FILE: ");
                Serial.print(file.size());
                Serial.print(" bytes\t");
            }
            Serial.println(file.name());
        }
        file = root.openNextFile();
    }
    
    root.close();
    return true;
}

size_t SDCardManager::getFileSize(String filename) {
    if (!card_initialized) return 0;
    
    File file = SD.open(filename.c_str(), FILE_READ);
    if (!file) return 0;
    
    size_t size = file.size();
    file.close();
    
    return size;
}

float SDCardManager::getProgress() {
    if (file_size == 0) return 0.0;
    return (float)bytes_read / (float)file_size * 100.0;
}

bool SDCardManager::writeFile(String filename, const uint8_t* data, size_t len, bool append) {
    if (!card_initialized) return false;
    
    const char* mode = append ? FILE_APPEND : FILE_WRITE;
    File file = SD.open(filename.c_str(), mode);
    if (!file) {
        return false;
    }
    
    size_t written = file.write(data, len);
    file.close();
    
    return written == len;
}

File SDCardManager::openFile(String filename, const char* mode) {
    if (!card_initialized) {
        return File();
    }
    return SD.open(filename.c_str(), mode);
}

void SDCardManager::closeFile(File& file) {
    if (file) {
        file.close();
    }
}
