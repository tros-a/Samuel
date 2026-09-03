#include "CH58x_common.h"
#include "oled_sh1106.h"

int main(void) {
    // 1. Initialize system clock to 60MHz
    SetSysClock(CLK_SOURCE_PLL_60MHz);

    // 2. Initialize the SH1106 OLED Display hardware
    OLED_Init();

    // 3. Output your exact text rows with standard line spacing
    OLED_ShowString(0, 0, "abcdefghj");
    OLED_ShowString(0, 2, "klmnopqrst");
    OLED_ShowString(0, 4, "`uvwxyz~");
    OLED_ShowString(0, 6, "2026 SANDBOX");

    while(1) {
        // Main loop processing
    }
}
