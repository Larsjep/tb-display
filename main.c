

// #include "stm8s.h"
// #include "STM8L152.h"
#include "STM8L152C4.h"
#include "string.h"


ISR_HANDLER(WakeupInterrupt, _RTC_WAKEUP_VECTOR_)
{
	/* in order to detect unexpected events during development, 
	   it is recommended to set a breakpoint on the following instruction
	*/
	sfr_RTC.ISR2.byte = 0;
	return;
}


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
	sfr_RTC.WPR.byte = 0xCA;
	sfr_RTC.WPR.byte = 0x53;
	
	sfr_CLK.ECKR.byte |= 1 << 2; // Turn on LSE clock
	while ((sfr_CLK.ECKR.LSERDY) == 0);
	
	sfr_RTC.ISR1.byte = (1 << 6); // Set INITF
	sfr_RTC.CR1.byte = (1 << 6) | (0); // Set 12 hour format
	sfr_RTC.ISR1.byte = (1 << 2); // Clear INITF and set WUTWF
	
	// Set wakeup to 1 second. Wake up timer clock 32768/16 = 2048
	sfr_RTC.WUTRH.byte = 2048 >> 8;
	sfr_RTC.WUTRL.byte = 2048 & 0xff;
	
	
	sfr_RTC.ISR2.byte = 0;
	sfr_RTC.CR3.byte = 3 << 5;
	sfr_RTC.CR2.byte |= (1 << 2) | (1 << 6); // Set Wakeup enable
	
	sfr_RTC.ISR1.byte = 0; // Clear init flags;

}

void clear_wakeup(void)
{
	sfr_RTC.ISR2.byte = 0;
}



void main(void)
{
  	u8 frame_buf[frame_size]; 

  	sfr_CLK.CKDIVR.byte = 0;
	sfr_CLK.CRTCR.byte = (8 << 1);
	sfr_CLK.PCKENR2.byte = (1 << 3) | (1 << 2); // Enable clock to RTC and LCD
		
  	sfr_LCD.FRQ.byte = (2 << 4) | (0); // Set prescalar to 16*(2^4)
	sfr_LCD.PM0.byte = 0xff;
	sfr_LCD.PM1.byte = 0x3f;
	sfr_LCD.PM2.byte = 0x0;
  	sfr_LCD.PM3.byte = 0x0;
	
  init_rtc();	
	
	//long_delay();
	//_asm("halt");
	
	memset(&sfr_LCD.RAM0.byte, 0xff, 14);
	sfr_LCD.RAM0.byte = 0x1;
	sfr_LCD.RAM0.byte = 0xff;
	
	sfr_LCD.CR1.byte = (0 << 6) | ( 4 << 3) | (3 << 1) | 1; // Blink all with Flcd/128, 1/2 duty, 1/3 bias
	sfr_LCD.CR2.byte = (4 << 5) | (4 << 1); // 4 clk pulses on duration, VLCD4 volatge, internal VLCD
	sfr_LCD.CR3.byte = (1 << 6) | (0); // Enable and no dead time, interrupt disable

	for(;;)
	{
		int i;
		for (i = 0; i < 8; i++)
		{
			volatile long x;
			memset(frame_buf, 0, frame_size);
			write_digit(frame_buf, digit1, i);
			write_digit(frame_buf, digit2, i);
			write_digit(frame_buf, digit3, i);
			write_digit(frame_buf, digit4, 1);
			memcpy(&sfr_LCD.RAM0.byte, frame_buf, frame_size);
		  for (x = 0; x < 80000L; x++);
			write_digit(frame_buf, digit4, 0);
			memcpy(&sfr_LCD.RAM0.byte, frame_buf, frame_size);
			clear_wakeup();
			ENTER_HALT();
			__asm__("nop");
			__asm__("nop");
			__asm__("nop");
			__asm__("nop");
			clear_wakeup();
		}		 
	}
}
