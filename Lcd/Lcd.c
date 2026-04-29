//
// Created by Khalaf on 29/04/2026.
//

#include "Lcd.h"

// --- Helper Delays (Replace with Timer later if needed) ---
static void delay_us(uint32 count) {
    uint32 i; for(i = 0; i < (count * 2); i++) { __asm("NOP"); }
}
static void delay_ms(uint32 count) {
    uint32 i; for(i = 0; i < (count * 2000); i++) { __asm("NOP"); }
}

void Lcd_PrepareConfig(P_void config_out,
                       GPIO_Pin_Location_t rs, GPIO_Pin_Location_t en,
                       GPIO_Pin_Location_t d4, GPIO_Pin_Location_t d5,
                       GPIO_Pin_Location_t d6, GPIO_Pin_Location_t d7) {

    Lcd_Config_Handle_t lcd = (Lcd_Config_Handle_t)config_out;

    lcd->rs_pin = rs;
    lcd->en_pin = en;
    lcd->d4_pin = d4;
    lcd->d5_pin = d5;
    lcd->d6_pin = d6;
    lcd->d7_pin = d7;
}

// --- Internal Helper: Pulse Enable ---
static void Lcd_PulseEnable(const Lcd_Config_Handle_t lcd_h) {
    GPIO_WritePin(&(lcd_h->en_pin), GPIO_HIGH);
    delay_us(5);
    GPIO_WritePin(&(lcd_h->en_pin), GPIO_LOW);
    delay_us(100);
}

// --- Internal Helper: Send 4 Bits ---
static void Lcd_SendNibble(const Lcd_Config_Handle_t lcd_h, uint8 nibble) {

    GPIO_WritePin(&(lcd_h->d4_pin), (nibble >> 0) & 0x01 ? GPIO_HIGH : GPIO_LOW);
    GPIO_WritePin(&(lcd_h->d5_pin), (nibble >> 1) & 0x01 ? GPIO_HIGH : GPIO_LOW);
    GPIO_WritePin(&(lcd_h->d6_pin), (nibble >> 2) & 0x01 ? GPIO_HIGH : GPIO_LOW);
    GPIO_WritePin(&(lcd_h->d7_pin), (nibble >> 3) & 0x01 ? GPIO_HIGH : GPIO_LOW);
}

// --- Core API: Send Command ---
void Lcd_SendCommand(const Lcd_Config_Handle_t lcd_h, uint8 cmd) {
    // 1. Set RS to LOW for Command
    GPIO_WritePin(&(lcd_h->rs_pin), GPIO_LOW);

    // 2. Send upper 4 bits
    Lcd_SendNibble(lcd_h, cmd >> 4);
    Lcd_PulseEnable(lcd_h);

    // 3. Send lower 4 bits
    Lcd_SendNibble(lcd_h, cmd & 0x0F);
    Lcd_PulseEnable(lcd_h);

    delay_ms(2);
}

// --- Core API: Initialization Sequence ---
void Lcd_Init(const Lcd_Config_Handle_t lcd_h) {
    // 1. Prepare standard GPIO Config for all LCD pins (Output, Push-Pull, Medium Speed)
    GPIO_PinConfig_t pin_cfg;
    GPIO_PrepareConfig(&pin_cfg, GPIO_MODE_OUTPUT, GPIO_PULL_NONE, GPIO_SPEED_MEDIUM, GPIO_OTYPE_PP);

    // 2. Initialize all 6 GPIO pins using your GPIO driver
    GPIO_InitPin(&(lcd_h->rs_pin), &pin_cfg);
    GPIO_InitPin(&(lcd_h->en_pin), &pin_cfg);
    GPIO_InitPin(&(lcd_h->d4_pin), &pin_cfg);
    GPIO_InitPin(&(lcd_h->d5_pin), &pin_cfg);
    GPIO_InitPin(&(lcd_h->d6_pin), &pin_cfg);
    GPIO_InitPin(&(lcd_h->d7_pin), &pin_cfg);

    // 3. Ensure RS and EN start LOW
    GPIO_WritePin(&(lcd_h->rs_pin), GPIO_LOW);
    GPIO_WritePin(&(lcd_h->en_pin), GPIO_LOW);

    // 4. The strict 4-bit initialization sequence
    delay_ms(50); // Wait for power to stabilize

    Lcd_SendNibble(lcd_h, 0x03); Lcd_PulseEnable(lcd_h); delay_ms(5);
    Lcd_SendNibble(lcd_h, 0x03); Lcd_PulseEnable(lcd_h); delay_us(150);
    Lcd_SendNibble(lcd_h, 0x03); Lcd_PulseEnable(lcd_h); delay_us(150);

    Lcd_SendNibble(lcd_h, 0x02); Lcd_PulseEnable(lcd_h); delay_us(150); // Set to 4-bit mode!

    // 5. Apply operational settings
    Lcd_SendCommand(lcd_h, 0x28); // 4-bit mode, 2 lines, 5x8 font
    Lcd_SendCommand(lcd_h, 0x0C); // Display ON, Cursor OFF
    Lcd_SendCommand(lcd_h, 0x01); // Clear display
    delay_ms(2);
    Lcd_SendCommand(lcd_h, 0x06); // Entry mode (Auto-increment cursor)
}
// --- Core API: Send Data (Character) ---
void Lcd_SendData(const Lcd_Config_Handle_t lcd_h, uint8 data) {
    // 1. Set RS to HIGH for Data
    GPIO_WritePin(&(lcd_h->rs_pin), GPIO_HIGH);

    // 2. Send upper 4 bits
    Lcd_SendNibble(lcd_h, data >> 4);
    Lcd_PulseEnable(lcd_h);

    // 3. Send lower 4 bits
    Lcd_SendNibble(lcd_h, data & 0x0F);
    Lcd_PulseEnable(lcd_h);

    delay_us(100);
}

// --- Core API: Send String ---
void Lcd_SendString(const Lcd_Config_Handle_t lcd_h, const char* str) {
    while (*str != '\0') {
        Lcd_SendData(lcd_h, (uint8)(*str));
        str++;
    }
}

// --- Core API: Set Cursor Position ---
void Lcd_SetCursor(const Lcd_Config_Handle_t lcd_h, uint8 row, uint8 col) {
    uint8 address;

    // The LCD1602 memory starts at 0x00 for Row 0, and 0x40 for Row 1
    if (row == 0) {
        address = col;
    } else {
        address = 0x40 + col;
    }

    // The command to set memory address requires the highest bit (Bit 7) to be 1.
    // 0x80 in hex is 10000000 in binary. We OR it with the address.
    Lcd_SendCommand(lcd_h, 0x80 | address);
}

// --- Core API: Clear Display ---
void Lcd_Clear(const Lcd_Config_Handle_t lcd_h) {
    Lcd_SendCommand(lcd_h, 0x01);
    delay_ms(2); // Clear command requires a longer delay to process
}