

// #include "stm8s.h"
// #include "STM8L152.h"
#include "STM8L152C4.h"
#include "string.h"


ISR_HANDLER(WakeupInterrupt, _RTC_WAKEUP_VECTOR_)
{
	sfr_RTC.ISR2.byte = 0;
	return;
}


typedef unsigned char u8;

const u8 digit1[] = { 92, 63,  6,  7,  8, 64, 35 };
const u8 digit2[] = { 93, 65, 36,  9, 10, 66, 38 };
const u8 digit3[] = { 95, 67, 39, 11, 12, 68, 40 };
const u8 digit4[] = { 97, 69, 41, 13,  0,  0,  0 };

const u8 stars[] = { 33, 5 ,4 ,88, 60 };
const u8 brush_pos[] = { 1, 85, 2, 31, 29 };

const u8 seg7_digits[] = { 0x7E, 0x30, 0x6D, 0x79, 0x33, 0x5B, 0x5F, 0x70, 0x7F, 0x7B };

inline void set_segment(u8* buf, u8 segment)
{
	u8 offset = segment >> 3;
	u8 bit = segment & 0x7;
	buf[offset] |= 1 << bit;
}


void write_digit(u8* buf, u8* digit, u8 value)
{
	int i;
	value = value % 10;
	u8 segments = seg7_digits[value];
	for (i = 0; i < 7; i++)
	{
		if (segments & (0x40 >> i))
		{
			set_segment(buf, digit[i]);
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
	sfr_CLK.PCKENR2.PCKEN22 = 1; // Enable RTC clock

	// Unprotect
	sfr_RTC.WPR.byte = 0xCA;
	sfr_RTC.WPR.byte = 0x53;
	
	sfr_CLK.ECKR.LSEON = 1; // Turn on LSE clock
	while (sfr_CLK.ECKR.LSERDY == 0);

	sfr_RTC.CR1.byte = 0;
	sfr_RTC.CR2.byte = 0;
	sfr_RTC.CR3.byte = 0;

	while (sfr_RTC.ISR1.WUTWF == 0);
	
	//sfr_RTC.ISR1.byte = (1 << 7);
	//while ((sfr_RTC.ISR1.byte & (1 << 6)) == 0);
	sfr_RTC.ISR1.INIT = 1; // Set INIT
	while (sfr_RTC.ISR1.INITF == 0);
	//sfr_RTC.ISR1.INITF = 1; // Set INITF
	//long_delay();
	sfr_RTC.CR1.FMT = 1; // Set 12 hour format	
	sfr_RTC.CR1.WUCKSEL = 0;
	sfr_RTC.CR1.RATIO = 0;
	sfr_RTC.TR1.byte = 0;
	sfr_RTC.TR1.SU = 5;
	sfr_RTC.TR1.ST = 5;
	sfr_RTC.TR2.byte = 0;
	sfr_RTC.TR2.MNU =9;
	sfr_RTC.TR2.MNT =5;
	sfr_RTC.TR3.byte = 0;
	sfr_RTC.TR3.HU = 2;
	sfr_RTC.TR3.HT = 1;
	sfr_RTC.TR3.PM = 0;
	sfr_RTC.DR1.byte = 0;
	sfr_RTC.DR2.byte = 0;
	sfr_RTC.DR3.byte = 0;

	sfr_RTC.ISR1.INITF = 0;
	sfr_RTC.ISR1.INIT = 0;
	
	// Set wakeup to 1 second. Wake up timer clock 32768/16 = 2048
	const int wake_time = 2048;
	sfr_RTC.WUTRH.byte = wake_time >> 8;
	sfr_RTC.WUTRL.byte = wake_time & 0xff;
	
	sfr_RTC.ISR2.WUTF = 0;
	sfr_RTC.CR3.OSEL = 3; // Enable wake output
	sfr_RTC.CR2.WUTE = 1; // Set Wakeup enable
	sfr_RTC.CR2.WUTIE = 1; // Set Wakeup interrupt enable
}

void set_rtc(u8 hour, u8 min, u8 sec)
{
	sfr_RTC.ISR1.INIT = 1; // Set INIT
	while (sfr_RTC.ISR1.INITF == 0);


	sfr_RTC.TR1.byte = sec;
	sfr_RTC.TR2.byte = min;
	sfr_RTC.TR3.byte = hour;
	sfr_RTC.DR1.byte = 0;
	sfr_RTC.DR2.byte = 0;
	sfr_RTC.DR3.byte = 0;

	sfr_RTC.ISR1.INITF = 0;
	sfr_RTC.ISR1.INIT = 0;

}

void init_lcd(void)
{
	sfr_CLK.PCKENR2.PCKEN23 = 1; // Enable LCD clock
  	sfr_LCD.FRQ.byte = (2 << 4) | (0); // Set prescalar to 16*(2^4)
	sfr_LCD.PM0.byte = 0xff; // Enable 14 rows pins
	sfr_LCD.PM1.byte = 0x3f; //     - " -
	sfr_LCD.PM2.byte = 0x0;
  	sfr_LCD.PM3.byte = 0x0;

	sfr_LCD.CR1.byte = (0 << 6) | ( 4 << 3) | (3 << 1) | 1; // Blink all with Flcd/128, 1/2 duty, 1/3 bias
	sfr_LCD.CR2.byte = (4u << 5) | (4u << 1); // 4 clk pulses on duration, VLCD4 volatge, internal VLCD
	sfr_LCD.CR3.byte = (1 << 6) | (0); // Enable and no dead time, interrupt disable

}

void init_clocks(void)
{
	sfr_CLK.CKDIVR.byte = 0;
	sfr_CLK.CRTCR.byte = (8 << 1);
}


u8 frame_buf[frame_size]; 
uint8_t lastMin = 0xff;

void clock_update(void)
{
	static uint8_t dots = 0; // The state of the 2 dots between hours and minutes

	sfr_RTC.DR3.byte;
	sfr_RTC.ISR1.RSF = 0;
	while (sfr_RTC.ISR1.RSF == 0);

	uint8_t sec = sfr_RTC.TR1.byte;
	uint8_t min = sfr_RTC.TR2.byte;
	uint8_t hour = sfr_RTC.TR3.byte;
	uint8_t year = sfr_RTC.DR3.byte;

	dots = !dots;

	if (min != lastMin)
	{		
		memset(frame_buf, 0, frame_size);
		write_digit(frame_buf, digit1, min & 0xf);
		write_digit(frame_buf, digit2, min >> 4);
		write_digit(frame_buf, digit3, hour & 0xf);
		if (((hour >> 4) & 3) > 0)
		{
			write_digit(frame_buf, digit4, 1);
		}
		
		if (dots)
		{
			set_segment(frame_buf, 94);
		}

		memcpy(&sfr_LCD.RAM0.byte, frame_buf, frame_size);
		lastMin = min;
	}
	else
	{
		// For some reason a delay is needed after wakeup before the display can be updated
		for (int i = 0; i < 30; i++)
		{
			__asm__("nop");
		}

		if (dots)
		{
			sfr_LCD.RAM11.byte |= 1 << 6;
		}
		else
		{
			sfr_LCD.RAM11.byte &= ~(1 << 6);
		}
	}
}

void write_lcd(uint8_t d4, uint8_t d3, uint8_t d2, uint8_t d1)
{
		int seconds = d1 + 10*d2 + 60*d3 + 600*d4;		
		memset(frame_buf, 0, frame_size);
		set_segment(frame_buf, brush_pos[0]);
		set_segment(frame_buf, brush_pos[1]);
		set_segment(frame_buf, stars[0]);
		if (seconds >= 30)
		{
			set_segment(frame_buf, brush_pos[2]);
			set_segment(frame_buf, stars[1]);
		}
		if (seconds >= 60)
		{
			set_segment(frame_buf, brush_pos[3]);
			set_segment(frame_buf, stars[2]);
		}
		if (seconds >= 90)
		{
			set_segment(frame_buf, brush_pos[4]);
			set_segment(frame_buf, stars[3]);
		}
		if (seconds >= 120)
		{
			set_segment(frame_buf, stars[4]);
			set_segment(frame_buf, 30);
			set_segment(frame_buf, 58);
			set_segment(frame_buf, 57);
			set_segment(frame_buf, 87);
			set_segment(frame_buf, 86);
		}

		if (seconds & 1)
		{
			set_segment(frame_buf, 94);
		}


		write_digit(frame_buf, digit1, d1);
		write_digit(frame_buf, digit2, d2);
		write_digit(frame_buf, digit3, d3);

		if (d4 > 0)
		{
			write_digit(frame_buf, digit4, 1);
		}

		memcpy(&sfr_LCD.RAM0.byte, frame_buf, frame_size);
}


enum State { NONE, CLOCK, TIMER };
volatile enum State state = CLOCK;


enum SerialCommand { CMD_SETTIME = 0x2A, CMD_CLOCK, CMD_TIMER };
void execute_serial_command(enum SerialCommand cmd, u8 data[3])
{
	switch (cmd)
	{
	case CMD_SETTIME:
		set_rtc(data[0], data[1], data[2]);
		state = CLOCK;
		lastMin = 0xff;
		break;

	case CMD_CLOCK:
		state = CLOCK;
		lastMin = 0xff;
		break;
	case CMD_TIMER:
		write_lcd(data[0] >> 4, data[0] & 0xf, data[1] >> 4, data[1] & 0xf);
		state = TIMER;
		ENTER_HALT();
		break;
	
	default:
		break;
	}
}


enum SerialState { SS_IDLE, SS_CMD, SS_DATA } serialState = SS_IDLE;

ISR_HANDLER(SerialRxInterrupt, _USART_R_RXNE_VECTOR_)
{
	u8 status = sfr_USART1.SR.byte;
	u8 serial_char = sfr_USART1.DR.byte;
	static u8 data[3];
	static u8 data_left;
	static u8 data_pos;
	static enum SerialCommand serialCommand;

	switch (serialState)
	{
		case SS_IDLE:
			if (serial_char == 0xAA)
			{
				serialState = SS_CMD;
			}
			break;
		case SS_CMD:
			data_left = 0;
			data_pos = 0;
			if (serial_char == CMD_SETTIME || serial_char == CMD_CLOCK || serial_char == CMD_TIMER)
			{
				if (serial_char == CMD_SETTIME)
				{
					data_left = 3;
				}

				if (serial_char == CMD_TIMER)
				{
					data_left = 2;
				}

				serialCommand = serial_char;
				serialState = SS_DATA;

				if (data_left == 0)
				{
					serialState = SS_IDLE;
					execute_serial_command(serialCommand, data);
				}
			}
			else
			{
				serialState = SS_IDLE;
			}
			break;
		case SS_DATA:
			data[data_pos++] = serial_char;
			data_left--;
			if (data_left == 0)
			{
				serialState = SS_IDLE;
				execute_serial_command(serialCommand, data);
			}
			break;
	}
}


ISR_HANDLER(Exti3Interrupt, _EXTI3_VECTOR_)
{
	sfr_ITC_EXTI.SR1.P3F = 1;
	state = NONE;
}

void init_serial_port(void)
{
  	sfr_CLK.PCKENR1.PCKEN15 = 1;
	sfr_REMAP.SYSCFG_RMPCR1.USART1TR_REMAP = 1;
	sfr_USART1.BRR2.byte = 0xB;
	sfr_USART1.BRR1.byte = 0x8;
	sfr_USART1.CR1.byte = 0;
	sfr_USART1.CR3.byte = 0;
	sfr_USART1.CR5.byte = 0;

	sfr_PORTA.CR2.C23 = 1; // Enable interrupt on port A bit 3, that is also uart RX pin.

	sfr_USART1.CR2.RIEN = 1;
	sfr_USART1.CR2.REN = 1;
}

void main(void)
{

	init_clocks();
  	init_rtc();
	init_lcd();

	init_serial_port();
	write_lcd(0,1,3,7);
	sfr_ITC_EXTI.SR1.P3F = 1; // Clear any pending interrupt on port bit 3
	sfr_ITC_EXTI.CR1.P3IS = 0x2; // Only interrupt on falling edge on port bit 3.
	ENABLE_INTERRUPTS();

	for(;;)
	{
		switch (state)
		{
			case CLOCK:
				clock_update();				
				ENTER_HALT();
			break;
		}		 
	}
}
