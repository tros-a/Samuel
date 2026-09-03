#include "CH58x_common.h"

#define SYS_CLK_FREQ     60000000   // 60MHz System clock

// Frequency calculations
#define CYCLES_700HZ     (SYS_CLK_FREQ / 700)   // ~85714 cycles
#define CYCLES_500HZ     (SYS_CLK_FREQ / 500)   // 120000 cycles

void PWM_Audio_Init(void) {
    // 1. Set PA9 to maximum 20mA output drive capability 
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_20mA);

    // 2. Initialize Timer 0 in standard PWM mode
    TMR0_PWMInit(High_Level, PWM_Times_1); 
    
    // 3. Start with 700Hz configuration by default
    TMR0_PWMCycleCfg(CYCLES_700HZ);
    TMR0_PWMActDataWidth(CYCLES_700HZ / 2);

    // 4. Explicitly map and turn on hardware wave tracking on the pin
    TMR0_PWMEnable();
    TMR0_Enable();
}

// Function to smoothly change the tone to any target frequency value
void Set_Audio_Tone(uint32_t period_cycles) {
    // Safely update the period structure parameters dynamically
    TMR0_PWMCycleCfg(period_cycles);
    TMR0_PWMActDataWidth(period_cycles / 2); // Maintain 50% square wave duty thickness
}

int main(void) {
    SetSysClock(CLK_SOURCE_PLL_60MHz);
    PWM_Audio_Init();

    while(1) {
        // 1. Play 700 Hz tone for 500 milliseconds
        Set_Audio_Tone(CYCLES_700HZ);
        DelayMs(500);

        // 2. Shift down to 500 Hz tone for 500 milliseconds
        Set_Audio_Tone(CYCLES_500HZ);
        DelayMs(500);

        // 3. Shift back up to 700 Hz tone for 500 milliseconds
        Set_Audio_Tone(CYCLES_700HZ);
        DelayMs(500);

        // Optional: Silence the audio completely for 1 second between sequences
        TMR0_Disable(); 
        DelayMs(1000);
        TMR0_Enable(); // Re-enable hardware clock to start over
    }
}
