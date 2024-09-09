
#include "mbed.h"
#include "Arduino.h"
#include "SDMMCBlockDevice.h"
#include "FATFileSystem.h"
#include <stdio.h>

#define MODEL_PATH "/fs/Model/sine.bin"
#define READ        0x03
#define WRITE       0x02
#define CS PI_0
#define SRAM_SIZE 4*1024*1024

SDMMCBlockDevice block_device;
mbed::FATFileSystem fs("fs"); // Name assigned to the filesystem when mounted
mbed::SPI spi(PC_3, PC_2, PI_1); 

uint32_t SRAM_ADDR = 0;

struct filedata{
  uint32_t address;
  uint32_t size;

}model_data;

// SPI SRAM Malloc
uint32_t SRAMMalloc(size_t size) {
    if (SRAM_ADDR + size >= SRAM_SIZE) {
        // No more memory available in external SRAM
        return 0xFFFFFFFF; // Return an invalid address to indicate failure
    }

    uint32_t allocated_address = SRAM_ADDR;
    SRAM_ADDR += size;
    return allocated_address;
}

void SpiWriteByteArray(uint32_t address, uint8_t *data, size_t size)
{  
    digitalWrite(CS,LOW);
    spi.write((uint8_t)WRITE);
    spi.write((uint8_t)((address >> 16)& 0xFF));
    spi.write((uint8_t)((address >> 8)& 0xFF));
    spi.write((uint8_t)address);
    for(size_t i=0;i<size;i++)
    {
      spi.write(data[i]);
    }
    
    digitalWrite(CS,HIGH);
}
//uint16_t value;


void SpiReadByteArray(uint32_t address, uint16_t size, uint8_t* readarray)
{

    
    digitalWrite(CS,LOW);
    spi.write((uint8_t)READ);
    spi.write((uint8_t)((address >> 16)& 0xFF));
    spi.write((uint8_t)((address >> 8)& 0xFF));
    spi.write((uint8_t)address);
    for(int i=0;i<size;i++)
    {
      readarray[i] = spi.write((uint8_t)0);
    }
    digitalWrite(CS,HIGH);
    
}
// Returns address
void WriteFileInChunks(const char* filepath, size_t chunk_size = 32*100) {
    
    FILE *file = fopen(filepath, "r");

    // Buffer to hold each chunk
    uint8_t buffer[chunk_size];

    // fread returns chunk size
    size_t bytesRead;
    uint32_t file_size=0;
    uint32_t address=0;
    uint32_t file_address;

    if (file == NULL) {
        Serial.println("Failed to open file.");
        return ;
    }
    
    while ((bytesRead = fread(buffer, 1, chunk_size, file)) > 0) {

        address = SRAMMalloc(bytesRead);

        if (address!= 0xFFFFFFFF){
            if (file_size==0)
            {
                // Save initial file address
                file_address = address;
                
            }
        
        SpiWriteByteArray(address,buffer,bytesRead);
        file_size+=bytesRead;
        }
    }
    
    // Close the file after reading
    fclose(file);
    model_data.address = address;
    model_data.size = file_size;
    
    Serial.println("File reading completed.");
    
}



void list_files_in_directory(const char *dirname, int level = 0) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(dirname);
    if (dir == NULL) {
        Serial.println("Failed to open directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        for (int i = 0; i < level; i++) {
            Serial.print("  "); // Indent subdirectories
        }

        Serial.print(entry->d_name);
        
        if (entry->d_type == DT_DIR) {
            Serial.println("/"); // Indicate it's a directory
            char path[256];
            snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

            // Recursively list contents of the directory
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                list_files_in_directory(path, level + 1);
            }
        } else {
            Serial.println(); // It's a file
        }
    }

    closedir(dir);
}
