#ifndef	__ORANGEPI_PI_H__
#define	__ORANGEPI_PI_H__

#define OrangePiH6		1
#define RevH6 			66
#define	PI_MODEL_H6		66
#define PI_VERSION_2_0  66

#define GPIO_BASE_MAP                      (0x0300B000)

/****************** Global data *********************/
/* Current version */
#define MAP_SIZE           (4096 * 1)
#define MAP_MASK           (MAP_SIZE - 1)
/****************** Global data *********************/

extern volatile unsigned int *OrangePi_gpio;
extern int physToWpiH6[27];
extern int pinToGpioH6 [17];
extern int physToGpioH6 [27];
extern char *physNamesH6 [27];

extern unsigned int readR(unsigned int addr);
extern void writeR(unsigned int val, unsigned int addr);

extern int isOrangePi(void);
extern int OrangePi_digitalRead(int pin);
extern int OrangePi_digitalWrite(int pin, int value);
extern int OrangePi_set_gpio_mode(int pin, int mode);

#endif
