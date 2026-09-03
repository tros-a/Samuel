#include "oled_sh1106.h"
#include "oled_font.h"

void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr) {
    // Correctly bind the standard ASCII mapping range
    if (chr < 32 || chr > 126) return;
	
	//uint16_t base_idx = (uint16_t)(chr - 32) * 6;
    
    uint8_t c = chr - 32; 
    
    if (x > 122) { 
        x = 0;
        y++;
    }
    
    OLED_SetPos(x, y);
    for (uint8_t i = 0; i < 6; i++) {
        OLED_WriteData(Font6x8[c][i]);
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, char *str) {
    uint8_t j = 0;
    while (str[j] != '\0') {
        OLED_ShowChar(x, y, str[j]);
        x += 6; // Move by 6 horizontal matrix steps per character loop
        if (x > 122) {
            x = 0;
            y++;
        }
        j++;
    }
}







// Microsecond delay helper
static void I2C_Delay(void) {
    DelayUs(2);
}

// Low-level Bit-Bang I2C Operations
static void I2C_Start(void) {
    GPIOB_SetBits(OLED_SDA_PIN);
    GPIOB_SetBits(OLED_SCL_PIN);
    I2C_Delay();
    GPIOB_ResetBits(OLED_SDA_PIN);
    I2C_Delay();
    GPIOB_ResetBits(OLED_SCL_PIN);
}

static void I2C_Stop(void) {
    GPIOB_ResetBits(OLED_SDA_PIN);
    GPIOB_SetBits(OLED_SCL_PIN);
    I2C_Delay();
    GPIOB_SetBits(OLED_SDA_PIN);
    I2C_Delay();
}

static void I2C_WriteByte(uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++) {
        if (byte & 0x80) {
            GPIOB_SetBits(OLED_SDA_PIN);
        } else {
            GPIOB_ResetBits(OLED_SDA_PIN);
        }
        I2C_Delay();
        GPIOB_SetBits(OLED_SCL_PIN);
        I2C_Delay();
        GPIOB_ResetBits(OLED_SCL_PIN);
        byte <<= 1;
    }
    // Pseudo-ACK clock cycle (ignoring hard error checking for simplicity)
    GPIOB_SetBits(OLED_SDA_PIN);
    I2C_Delay();
    GPIOB_SetBits(OLED_SCL_PIN);
    I2C_Delay();
    GPIOB_ResetBits(OLED_SCL_PIN);
}

// Send Command to SH1106
void OLED_WriteCmd(uint8_t cmd) {
    I2C_Start();
    I2C_WriteByte(OLED_I2C_ADDR);
    I2C_WriteByte(0x00); // Co = 0, D/C# = 0 -> Control byte follows commands
    I2C_WriteByte(cmd);
    I2C_Stop();
}

// Send Data to SH1106
void OLED_WriteData(uint8_t data) {
    I2C_Start();
    I2C_WriteByte(OLED_I2C_ADDR);
    I2C_WriteByte(0x40); // Co = 0, D/C# = 1 -> Control byte follows data
    I2C_WriteByte(data);
    I2C_Stop();
}

// Set Memory Addressing Position
void OLED_SetPos(uint8_t x, uint8_t y) {
    // SH1106 column offset correction: RAM is 132 columns wide, panel uses central 128 (offset by 2)
    x += 2; 
    OLED_WriteCmd(0xB0 + y);                 // Set Page Address
    OLED_WriteCmd(((x & 0xF0) >> 4) | 0x10); // Set Higher 4 bits of Column Start Address
    OLED_WriteCmd(x & 0x0F);                 // Set Lower 4 bits of Column Start Address
}

// Clear the Screen
void OLED_Clear(void) {
    for (uint8_t y = 0; y < 8; y++) {
        OLED_SetPos(0, y);
        for (uint8_t x = 0; x < 128; x++) {
            OLED_WriteData(0x00);
        }
    }
}

// Initialize SH1106 Controller
void OLED_Init(void) {
    // Configure PB12 and PB13 as Open-Drain Outputs
    GPIOB_ModeCfg(OLED_SCL_PIN, GPIO_ModeOut_PP_5mA); // Soft I2C works well with Push-Pull/Open-Drain
    GPIOB_ModeCfg(OLED_SDA_PIN, GPIO_ModeOut_PP_5mA);
    
    GPIOB_SetBits(OLED_SCL_PIN);
    GPIOB_SetBits(OLED_SDA_PIN);
    DelayMs(100); // Wait for OLED Power Stabilization

    OLED_WriteCmd(0xAE); // Turn Display OFF
    OLED_WriteCmd(0x02); // Set Lower Column Address
    OLED_WriteCmd(0x10); // Set Higher Column Address
    OLED_WriteCmd(0x40); // Set Display Start Line
    OLED_WriteCmd(0x81); // Set Contrast Control
    OLED_WriteCmd(0xCF); // Contrast value
    OLED_WriteCmd(0xA1); // Segment Remap (ADC)
    OLED_WriteCmd(0xC8); // COM Output Scan Direction
    OLED_WriteCmd(0xA6); // Normal Display Mode
    OLED_WriteCmd(0xA8); // Multiplex Ratio
    OLED_WriteCmd(0x3F); // 1/64 duty
    OLED_WriteCmd(0xD3); // Display Offset
    OLED_WriteCmd(0x00); // No offset
    OLED_WriteCmd(0xD5); // Display Clock Divide Ratio
    OLED_WriteCmd(0x80); 
    OLED_WriteCmd(0xD9); // Pre-charge Period
    OLED_WriteCmd(0xF1); 
    OLED_WriteCmd(0xDA); // COM Pins Hardware Configuration
    OLED_WriteCmd(0x12); 
    OLED_WriteCmd(0xDB); // VCOMH Deselect Level
    OLED_WriteCmd(0x40); 
    OLED_WriteCmd(0x8D); // Charge Pump Setting
    OLED_WriteCmd(0x14); // Enable Charge Pump
    OLED_WriteCmd(0xAF); // Turn Display ON
    
    OLED_Clear();
}
