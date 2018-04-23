#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "orangePi.h"
#include "wiringPi.h"


int ORANGEPI_PIN_MASK[12][32] =  //[BANK]  [INDEX]
{
	{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,},//PA
	{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,},//PB
	{ 0,-1, 2, 3,-1, 5,-1, 7, 8, 9,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,},//PC
	{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,21,22,23,24,25,26,-1,-1,-1,-1,-1,},//PD
	{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,},//PE
	{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,},//PF
	{-1,-1,-1,-1,-1,-1, 6, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,},//PG
	{-1,-1,-1, 3, 4, 5, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,},//PH
};

int physToWpiH6 [27] = 
{
  -1,           // 0
  -1, -1,       // 1, 2
   7, -1,
   6, -1,		
   8,  3,		//7, 8
  -1,  4,		
  10,  2,		//11, 12
   9, -1,		
  12,  1,		//15, 16
  -1,  0,		
  14, -1,		//19, 20
  15, 11,		
  13, 16,		//23, 24
  -1,  5,       // 25, 26
};

//pinToGpioR3 is for OrangePi H6 Lite2/OnePlus
int pinToGpioH6 [17] =
{
  71, 72, 73, 117, 118, 227, //GPIO 0 through 6:		wpi  0 -  5
  229, 230, 			// I2C1  - SCL1, SDA1				wpi  6 -  7
  228,				    // PWM1							wpi  8
  119, 120, 121, 122, 	// UART3 - Tx3, Rx3, RTS3, CTS3		wpi  9 - 12
  64, 66, 67, 69,		// SPI0  - SCLK, MOSI, MISO, CS		wpi  13 - 16
} ;

//physToGpioR3 is for OrangePi H6 Lite2/OnePlus
int physToGpioH6 [27] =
{
    -1,		// 0
    
    -1, -1,	// 1, 2
   230, -1,
   229, -1,
   228, 117,
    -1, 118,
   120, 73,
   119, -1,
   122, 72,
    -1, 71,
    66, -1,
    67, 121,
    64, 69,
    -1, 227,	// 25, 26
} ;

char *physNamesH6 [27] = 
{
  NULL,

  "     3.3v", "5v       ",
  "	    SDA1", "5v       ",
  "     SCL1", "0v       ",
  "	    PWM1", "PD21     ",
  "       0v", "PD22     ",
  " UART3_RX", "PC09     ",
  " UART3_TX", "0v       ",
  "UART3_CTS", "PC08     ",
  "     3.3v", "PC07     ",
  "SPI0_MOSI", "0v       ",
  "SPI0_MISO", "UART3_RTS",
  " SPI0_CLK", "SPI0_CS  ",
  "       0v", "PH03     ",
};


/*
 * Read value to register helper
 */
unsigned int readR(unsigned int addr)
{
	unsigned int val = 0;
	unsigned int mmap_base = (addr & ~MAP_MASK);
	unsigned int mmap_seek = ((addr - mmap_base) >> 2);

	val = *(_wiringPiGpio + mmap_seek);

	return val;
}

/*
 * Wirte value to register helper
 */
void writeR(unsigned int val, unsigned int addr)
{
	unsigned int mmap_base = (addr & ~MAP_MASK);
	unsigned int mmap_seek = ((addr - mmap_base) >> 2);

	*(_wiringPiGpio + mmap_seek) = val;
}

