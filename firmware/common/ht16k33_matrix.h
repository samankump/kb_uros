#ifndef HT16K33_MATRIX_H
#define HT16K33_MATRIX_H

#include <Arduino.h>
#include <Wire.h>

#define HT16K33_DEFAULT_ADDR 0x70
#define HT16K33_CMD_OSC_ON   0x21
#define HT16K33_CMD_DISP_ON  0x81
#define HT16K33_CMD_DISP_OFF 0x80
#define HT16K33_CMD_BRIGHT   0xE0

// KidBright 32 Default I2C Pins for Built-in LED Matrix
#define KB_MATRIX_SDA 21
#define KB_MATRIX_SCL 22

enum MatrixIcon {
    ICON_CLEAR = 0,
    ICON_HEART,
    ICON_SMILE,
    ICON_EXCLAMATION,
    ICON_CHECK,
    ICON_NUM1,
    ICON_NUM2
};

class KidbrightMatrix {
public:
    KidbrightMatrix(uint8_t addr = HT16K33_DEFAULT_ADDR) : _addr(addr) {
        memset(_buffer, 0, sizeof(_buffer));
    }

    bool begin(int sda = KB_MATRIX_SDA, int scl = KB_MATRIX_SCL) {
        Wire.begin(sda, scl);
        
        // Check if device responds on I2C
        Wire.beginTransmission(_addr);
        if (Wire.endTransmission() != 0) {
            return false;
        }

        // Turn on system oscillator
        sendCommand(HT16K33_CMD_OSC_ON);
        // Display on, blinking off
        sendCommand(HT16K33_CMD_DISP_ON);
        // Set brightness (default medium-high: 10/15)
        setBrightness(10);
        
        clear();
        return true;
    }

    void setBrightness(uint8_t brightness) {
        if (brightness > 15) brightness = 15;
        sendCommand(HT16K33_CMD_BRIGHT | brightness);
    }

    void clear() {
        memset(_buffer, 0, sizeof(_buffer));
        flush();
    }

    void flush() {
        Wire.beginTransmission(_addr);
        Wire.write(0x00); // RAM address pointer
        for (uint8_t i = 0; i < 16; i++) {
            Wire.write(_buffer[i]);
        }
        Wire.endTransmission();
    }

    void setPixel(uint8_t x, uint8_t y, bool on) {
        if (x >= 16 || y >= 8) return;
        uint8_t byteIndex = y * 2 + (x >= 8 ? 1 : 0);
        uint8_t bitPos = x % 8;

        if (on) {
            _buffer[byteIndex] |= (1 << (7 - bitPos));
        } else {
            _buffer[byteIndex] &= ~(1 << (7 - bitPos));
        }
    }

    void showIcon(MatrixIcon icon) {
        switch (icon) {
            case ICON_HEART: {
                const uint8_t heartMap[16] = {
                    0x00, 0x00,
                    0x0C, 0x30,
                    0x1E, 0x78,
                    0x1F, 0xF8,
                    0x0F, 0xF0,
                    0x07, 0xE0,
                    0x03, 0xC0,
                    0x01, 0x80
                };
                memcpy(_buffer, heartMap, 16);
                break;
            }
            case ICON_SMILE: {
                const uint8_t smileMap[16] = {
                    0x00, 0x00,
                    0x18, 0x18,
                    0x18, 0x18,
                    0x00, 0x00,
                    0x20, 0x04,
                    0x10, 0x08,
                    0x0F, 0xF0,
                    0x00, 0x00
                };
                memcpy(_buffer, smileMap, 16);
                break;
            }
            case ICON_EXCLAMATION: {
                const uint8_t alertMap[16] = {
                    0x01, 0x80,
                    0x01, 0x80,
                    0x01, 0x80,
                    0x01, 0x80,
                    0x01, 0x80,
                    0x00, 0x00,
                    0x01, 0x80,
                    0x00, 0x00
                };
                memcpy(_buffer, alertMap, 16);
                break;
            }
            case ICON_CHECK: {
                const uint8_t checkMap[16] = {
                    0x00, 0x02,
                    0x00, 0x06,
                    0x00, 0x0C,
                    0x00, 0x18,
                    0x60, 0x30,
                    0x30, 0x60,
                    0x18, 0xC0,
                    0x0C, 0x00
                };
                memcpy(_buffer, checkMap, 16);
                break;
            }
            case ICON_NUM1: {
                const uint8_t num1Map[16] = {
                    0x02, 0x00,
                    0x06, 0x00,
                    0x02, 0x00,
                    0x02, 0x00,
                    0x02, 0x00,
                    0x02, 0x00,
                    0x07, 0x00,
                    0x00, 0x00
                };
                memcpy(_buffer, num1Map, 16);
                break;
            }
            case ICON_NUM2: {
                const uint8_t num2Map[16] = {
                    0x07, 0x00,
                    0x01, 0x80,
                    0x00, 0x80,
                    0x01, 0x00,
                    0x02, 0x00,
                    0x04, 0x00,
                    0x07, 0x80,
                    0x00, 0x00
                };
                memcpy(_buffer, num2Map, 16);
                break;
            }
            case ICON_CLEAR:
            default:
                memset(_buffer, 0, sizeof(_buffer));
                break;
        }
        flush();
    }

private:
    uint8_t _addr;
    uint8_t _buffer[16];

    void sendCommand(uint8_t cmd) {
        Wire.beginTransmission(_addr);
        Wire.write(cmd);
        Wire.endTransmission();
    }
};

#endif // HT16K33_MATRIX_H
