

// #include "stm8s.h"
#include "STM8L152.h"
#include "string.h"


typedef unsigned char u8;

u8 digit1[] = { 92, 63,  6,  7,  8, 64, 35 };
u8 digit2[] = { 93, 65, 36,  9, 10, 66, 38 };
u8 digit3[] = { 95, 67, 39, 11, 12, 68, 40 };
u8 digit4[] = { 97, 69, 41, 13,  0,  0,  0 };

u8 stars[] = { 33, 5 ,4 ,88, 60 };
u8 brush_pos[] = { 1, 85, 2, 31, 29 };

u8 seg7_digits[] = { 0x7E, 0x30, 0x6D, 0x79, 0x33, 0x5B, 0x5F, 0x70, 0x7F, 0x7B };


void write_digit(u8* buf, u8* digit, u8 value)
{
	int i;
	u8 segments = seg7_digits[value];
	for (i = 0; i <= 7; i++)
	{
		if (segments & (0x40 >> i))
		{
			u8 offset = digit[i] >> 3;
			u8 bit = digit[i] & 0x7;
			buf[offset] |= 1 << bit;
		}		
	}
	
}


#define frame_size 13

void delay(void)
{
	volatile long x;
	for (x = 0; x < 100L; x++);
}

void long_delay(void)
{
	volatile long x;
	for (x = 0; x < 100000L; x++);
}

void init_rtc(void)
{
	// Unprotect
	 RTC_WPR = 0xCA;
	 RTC_WPR = 0x53;
	
	CLK_ECKCR |= 1 << 2; // Turn on LSE clock
	while ((CLK_ECKCR & (1 << 3)) == 0);
	
	RTC_ISR1 = (1 << 6); // Set INITF
	RTC_CR1 = (1 << 6) | (0); // Set 12 hour format
	RTC_ISR1 = (1 << 2); // Clear INITF and set WUTWF
	
	// Set wakeup to 1 second. Wake up timer clock 32768/16 = 2048
	RTC_WUTRH = 2048 >> 8;
	RTC_WUTRL = 2048 & 0xff;
	
	
	RTC_ISR2 = 0;
	RTC_CR3 = 3 << 5;
	RTC_CR2 |= (1 << 2) | (1 << 6); // Set Wakeup enable
	
	RTC_ISR1 = 0; // Clear init flags;

}

void clear_wakeup(void)
{
	RTC_ISR2 = 0;
}



main()
{
  u8 frame_buf[frame_size]; 

  CLK_CKDIVR = 0;
	CLK_CRTCR = (8 << 1);
	CLK_PCKENR2 = (1 << 3) | (1 << 2); // Enable clock to RTC and LCD
		
  LCD_FRQ = (2 << 4) | (0); // Set prescalar to 16*(2^4)
	LCD_PM0 = 0xff;
	LCD_PM1 = 0x3f;
	LCD_PM2 = 0x0;
  LCD_PM3 = 0x0;
	
  init_rtc();	
	
	//long_delay();
	//_asm("halt");
	
	memset(&LCD_RAM0, 0xff, 14);
	LCD_RAM0 = 0x1;
	LCD_RAM0 = 0xff;
	
	LCD_CR1 = (0 << 6) | ( 4 << 3) | (3 << 1) | 1; // Blink all with Flcd/128, 1/2 duty, 1/3 bias
	LCD_CR2 = (4 << 5) | (4 << 1); // 4 clk pulses on duration, VLCD4 volatge, internal VLCD
	LCD_CR3 = (1 << 6) | (0); // Enable and no dead time, interrupt disable

	for(;;)
	{
		int i;
		for (i = 0; i < 5; i++)
		{
			volatile long x;
			memset(frame_buf, 0, frame_size);
			write_digit(frame_buf, digit1, i);
			write_digit(frame_buf, digit2, i);
			write_digit(frame_buf, digit3, i);
			write_digit(frame_buf, digit4, 1);
			memcpy(&LCD_RAM0, frame_buf, frame_size);
		  for (x = 0; x < 80000L; x++);
			write_digit(frame_buf, digit4, 0);
			memcpy(&LCD_RAM0, frame_buf, frame_size);
			clear_wakeup();
			_asm("halt");
			_asm("nop");
			_asm("nop");
			_asm("nop");
			_asm("nop");			
			clear_wakeup();
		}
		 
	}
	
}