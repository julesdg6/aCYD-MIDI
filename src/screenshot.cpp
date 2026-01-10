#include "screenshot.h"

static bool sdInitialized = false;

// BMP file header structures
#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

bool initializeSD() {
    if (sdInitialized) {
        return true;
    }
    
    // Initialize SD card with SPI
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD Card initialization failed!");
        return false;
    }
    
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD card attached");
        return false;
    }
    
    Serial.println("SD Card initialized successfully");
    sdInitialized = true;
    return true;
}

void shutdownSD() {
    if (sdInitialized) {
        SD.end();
        sdInitialized = false;
    }
}

// Convert RGB565 to RGB888 (24-bit color)
void rgb565ToRgb888(uint16_t rgb565, uint8_t* r, uint8_t* g, uint8_t* b) {
    *r = ((rgb565 >> 11) & 0x1F) << 3;
    *g = ((rgb565 >> 5) & 0x3F) << 2;
    *b = (rgb565 & 0x1F) << 3;
}

bool takeScreenshot() {
    Serial.println("Taking screenshot...");
    
    // Initialize SD card
    if (!initializeSD()) {
        Serial.println("Failed to initialize SD card");
        return false;
    }
    
    // Get display
    lv_display_t* disp = lv_display_get_default();
    if (!disp) {
        Serial.println("No display found");
        return false;
    }
    
    int32_t width = lv_display_get_horizontal_resolution(disp);
    int32_t height = lv_display_get_vertical_resolution(disp);
    
    Serial.printf("Display size: %dx%d\n", width, height);
    
    // Allocate buffer for snapshot (RGB565 format)
    size_t buf_size = width * height * sizeof(lv_color_t);
    void* buf = malloc(buf_size);
    if (!buf) {
        Serial.println("Failed to allocate snapshot buffer");
        return false;
    }
    
    // Take snapshot to buffer
    lv_obj_t* screen = lv_screen_active();
    lv_result_t res = lv_snapshot_take_to_buf(screen, LV_COLOR_FORMAT_RGB565, buf, buf_size);
    
    if (res != LV_RESULT_OK) {
        Serial.printf("Failed to take snapshot: %d\n", res);
        free(buf);
        return false;
    }
    
    lv_color_t* color_buf = (lv_color_t*)buf;
    
    Serial.println("Snapshot captured, saving to BMP...");
    
    // Generate filename with timestamp
    static int screenshot_count = 0;
    char filename[32];
    snprintf(filename, sizeof(filename), "/screen%03d.bmp", screenshot_count++);
    
    // Open file for writing
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        free(buf);
        return false;
    }
    
    // BMP file format requires rows to be padded to 4-byte boundaries
    int row_size = width * 3; // 3 bytes per pixel (RGB)
    int padding = (4 - (row_size % 4)) % 4;
    int padded_row_size = row_size + padding;
    
    // Write BMP file header
    BITMAPFILEHEADER fileHeader;
    fileHeader.bfType = 0x4D42; // "BM"
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (padded_row_size * height);
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    
    file.write((uint8_t*)&fileHeader, sizeof(fileHeader));
    
    // Write BMP info header
    BITMAPINFOHEADER infoHeader;
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height; // Positive height means bottom-up
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = 0; // BI_RGB
    infoHeader.biSizeImage = padded_row_size * height;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;
    
    file.write((uint8_t*)&infoHeader, sizeof(infoHeader));
    
    // Write pixel data (BMP is stored bottom-up)
    uint8_t padding_bytes[3] = {0, 0, 0};
    
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            uint16_t pixel = color_buf[index].full;
            
            uint8_t r, g, b;
            rgb565ToRgb888(pixel, &r, &g, &b);
            
            // BMP stores colors in BGR order
            file.write(b);
            file.write(g);
            file.write(r);
        }
        
        // Write padding
        if (padding > 0) {
            file.write(padding_bytes, padding);
        }
    }
    
    file.close();
    free(buf);
    
    Serial.printf("Screenshot saved to %s\n", filename);
    return true;
}
