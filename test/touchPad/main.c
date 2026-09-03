/*
 * tky_oled_diag.c
 *
 * TouchKey диагностика на OLED.
 * Показывает сырые значения ADC, baseline и delta в реальном времени.
 *
 * Подключение:
 *   Левый  пад → A5  (ch1)
 *   Правый пад → A14 (ch4)
 *   SDA → B12,  SCL → B13
 *
 * Нужны: CH58x_adc.c, CH58x_gpio.c, CH58x_i2c.c
 */

#include "CH58x_common.h"
#include <string.h>

#define OLED_ADDR      0x3C
#define SH1106_COL_OFF 2

static void wait(uint32_t n){ volatile uint32_t t=n; while(t--); }

/* ---- I2C ---- */
static void i2c_write(uint8_t addr, uint8_t ctrl,
                      const uint8_t *data, uint16_t len)
{
    I2C_GenerateSTART(ENABLE);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress((uint8_t)(addr<<1), I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    while(I2C_GetFlagStatus(I2C_FLAG_TXE)==RESET);
    I2C_SendData(ctrl);
    for(uint16_t i=0;i<len;i++){
        while(I2C_GetFlagStatus(I2C_FLAG_TXE)==RESET);
        I2C_SendData(data[i]);
    }
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_GenerateSTOP(ENABLE);
    for(uint32_t i=0;i<100;i++) __nop();
}

/* ---- OLED ---- */
static uint8_t g_fb[128*8];
static void oled_cmd(uint8_t c){ i2c_write(OLED_ADDR,0x00,&c,1); }

static void oled_init(void)
{
    wait(500000);
    const uint8_t s[]={
        0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,
        0xAD,0x8B,0xA1,0xC8,0xDA,0x12,
        0x81,0xFF,0xD9,0xF1,0xDB,0x40,
        0xA4,0xA6,0xAF
    };
    for(uint8_t i=0;i<sizeof(s);i++) oled_cmd(s[i]);
    memset(g_fb,0,sizeof(g_fb));
}

static void oled_flush(void)
{
    for(uint8_t p=0;p<8;p++){
        oled_cmd((uint8_t)(0xB0|p));
        oled_cmd(SH1106_COL_OFF&0x0F);
        oled_cmd(0x10|(SH1106_COL_OFF>>4));
        i2c_write(OLED_ADDR,0x40,&g_fb[p*128],128);
    }
}

/* ---- Шрифт ---- */
static const uint8_t F[][5]={
    {0x00,0x00,0x00,0x00,0x00},
    {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},
    {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},
    {0x3E,0x41,0x41,0x41,0x22},{0x7F,0x41,0x41,0x22,0x1C},
    {0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},
    {0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},
    {0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E},{0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},
    {0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},
    {0x08,0x08,0x08,0x08,0x08},{0x00,0x36,0x36,0x00,0x00},
    {0x00,0x00,0x5F,0x00,0x00},
};
static uint8_t ci(char c){
    if(c>='0'&&c<='9') return (uint8_t)(1+(c-'0'));
    if(c>='A'&&c<='Z') return (uint8_t)(11+(c-'A'));
    if(c=='-') return 37; if(c==':') return 38; if(c=='!') return 39;
    return 0;
}
static void fb_ch(uint8_t col,uint8_t row,char c){
    if(col>=21||row>=8) return;
    uint16_t off=(uint16_t)row*128u+(uint16_t)col*6u;
    if(off+5>sizeof(g_fb)) return;
    for(uint8_t i=0;i<5;i++) g_fb[off+i]=F[ci(c)][i];
    g_fb[off+5]=0;
}
static void fb_str(uint8_t col,uint8_t row,const char *s){
    while(*s&&col<21) fb_ch(col++,row,*s++);
}
static void fb_clr(void){ memset(g_fb,0,sizeof(g_fb)); }

static void fb_u16(uint8_t col,uint8_t row,uint16_t v){
    char buf[6]; buf[5]='\0';
    for(int8_t i=4;i>=0;i--){
        buf[i]=(char)('0'+v%10); v/=10;
        if(!v){ while(i-->0) buf[i]=' '; break; }
    }
    fb_str(col,row,buf);
}

static void fb_i32(uint8_t col,uint8_t row,int32_t v){
    char buf[7]; buf[6]='\0';
    uint8_t neg=(v<0); if(neg) v=-v;
    for(int8_t i=5;i>=1;i--){
        buf[i]=(char)('0'+v%10); v/=10;
        if(!v){ while(i-->1) buf[i]=' '; break; }
    }
    buf[0]=neg?'-':'+';
    fb_str(col,row,buf);
}

/* ---- TouchKey ---- */
static uint16_t tky_read(uint8_t ch)
{
    ADC_ChannelCfg(ch);
    uint32_t s=0;
    for(uint8_t i=0;i<16;i++)
        s += TouchKey_ExcutSingleConver(0x10,0);
    return (uint16_t)(s/16);
}

/* ================================================================
 * MAIN
 * ================================================================ */
int main(void)
{
    SetSysClock(CLK_SOURCE_PLL_60MHz);

    /* I2C */
    GPIOB_ModeCfg(GPIO_Pin_13|GPIO_Pin_12, GPIO_ModeIN_PU);
    I2C_Init(I2C_Mode_I2C,400000,I2C_DutyCycle_16_9,
             I2C_Ack_Enable,I2C_AckAddr_7bit,0x00);
    while(I2C_GetFlagStatus(I2C_FLAG_BUSY)!=RESET);

    oled_init();

    /* TouchKey */
    GPIOA_ModeCfg(GPIO_Pin_5,  GPIO_ModeIN_Floating);
    GPIOA_ModeCfg(GPIO_Pin_14, GPIO_ModeIN_Floating);
    TouchKey_ChSampInit();
    wait(100000);

    /* Экран ожидания */
    fb_clr();
    fb_str(0,0,"TKY DIAG");
    fb_str(0,1,"PLACE FOIL");
    fb_str(0,2,"ON A5 AND A14");
    fb_str(0,3,"DO NOT TOUCH!");
    fb_str(0,4,"CALIB 3 SEC...");
    oled_flush();

    /* Калибровка 3 сек */
    uint32_t sl=0, sr=0;
    for(uint8_t i=0;i<64;i++){
        sl += tky_read(1);
        sr += tky_read(4);
        wait(700000); /* ~3сек total при PLL 60MHz */
    }
    uint16_t bl_l=(uint16_t)(sl/64);
    uint16_t bl_r=(uint16_t)(sr/64);

    /* Порог — начинаем с 50, корректируем по данным */
    int32_t thr = 50;

    while(1)
    {
        /* Быстрый опрос — 4 усреднения */
        ADC_ChannelCfg(1);
        uint32_t s1=0;
        for(uint8_t i=0;i<4;i++) s1+=TouchKey_ExcutSingleConver(0x10,0);
        uint16_t vl=(uint16_t)(s1/4);

        ADC_ChannelCfg(4);
        uint32_t s2=0;
        for(uint8_t i=0;i<4;i++) s2+=TouchKey_ExcutSingleConver(0x10,0);
        uint16_t vr=(uint16_t)(s2/4);

        int32_t dl=(int32_t)vl-(int32_t)bl_l;
        int32_t dr=(int32_t)vr-(int32_t)bl_r;
        uint8_t tl=(dl>thr)?1:0;
        uint8_t tr=(dr>thr)?1:0;

        fb_clr();

        /* Строка 0: статус */
        if(tl&&tr)      fb_str(0,0,"BOTH  !");
        else if(tl)     fb_str(0,0,"LEFT   L");
        else if(tr)     fb_str(0,0,"RIGHT  R");
        else            fb_str(0,0,"NO TOUCH");

        /* Строка 1-2: левый канал */
        fb_str(0,1,"L A5:");  fb_u16(5,1,vl);
        fb_str(12,1,tl?"***":"   ");
        fb_str(0,2,"DL:"); fb_i32(3,2,dl);

        /* Строка 3-4: правый канал */
        fb_str(0,3,"R A14:"); fb_u16(6,3,vr);
        fb_str(12,3,tr?"***":"   ");
        fb_str(0,4,"DR:"); fb_i32(3,4,dr);

        /* Строка 5: baseline */
        fb_str(0,5,"BL:"); fb_u16(3,5,bl_l);
        fb_str(9,5,"/");   fb_u16(10,5,bl_r);

        /* Строка 6: порог */
        fb_str(0,6,"THR:"); fb_u16(4,6,(uint16_t)thr);

        /* Строка 7: полоска delta — масштаб до 2000 */
        {
            int32_t bl2=dl<0?0:(dl>2000?2000:dl);
            int32_t br2=dr<0?0:(dr>2000?2000:dr);
            uint8_t bars_l=(uint8_t)(bl2*60/2000);
            uint8_t bars_r=(uint8_t)(br2*60/2000);
            for(uint8_t px=0;px<bars_l;px++)
                g_fb[7*128+px]=0xFF;
            for(uint8_t px=0;px<bars_r;px++)
                g_fb[7*128+64+px]=0xFF;
        }

        oled_flush();

        wait(100000);
    }
}