int OrangePi_set_gpio_mode(int pin, int mode)
{
	unsigned int regval = 0;
	unsigned int bank   = pin >> 5;  //算出 pin 属于哪一组  H6 有 2/3/5/6/7
	unsigned int index  = pin - (bank << 5); //算出 pin 属于组中的第多少个
	unsigned int phyaddr = 0;

	// (index - ((index >> 3) << 3)) 相当于获得除以 8 的余数，
	// offset 表示 GPIO 引脚的在寄存器中的偏移值，3 位可以配 + 1 位保留位，所以是左移 2 位
	int offset = ((index - ((index >> 3) << 3)) << 2);

	// (index >> 3) 算出属于哪个配置寄存器，每个寄存器包含 8 个 GPIO 引脚的设置，所以右移三位
	// ((index >> 3) << 2) 算出配置寄存器的偏移地址，每个寄存器 4 字节，所以是左移两位
	phyaddr = GPIO_BASE_MAP + (bank * 0x24) + ((index >> 3) << 2);

	/* Ignore unused gpio */
	if (ORANGEPI_PIN_MASK[bank][index] != -1) {
		/* Set Input */
		if(INPUT == mode) 
		{
			regval &= ~(7 << offset);
			writeR(regval, phyaddr);
			regval = readR(phyaddr);
			
			if (wiringPiDebug)
				printf("%s: Input mode set over reg val: %#x\n", __FUNCTION__, regval);
		} 
		else if(OUTPUT == mode) 
		{ 	/* Set Output */
			regval &= ~(7 << offset);
			regval |=  (1 << offset);
			
			if (wiringPiDebug)
				printf("%s: Out mode ready set val: 0x%x\n", __FUNCTION__, regval);
			writeR(regval, phyaddr);
			regval = readR(phyaddr);
			if (wiringPiDebug)
				printf("%s: Out mode get value end: 0x%x\n", __FUNCTION__, regval);
		}
		else 
			printf("Unknow mode\n");
		
		if (wiringPiDebug)
			printf("%s: Register[%#x]: %#x index:%d\n", __FUNCTION__, phyaddr, regval, index);
	}
	else
		printf("%s: Pin: %d mode failed!\n", __FUNCTION__, pin);

	return 0;
}

int OrangePi_get_gpio_mode(int pin)
{
	unsigned int regval = 0;
	unsigned int bank   = pin >> 5;  //算出 pin 属于哪一组  H6 有 2/3/5/6/7
	unsigned int index  = pin - (bank << 5); //算出 pin 属于组中的第多少个
	unsigned int phyaddr = 0;
	unsigned char mode = -1;

	int offset = ((index - ((index >> 3) << 3)) << 2);
	phyaddr = GPIO_BASE_MAP + (bank * 0x24) + ((index >> 3) << 2);

	/* Ignore unused gpio */
	if (ORANGEPI_PIN_MASK[bank][index] != -1) {
		regval = readR(phyaddr);
		mode = (regval >> offset) & 7;
	}

	return mode;
}

/*
 * OrangePi Digital write 
 */
int OrangePi_digitalWrite(int pin, int value)
{
    unsigned int bank   = pin >> 5;
    unsigned int index  = pin - (bank << 5);
    unsigned int phyaddr = 0;
    unsigned int regval = 0;

	//Pn_DAT: Offset = n * 0x0024 + 0x10
	phyaddr = GPIO_BASE_MAP + (bank * 0x24) + 0x10;

    /* Ignore unused gpio */
    if (ORANGEPI_PIN_MASK[bank][index] != -1) {
		
		regval = readR(phyaddr);
		if (wiringPiDebug)
			printf("befor write reg val: 0x%x,index:%d\n", regval, index);
		if(0 == value) 
		{
			regval &= ~(1 << index);
			writeR(regval, phyaddr);
			regval = readR(phyaddr);
			
			if (wiringPiDebug)
				printf("%s: LOW val set over reg val: 0x%x\n", __FUNCTION__, regval);
		} 
		else
		{
			regval |= (1 << index);
			writeR(regval, phyaddr);
			regval = readR(phyaddr);
			
			if (wiringPiDebug)
				printf("%s: HIGH val set over reg val: 0x%x\n", __FUNCTION__, regval);
		}
    } 
	else
        printf("%s: Pin mode failed!\n", __FUNCTION__);
	
    return 0;
}

/*
 * OrangePi Digital Read
 */
int OrangePi_digitalRead(int pin)
{
	int bank = pin >> 5;
	int index = pin - (bank << 5);
	int val;
	unsigned int phyaddr;
	
	phyaddr = GPIO_BASE_MAP + (bank * 0x24) + 0x10;

	if (ORANGEPI_PIN_MASK[bank][index] != -1) {
		val = readR(phyaddr);
		val = val >> index;
		val &= 1;
		
		if (wiringPiDebug)
		//	printf("Read reg val: 0x%#x, bank:%d, index:%d\n", val, bank, index);
		
		return val;
	}
	
	return 0;
}
