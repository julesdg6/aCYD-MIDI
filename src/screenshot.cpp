#include "screenshot.h"
#include "common_definitions.h"

#include <cctype>

static bool sdInitialized = false;
static int screenshot_count = 0;

static void sanitizeLabel(const char *src, char *dst, size_t len) {
    size_t idx = 0;
    while (*src && idx + 1 < len) {
        char c = *src++;
        if (isalnum((unsigned char)c) || c == '_' || c == '-') {
            dst[idx++] = c;
        } else if (c == ' ' || c == '/' || c == '\\' || c == '.' || c == ':' ) {
            dst[idx++] = '_';
        }
    }
    if (idx == 0 && len > 1) {
        dst[idx++] = 'x';
    }
    dst[idx] = '\0';
}

static bool ensureScreenshotDirectory() {
    if (SD.exists("/screenshots")) {
        return true;
    }
    return SD.mkdir("/screenshots");
}

// BMP file header structures (Windows BMP format)
#pragma pack(push, 1)
// BITMAPFILEHEADER - File header (14 bytes)
typedef struct {
    uint16_t bfType;        // "BM" signature (0x4D42)
    uint32_t bfSize;        // File size in bytes
    uint16_t bfReserved1;   // Reserved (0)
    uint16_t bfReserved2;   // Reserved (0)
    uint32_t bfOffBits;     // Offset to pixel data
} BITMAPFILEHEADER;

// BITMAPINFOHEADER - DIB header (40 bytes)
typedef struct {
    uint32_t biSize;            // Header size (40)
    int32_t biWidth;            // Image width
    int32_t biHeight;           // Image height (positive = bottom-up)
    uint16_t biPlanes;          // Color planes (1)
    uint16_t biBitCount;        // Bits per pixel (24)
    uint32_t biCompression;     // Compression type (0 = none)
    uint32_t biSizeImage;       // Image size in bytes
    int32_t biXPelsPerMeter;    // Horizontal resolution (0)
    int32_t biYPelsPerMeter;    // Vertical resolution (0)
    uint32_t biClrUsed;         // Colors used (0 = all)
    uint32_t biClrImportant;    // Important colors (0 = all)
} BITMAPINFOHEADER;
#pragma pack(pop)

bool initializeSD() {
    if (sdInitialized) {
        Serial.println("SD Card already initialized");
        return true;
    }
    
    // Initialize SD card with SPI
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD Card initialization failed (SD.begin returned false)");
        return false;
    }
    
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD card attached");
        return false;
    }

    const char *typeLabel = "UNKNOWN";
    switch (cardType) {
        case CARD_MMC:
            typeLabel = "MMC";
            break;
        case CARD_SD:
            typeLabel = "SDSC";
            break;
        case CARD_SDHC:
            typeLabel = "SDHC/SDXC";
            break;
        default:
            break;
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card detected: %s, %lluMB\n", typeLabel, cardSize);
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

bool takeScreenshot(const char* label) {
    Serial.println("Taking screenshot...");
    
    if (!initializeSD()) {
        Serial.println("Failed to initialize SD card");
        Serial.println("Screenshot aborted: SD card not present or not initialized");
        return false;
    }
    
    if (!ensureScreenshotDirectory()) {
        Serial.println("Failed to create screenshots directory");
        return false;
    }

    lv_display_t* disp = lv_display_get_default();
    if (!disp) {
        Serial.println("No display found");
        return false;
    }

    int32_t width = lv_display_get_horizontal_resolution(disp);
    int32_t height = lv_display_get_vertical_resolution(disp);

    Serial.printf("Display size: %dx%d\n", width, height);

    size_t pixels = (size_t)width * (size_t)height;
    size_t buf_size = pixels * LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565);
    if (buf_size == 0 || buf_size > 1024 * 1024) {
        Serial.printf("Invalid buffer size: %zu bytes\n", buf_size);
        return false;
    }

    void* buf = malloc(buf_size);
    if (!buf) {
        Serial.println("Failed to allocate snapshot buffer");
        return false;
    }

    lv_obj_t* screen = lv_screen_active();
    lv_image_dsc_t snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    lv_result_t res = lv_snapshot_take_to_buf(screen, LV_COLOR_FORMAT_RGB565, &snapshot, buf, buf_size);
    if (res != LV_RESULT_OK) {
        Serial.printf("Failed to take snapshot: %d\n", res);
        free(buf);
        return false;
    }

    const lv_color16_t* color_buf = (const lv_color16_t*)snapshot.data;
    Serial.println("Snapshot captured, saving to BMP...");

    char labelBuf[32];
    const char* baseLabel = (label && label[0]) ? label : "screen";
    sanitizeLabel(baseLabel, labelBuf, sizeof(labelBuf));

    char boardBuf[32];
    sanitizeLabel(BOARD_NAME, boardBuf, sizeof(boardBuf));

    // Convert version to use dashes instead of dots
    char versionBuf[16];
    strncpy(versionBuf, ACYD_MIDI_VERSION, sizeof(versionBuf) - 1);
    versionBuf[sizeof(versionBuf) - 1] = '\0';
    for (size_t i = 0; i < sizeof(versionBuf) && versionBuf[i]; i++) {
        if (versionBuf[i] == '.') {
            versionBuf[i] = '-';
        }
    }

    char filename[96];
    snprintf(filename, sizeof(filename), "/screenshots/%s-%s_%s_%03d.bmp", boardBuf, versionBuf, labelBuf, screenshot_count++);

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
            uint16_t pixel = (color_buf[index].red << 11) |
                             (color_buf[index].green << 5) |
                             (color_buf[index].blue);
            
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

bool writeScreenshotDocumentation(const char *documentation[], int count) {
    if (!initializeSD()) {
        Serial.println("Failed to initialize SD card for documentation");
        return false;
    }
    
    if (!ensureScreenshotDirectory()) {
        Serial.println("Failed to create screenshots directory");
        return false;
    }

    char boardBuf[32];
    sanitizeLabel(BOARD_NAME, boardBuf, sizeof(boardBuf));

    char versionBuf[16];
    strncpy(versionBuf, ACYD_MIDI_VERSION, sizeof(versionBuf) - 1);
    versionBuf[sizeof(versionBuf) - 1] = '\0';
    for (size_t i = 0; i < sizeof(versionBuf) && versionBuf[i]; i++) {
        if (versionBuf[i] == '.') {
            versionBuf[i] = '-';
        }
    }

    char filename[96];
    snprintf(filename, sizeof(filename), "/screenshots/%s-%s_documentation.txt", boardBuf, versionBuf);

    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open documentation file for writing");
        return false;
    }

    file.printf("aCYD MIDI Screenshot Documentation\n");
    file.printf("===================================\n\n");
    file.printf("Board: %s\n", BOARD_NAME);
    file.printf("Version: %s\n", ACYD_MIDI_VERSION);
    file.printf("Capture Time: %lu ms since boot\n", millis());
    file.printf("\nScreenshots:\n");
    file.printf("------------\n\n");

    for (int i = 0; i < count; i++) {
        file.printf("%s\n", documentation[i]);
    }

    file.close();
    Serial.printf("Documentation saved to %s\n", filename);
    return true;
}
