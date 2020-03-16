/* ------------------------------------------------------------ *
 * file:        i2c_bme280.c                                    *
 * purpose:     Extract sensor data from Bosch BME280 modules.  *
 *              Functions for I2C bus communication, get and    *
 *              set sensor register data. Ths file belongs to   *
 *              the pi-bme280 package. Functions are called     *
 *              from getbme280.c, globals are in getbme280.h.   *
 *                                                              *
 * Requires:	I2C development packages i2c-tools libi2c-dev   *
 *                                                              *
 * author:      03/10/2020 Frank4DD                             *
 * ------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include "getbme280.h"

/* ------------------------------------------------------------ *
 * get_i2cbus() - Enables the I2C bus communication. RPi 2,3,4  *
 * use /dev/i2c-1, RPi 1 used i2c-0, NanoPi Neo also uses i2c-0 *
 * ------------------------------------------------------------ */
void get_i2cbus(char *i2cbus, char *i2caddr) {

   if((i2cfd = open(i2cbus, O_RDWR)) < 0) {
      printf("Error failed to open I2C bus [%s].\n", i2cbus);
      exit(-1);
   }
   if(verbose == 1) printf("Debug: I2C bus device: [%s]\n", i2cbus);
   /* --------------------------------------------------------- *
    * Set I2C device (BME280 I2C address is 0x76 or 0xF77)      *
    * --------------------------------------------------------- */
   int addr = (int)strtol(i2caddr, NULL, 16);
   if(verbose == 1) printf("Debug: Sensor address: [0x%02X]\n", addr);

   if(ioctl(i2cfd, I2C_SLAVE, addr) != 0) {
      printf("Error can't find sensor at address [0x%02X].\n", addr);
      exit(-1);
   }
   /* --------------------------------------------------------- *
    * I2C communication test is the only way to confirm success *
    * --------------------------------------------------------- */
   if(get_chipid(addr) == 0) {
      printf("Error: No response from I2C. addr [0x%02X]?\n", addr);
      exit(-1);
   }
   if(verbose == 1) printf("Debug: Got data @addr: [0x%02X]\n", addr);
}

/* --------------------------------------------------------------- *
 * get_chipid() returns the chip id from register 0xD0.            *
 * --------------------------------------------------------------- */
char get_chipid() {
   char reg = BME280_CHIP_ID_ADDR;
   char buf = 0;
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
   }

   if(read(i2cfd, &buf, 1) != 1) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
   }
   return buf;
}

/* --------------------------------------------------------------- *
 * bme_dump() dumps the complete register map data (58 bytes).     *
 * --------------------------------------------------------------- */
int bme_dump() {
   char buf[31] = {0};  // 31 bytes is the max size, we read in step 3
   char reg;

   printf("------------------------------------------------------\n");
   printf("BME280 register dump:\n");
   printf("------------------------------------------------------\n");
   printf(" reg    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
   printf("------------------------------------------------------\n");

   /* ------------------------------------------------------ *
    * register data starts at address 0x88. For our display, * 
    * we start at 0x80, printing spaces to 0x87              * 
    * ------------------------------------------------------ */
   printf("[0x80]                         ");

   /* ------------------------------------------------------ *
    * Next read 26 bytes calibration, starting at addr 0x88  *
    * ------------------------------------------------------ */
   reg = 0x88;
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
      exit(-1);
   }

   if(read(i2cfd, &buf, 26) != 26) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
      exit(-1);
   }
   printf("%02X %02X %02X %02X %02X %02X %02X %02X\n",
          buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
   printf("[0x90] %02X %02X %02X %02X %02X %02X %02X %02X ",
          buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
   printf("%02X %02X %02X %02X %02X %02X %02X %02X\n",
          buf[16], buf[17], buf[18], buf[19], buf[20], buf[21], buf[22], buf[23]);
   printf("[0xA0] %02X %02X\n", buf[24], buf[25]);

   /* ------------------------------------------------------ *
    * Next we read 1 byte from address 0xD0                  *
    * ------------------------------------------------------ */
   memset(buf, 0, sizeof(buf)); // clear all data from buf
   reg = 0xD0;

   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
      exit(-1);
   }

   if(read(i2cfd, &buf, 1) != 1) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
      exit(-1);
   }
   printf("[0xD0] %02X\n", buf[0]);

   /* ------------------------------------------------------ *
    * Finally we read 31 bytes from address 0xE0             *
    * ------------------------------------------------------ */
   memset(buf, 0, sizeof(buf)); // clear all data from buf
   reg = 0xE0;

   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
      exit(-1);
   }

   if(read(i2cfd, &buf, 31) != 31) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
      exit(-1);
   }
   printf("[0xE0] %02X %02X %02X %02X %02X %02X %02X %02X ",
          buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
   printf("%02X %02X %02X %02X %02X %02X %02X %02X\n",
          buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
   printf("[0xF0] %02X %02X %02X %02X %02X %02X %02X %02X ",
          buf[16], buf[17], buf[18], buf[19], buf[20], buf[21], buf[22], buf[23]);
   printf("%02X %02X %02X %02X %02X %02X %02X\n",
          buf[24], buf[25], buf[26], buf[27], buf[28], buf[29], buf[30]);
   exit(0);
}

/* --------------------------------------------------------------- *
 * bme_reset() resets the sensor. This clears config data as well  *
 * --------------------------------------------------------------- */
int bme_reset() {
   char data[2];
   data[0] = BME280_RESET_ADDR;
   data[1] = 0xB6;
   if(write(i2cfd, data, 2) != 2) {
      printf("Error: I2C write failure for register 0x%02X\n", data[0]);
      exit(-1);
   }
   if(verbose == 1) printf("Debug: BME280 Sensor Reset complete\n");
   
   /* ------------------------------------------------------------ *
    * After a reset, the sensor needs at leat 2ms to boot up.      *
    * ------------------------------------------------------------ */
   usleep(2 * 1000);
   exit(0);
}

/* ------------------------------------------------------------ *
 * set_power() - set the sensor power mode in register 0xF4.    *
 * Because this is a multi-purpose control register, we get its *
 * content, manipulate the power bits, and write the register.  *
 * ------------------------------------------------------------ */
int set_power(power_t mode) {
   char reg = BME280_CTRL_MEAS_ADDR;
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
      return(-1);
   }

   char regdata = 0;
   if(read(i2cfd, &regdata, 1) != 1) {
      printf("Error: I2C read failure for register data 0x%02X\n", reg);
      return(-1);
   }

   if((regdata & 0x03) == mode) { // quit if new mode is already there
      if(verbose == 1) printf("Debug: existing pwr_mode was already set to [0x%02X]\n", mode);
      return(0);
   }

   switch(mode) {
      case 0x00:
         // SLEEP: clear bit-0 and bit-1 
         regdata &= ~(1 << 0); // bit-0
         regdata &= ~(1 << 1); // bit-1
         break;
      case 0x01:
         // FORCED: set bit-0 and clear bit-1 
         regdata |= 1 << 0;    // bit-0
         regdata &= ~(1 << 1); // bit-1
         break;
      case 0x02:
         // 0x2 also FORCED, we only set 0x1
         break;
      case 0x03:
         // NORMAL: set bit-0 and bit-1 
         regdata |= 1 << 0;    // bit-0
         regdata |= 1 << 1;    // bit-1
         break;
   }

   char buf[2] = {0};
   buf[0] = BME280_CTRL_MEAS_ADDR;
   buf[1] = regdata;
   if(verbose == 1) printf("Debug: Write pwr_mode: [0x%02X] to register [0x%02X]\n", buf[1], buf[0]);
   if(write(i2cfd, buf, 2) != 2) {
      printf("Error: I2C write failure for register 0x%02X\n", buf[0]);
      return(-1);
   }

   if(get_power() == mode) return(0); // check if new mode is set
   else return(-1);
}

/* ------------------------------------------------------------ *
 * get_power() returns the sensor power mode from register 0xF4 *
 * Only the lowest 2 bit are used, ignore the unused bits 2-7.  *
 * ------------------------------------------------------------ */
char get_power() {
   char reg = BME280_CTRL_MEAS_ADDR;
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
      return(-1);
   }

   char buf = 0;
   if(read(i2cfd, &buf, 1) != 1) {
      printf("Error: I2C read failure for register data 0x%02X\n", reg);
      return(-1);
   }

   if(verbose == 1) printf("Debug: Get power mode: [0x%02X] register [0x%02X]\n", buf & 0x03, buf);
   return(buf & 0x03);  // only return the lowest 2 bits
}

/* ------------------------------------------------------------ *
 * print_power() - prints the sensor power mode string from the *
 * sensors power mode numeric value.                            *
 * ------------------------------------------------------------ */
void print_power(char mode) {
   if(mode < 0 || mode > 3) exit(-1);

   switch(mode) {
      case 0x00:
         printf("SLEEP\n");
         break;
      case 0x01:
         printf("FORCED\n");
         break;
      case 0x02:
         printf("FORCED\n");
         break;
      case 0x03:
         printf("NORMAL\n");
         break;
   }
}

/* ------------------------------------------------------------ *
 * bme_info() - reads sensor configuration data from registers  *
 * 0xD0, 0xF2, 0xF3, 0xF4, 0xF5:                                *
 * char chip_id;     // reg 0xD0 returns 0x60 for type BME280   *
 * char osrs_h_mode; // reg 0xF2 hum oversampling 2-0 bit       *
 * char osrs_p_mode; // reg 0xF4 default 0x08 oversampling pres *
 * char osrs_t_mode; // reg 0xF4 default 0x08 oversampling temp *
 * char power_mode;  // reg 0xF4 0=sleep, 1,2=forced, 3=normal  *
 * char spi3we_mode; // reg 0xF5 bit-0, 2x values: 0=off, 1=on  *
 * char filter_mode; // reg 0xF5 4-2 bit 5x values              *
 * char stby_time;   // reg 0xF5 7-5 bit range 0.5 ... 1000 ms  *
 * ------------------------------------------------------------ */
void bme_info(struct bmeinf *bmei) {
   bmei->chip_id = get_chipid();
   bmei->osrs_h_mode = get_h_osrs();
   bmei->osrs_p_mode = get_p_osrs();
   bmei->osrs_t_mode = get_t_osrs();
   bmei->power_mode  = get_power();
   bmei->spi3we_mode = get_spi3we();
   bmei->filter_mode = get_filter();
   bmei->stby_time   = get_stby();
}

/* --------------------------------------------------------------- *
 * get_h_osrs() returns humidity settings from register 0xF2.      *
 * --------------------------------------------------------------- */
char get_h_osrs() {
   char reg = BME280_CTRL_HUM_ADDR;
   char buf = 0;
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
   }

   if(read(i2cfd, &buf, 1) != 1) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
   }
   if(verbose == 1) printf("Debug:  Humidity Mode: [0x%02X] 3bit [0x%02X]\n", buf, buf & 0x07);
   return(buf & 0x07);  // only return bit 0-2
}

/* --------------------------------------------------------------- *
 * get_p_osrs() returns pressure settings from register 0xF4.      *
 * --------------------------------------------------------------- */
char get_p_osrs() {
   char reg = BME280_CTRL_MEAS_ADDR;
   char buf = 0;
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
   }

   if(read(i2cfd, &buf, 1) != 1) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
   }

   if(verbose == 1) printf("Debug:  Pressure Mode: [0x%02X] 3bit [0x%02X]\n", buf, (buf >>2) & 0x07);
   return((buf >>2) & 0x07);  // only return bit 2-4
}

/* --------------------------------------------------------------- *
 * get_t_osrs() returns temperature settings from register 0xF4.   *
 * --------------------------------------------------------------- */
char get_t_osrs() {
   char reg = BME280_CTRL_MEAS_ADDR;
   char buf = 0;
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
   }

   if(read(i2cfd, &buf, 1) != 1) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
   }

   if(verbose == 1) printf("Debug: Temperat. Mode: [0x%02X] 3bit [0x%02X]\n", buf, (buf >>5) & 0x07);
   return((buf >>5) & 0x07);  // only return bit 5-7
}

/* --------------------------------------------------------------- *
 * set_h_osrs() sets the oversampling rate for humidity            *
 * --------------------------------------------------------------- */
int set_h_osrs(char *mode){
   char regdata = 0;

   if(strcmp(mode, "skip")    == 0) regdata = 0;
   else if(strcmp(mode, "1")  == 0) regdata = 1;
   else if(strcmp(mode, "2")  == 0) regdata = 2;
   else if(strcmp(mode, "4")  == 0) regdata = 3;
   else if(strcmp(mode, "8")  == 0) regdata = 4;
   else if(strcmp(mode, "16") == 0) regdata = 5;
   else {
      printf("Error: Unknown oversampling mode %s\n", mode);
      return(-1);
   }

   char buf[2] = {0};
   buf[0] = BME280_CTRL_HUM_ADDR;
   buf[1] = regdata;
   if(verbose == 1) printf("Debug: Write osrsmode: [0x%02X] to register [0x%02X]\n", buf[1], buf[0]);
   if(write(i2cfd, buf, 2) != 2) {
      printf("Error: I2C write failure for register 0x%02X\n", buf[0]);
      return(-1);
   }
   return(0);
}

/* --------------------------------------------------------------- *
 * set_t_osrs() sets the oversampling rate for temperature         *
 * Unlike humidity, temp and pressure use a multi-purpose register *
 * --------------------------------------------------------------- */
int set_t_osrs(char *mode){
   char reg = BME280_CTRL_MEAS_ADDR;
   char regdata = 0;
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
   }

   if(read(i2cfd, &regdata, 1) != 1) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
   }

   if(strcmp(mode, "skip")    == 0) {
      regdata &= ~(1 << 5); // bit-5
      regdata &= ~(1 << 6); // bit-6
      regdata &= ~(1 << 7); // bit-7
   }
   else if(strcmp(mode, "1")  == 0) {
      regdata |= 1 << 5;    // bit-5
      regdata &= ~(1 << 6); // bit-6
      regdata &= ~(1 << 7); // bit-7
   }
   else if(strcmp(mode, "2")  == 0) {
      regdata &= ~(1 << 5); // bit-5
      regdata |= 1 << 6;    // bit-6
      regdata &= ~(1 << 7); // bit-7
   }
   else if(strcmp(mode, "4")  == 0) {
      regdata |= 1 << 5;    // bit-5
      regdata |= 1 << 6;    // bit-6
      regdata &= ~(1 << 7); // bit-7
   }
   else if(strcmp(mode, "8")  == 0) {
      regdata &= ~(1 << 5); // bit-5
      regdata &= ~(1 << 6); // bit-6
      regdata |= 1 << 7;    // bit-7
   }
   else if(strcmp(mode, "16") == 0) {
      regdata &= ~(1 << 5); // bit-5
      regdata |= 1 << 6;    // bit-6
      regdata |= 1 << 7;    // bit-7
   }
   else {
      printf("Error: Unknown oversampling mode %s\n", mode);
      return(-1);
   }

   char buf[2] = {0};
   buf[0] = reg;
   buf[1] = regdata;
   if(verbose == 1) printf("Debug: Write osrsmode: [0x%02X] to register [0x%02X]\n", buf[1], buf[0]);
   if(write(i2cfd, buf, 2) != 2) {
      printf("Error: I2C write failure for register 0x%02X\n", buf[0]);
      return(-1);
   }
   return(0);
}

/* --------------------------------------------------------------- *
 * set_p_osrs() sets the oversampling rate for pressure            *
 * Unlike humidity, temp and pressure use a multi-purpose register *
 * --------------------------------------------------------------- */
int set_p_osrs(char *mode){
   char reg = BME280_CTRL_MEAS_ADDR;
   char regdata = 0;
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
   }

   if(read(i2cfd, &regdata, 1) != 1) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
   }

   if(strcmp(mode, "skip")    == 0) {
      regdata &= ~(1 << 2); // bit-2
      regdata &= ~(1 << 3); // bit-3
      regdata &= ~(1 << 4); // bit-4
   }
   else if(strcmp(mode, "1")  == 0) {
      regdata |= 1 << 2;    // bit-2
      regdata &= ~(1 << 3); // bit-3
      regdata &= ~(1 << 4); // bit-4
   }
   else if(strcmp(mode, "2")  == 0) {
      regdata &= ~(1 << 2); // bit-2
      regdata |= 1 << 3;    // bit-3
      regdata &= ~(1 << 4); // bit-4
   }
   else if(strcmp(mode, "4")  == 0) {
      regdata |= 1 << 2;    // bit-2
      regdata |= 1 << 3;    // bit-3
      regdata &= ~(1 << 4); // bit-4
   }
   else if(strcmp(mode, "8")  == 0) {
      regdata &= ~(1 << 2); // bit-2
      regdata &= ~(1 << 3); // bit-3
      regdata |= 1 << 4;    // bit-4
   }
   else if(strcmp(mode, "16") == 0) {
      regdata &= ~(1 << 2); // bit-2
      regdata |= 1 << 3;    // bit-3
      regdata |= 1 << 4;    // bit-4
   }
   else {
      printf("Error: Unknown oversampling mode %s\n", mode);
      return(-1);
   }

   char buf[2] = {0};
   buf[0] = reg;
   buf[1] = regdata;
   if(verbose == 1) printf("Debug: Write osrsmode: [0x%02X] to register [0x%02X]\n", buf[1], buf[0]);
   if(write(i2cfd, buf, 2) != 2) {
      printf("Error: I2C write failure for register 0x%02X\n", buf[0]);
      return(-1);
   }
   return(0);
}

/* --------------------------------------------------------------- *
 * get_spi3we() returns the SPI 3-Wire setting from register 0xF5. *
 * --------------------------------------------------------------- */
char get_spi3we() {
   char reg = BME280_CONFIG_ADDR;
   char buf = 0;
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
   }

   if(read(i2cfd, &buf, 1) != 1) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
   }

   if(verbose == 1) printf("Debug:  SPI 3-Wire On: [0x%02X] 2bit [0x%02X]\n", buf, buf & 0x01);
   return(buf & 0x01);  // only return bit 0
}

/* --------------------------------------------------------------- *
 * get_filter() returns the IIR filter setting from register 0xF5. *
 * --------------------------------------------------------------- */
char get_filter() {
   char reg = BME280_CONFIG_ADDR;
   char buf = 0;
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
   }

   if(read(i2cfd, &buf, 1) != 1) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
   }

   if(verbose == 1) printf("Debug: IIR Filter Set: [0x%02X] 3bit [0x%02X]\n", buf, (buf >>2) & 0x07);
   return((buf >>2) & 0x07);  // only return bit 2-4
}

/* --------------------------------------------------------------- *
 * set_filter() sets the IIR filter mode in register 0xF5.         *
 * This register is multi-purpose, requires to set individual bits *
 * --------------------------------------------------------------- */
int set_filter(char *mode) {
   char reg = BME280_CONFIG_ADDR;
   char regdata = 0;
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
   }

   if(read(i2cfd, &regdata, 1) != 1) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
   }

   if(strcmp(mode, "off")    == 0) {
      regdata &= ~(1 << 2); // bit-2
      regdata &= ~(1 << 3); // bit-3
      regdata &= ~(1 << 4); // bit-4
   }
   else if(strcmp(mode, "2")  == 0) {
      regdata |= 1 << 2;    // bit-2
      regdata &= ~(1 << 3); // bit-3
      regdata &= ~(1 << 4); // bit-4
   }
   else if(strcmp(mode, "4")  == 0) {
      regdata &= ~(1 << 2); // bit-2
      regdata |= 1 << 3;    // bit-3
      regdata &= ~(1 << 4); // bit-4
   }
   else if(strcmp(mode, "8")  == 0) {
      regdata |= 1 << 2;    // bit-2
      regdata |= 1 << 3;    // bit-3
      regdata &= ~(1 << 4); // bit-4
   }
   else if(strcmp(mode, "16")  == 0) {
      regdata &= ~(1 << 2); // bit-2
      regdata &= ~(1 << 3); // bit-3
      regdata |= 1 << 4;    // bit-4
   }
   else {
      printf("Error: Unknown IIR filter mode %s\n", mode);
      return(-1);
   }

   char buf[2] = {0};
   buf[0] = reg;
   buf[1] = regdata;
   if(verbose == 1) printf("Debug: Write IIR mode: [0x%02X] to register [0x%02X]\n", buf[1], buf[0]);
   if(write(i2cfd, buf, 2) != 2) {
      printf("Error: I2C write failure for register 0x%02X\n", buf[0]);
      return(-1);
   }
   return(0);
}

/* --------------------------------------------------------------- *
 * get_stby() returns the standby time from register 0xF5.         *
 * --------------------------------------------------------------- */
char get_stby() {
   char reg = BME280_CONFIG_ADDR;
   char buf = 0;
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
   }

   if(read(i2cfd, &buf, 1) != 1) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
   }

   if(verbose == 1) printf("Debug:   Standby Time: [0x%02X] 3bit [0x%02X]\n", buf, (buf >>5) & 0x07);
   return((buf >>5) & 0x07);  // only return bit 5-7
}

/* --------------------------------------------------------------- *
 * set_stby() sets the standby time in register 0xF5.              *
 * This register is multi-purpose, requires to set individual bits *
 * --------------------------------------------------------------- */
int set_stby(char *mode) {
   char reg = BME280_CONFIG_ADDR;
   char regdata = 0;
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
   }

   if(read(i2cfd, &regdata, 1) != 1) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
   }

   if(strcmp(mode, "0.5")    == 0) {
      regdata &= ~(1 << 5); // bit-5
      regdata &= ~(1 << 6); // bit-6
      regdata &= ~(1 << 7); // bit-7
   }
   else if(strcmp(mode, "62.5")  == 0) {
      regdata |= 1 << 5;    // bit-5
      regdata &= ~(1 << 6); // bit-6
      regdata &= ~(1 << 7); // bit-7
   }
   else if(strcmp(mode, "125")  == 0) {
      regdata &= ~(1 << 5); // bit-5
      regdata |= 1 << 6;    // bit-6
      regdata &= ~(1 << 7); // bit-7
   }
   else if(strcmp(mode, "250")  == 0) {
      regdata |= 1 << 5;    // bit-5
      regdata |= 1 << 6;    // bit-6
      regdata &= ~(1 << 7); // bit-7
   }
   else if(strcmp(mode, "500")  == 0) {
      regdata &= ~(1 << 5); // bit-5
      regdata &= ~(1 << 6); // bit-6
      regdata |= 1 << 7;    // bit-7
   }
   else if(strcmp(mode, "1000")  == 0) {
      regdata |= 1 << 5;    // bit-5
      regdata &= ~(1 << 6); // bit-6
      regdata |= 1 << 7;    // bit-7
   }
   else if(strcmp(mode, "10")  == 0) {
      regdata &= ~(1 << 5); // bit-5
      regdata |= 1 << 6;    // bit-6
      regdata |= 1 << 7;    // bit-7
   }
   else if(strcmp(mode, "20")  == 0) {
      regdata |= 1 << 5;    // bit-5
      regdata |= 1 << 6;    // bit-6
      regdata |= 1 << 7;    // bit-7
   }
   else {
      printf("Error: Unknown standby time value %s\n", mode);
      return(-1);
   }

   char buf[2] = {0};
   buf[0] = reg;
   buf[1] = regdata;
   if(verbose == 1) printf("Debug: Write stbytime: [0x%02X] to register [0x%02X]\n", buf[1], buf[0]);
   if(write(i2cfd, buf, 2) != 2) {
      printf("Error: I2C write failure for register 0x%02X\n", buf[0]);
      return(-1);
   }
   return(0);
}


/* ------------------------------------------------------------ *
 * print_osrs() print the oversampling setting for the numeric  *
 * values of hunidity, pressure and temperature.                *
 * ------------------------------------------------------------ */
void print_osrs(char mode) {
   if(mode < 0 || mode > 7) exit(-1);

   switch(mode) {
      case 0x00:
         printf("OFF (skip)\n");
         break;
      case 0x01:
         printf("1x\n");
         break;
      case 0x02:
         printf("2x\n");
         break;
      case 0x03:
         printf("4x\n");
         break;
      case 0x04:
         printf("8x\n");
         break;
      case 0x05:
         printf("16x\n");
         break;
      case 0x06:
         printf("16x\n");
         break;
      case 0x07:
         printf("16x\n");
         break;
   }
}

/* ------------------------------------------------------------ *
 * print_spi3we() - prints the SPI 3-Wire mode setting          *
 * ------------------------------------------------------------ */
void print_spi3we(char mode) {
   if(mode < 0 || mode > 1) exit(-1);
   if(mode == 0x00) printf("OFF\n");
   else printf("ON\n");
}

/* ------------------------------------------------------------ *
 * print_filter() - prints the IIR filter mode                  *
 * ------------------------------------------------------------ */
void print_filter(char mode) {
   if(mode < 0 || mode > 7) exit(-1);

   switch(mode) {
      case 0x00:
         printf("OFF\n");
         break;
      case 0x01:
         printf("2\n");
         break;
      case 0x02:
         printf("4\n");
         break;
      case 0x03:
         printf("8\n");
         break;
      case 0x04:
         printf("16\n");
         break;
      case 0x05:
         printf("16\n");
         break;
      case 0x06:
         printf("16\n");
         break;
      case 0x07:
         printf("16\n");
         break;
   }
}

/* ------------------------------------------------------------ *
 * print_stby() - prints the standby timer setting              *
 * ------------------------------------------------------------ */
void print_stby(char mode) {
   if(mode < 0 || mode > 7) exit(-1);

   switch(mode) {
      case 0x00:
         printf("0.5ms\n");
         break;
      case 0x01:
         printf("62.5ms\n");
         break;
      case 0x02:
         printf("125ms\n");
         break;
      case 0x03:
         printf("250ms\n");
         break;
      case 0x04:
         printf("500ms\n");
         break;
      case 0x05:
         printf("1s\n");
         break;
      case 0x06:
         printf("10ms\n");
         break;
      case 0x07:
         printf("20ms\n");
         break;
   }
}

/* --------------------------------------------------------------- *
 * get_calib() loads sensor calibration data into a global struct  *
 * --------------------------------------------------------------- */
void get_calib(struct bmecal *bmec) {

   /* ------------------------------------------------------------ *
    * Read 24 bytes calib00-23 calibration, register 0x88 - 0x9F   *
    * ------------------------------------------------------------ */
   char reg = BME280_CALIB_00_ADDR;
   char buf[24] = {0};
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
   }

   if(read(i2cfd, buf, 24) != 24) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
   }

   /* ------------------------------------------------------------ *
    * convert calibration register data to temperature coefficents *
    * ------------------------------------------------------------ */
   bmec->dig_T1 = (buf[0] + buf[1] * 256);
   bmec->dig_T2 = (buf[2] + buf[3] * 256);
   if(bmec->dig_T2 > 32767) bmec->dig_T2 -= 65536;
   bmec->dig_T3 = (buf[4] + buf[5] * 256);
   if(bmec->dig_T3 > 32767) bmec->dig_T3 -= 65536;

   /* ------------------------------------------------------------ *
    * convert calibration register data to pressure coefficents    *
    * ------------------------------------------------------------ */
   bmec->dig_P1 = (buf[6] + buf[7] * 256);
   bmec->dig_P2 = (buf[8] + buf[9] * 256);
   if(bmec->dig_P2 > 32767) bmec->dig_P2 -= 65536;
   bmec->dig_P3 = (buf[10] + buf[11] * 256);
   if(bmec->dig_P3 > 32767) bmec->dig_P3 -= 65536;
   bmec->dig_P4 = (buf[12] + buf[13] * 256);
   if(bmec->dig_P4 > 32767) bmec->dig_P4 -= 65536;
   bmec->dig_P5 = (buf[14] + buf[15] * 256);
   if(bmec->dig_P5 > 32767) bmec->dig_P5 -= 65536;
   bmec->dig_P6 = (buf[16] + buf[17] * 256);
   if(bmec->dig_P6 > 32767) bmec->dig_P6 -= 65536;
   bmec->dig_P7 = (buf[18] + buf[19] * 256);
   if(bmec->dig_P7 > 32767) bmec->dig_P7 -= 65536;
   bmec->dig_P8 = (buf[20] + buf[21] * 256);
   if(bmec->dig_P8 > 32767) bmec->dig_P8 -= 65536;
   bmec->dig_P9 = (buf[22] + buf[23] * 256);
   if(bmec->dig_P9 > 32767) bmec->dig_P9 -= 65536;

   /* ------------------------------------------------------------ *
    * convert calibration register data to humidity coefficents    *
    * ------------------------------------------------------------ */
   memset(buf, 0, sizeof(buf)); // clear buf
   reg = BME280_CALIB_25_ADDR;  // register 0xA1
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
   }

   if(read(i2cfd, buf, 1) != 1) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
   }
   bmec->dig_H1 = buf[0];

   /* ------------------------------------------------------------ *
    * Read 7 bytes of data from register(0xE1) calib26-32          *
    * ------------------------------------------------------------ */
   memset(buf, 0, sizeof(buf)); // clear buf
   reg = BME280_CALIB_26_ADDR; // register 0xE1
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
   }

   if(read(i2cfd, buf, 7) != 7) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
   }

   /* ------------------------------------------------------------ *
    * Convert the data: humidity coefficents                       *
    * ------------------------------------------------------------ */
   bmec->dig_H2 = (buf[0] + buf[1] * 256);
   if(bmec->dig_H2 > 32767) bmec->dig_H2 -= 65536;
   bmec->dig_H3 = buf[2] & 0xFF ;
   bmec->dig_H4 = (buf[3] * 16 + (buf[4] & 0xF));
   if(bmec->dig_H4 > 32767) bmec->dig_H4 -= 65536;
   bmec->dig_H5 = (buf[4] / 16) + (buf[5] * 16);
   if(bmec->dig_H5 > 32767) bmec->dig_H5 -= 65536;
   bmec->dig_H6 = buf[6];
   if(bmec->dig_H6 > 127) bmec->dig_H6 -= 256;
}

/* ------------------------------------------------------------ *
 * Get the data readings for Temp, Humidity and Pressure. For   *
 * compensation, make sure get_calib() has been called before.  *
 * ------------------------------------------------------------ */
void get_data(struct bmecal *bmec, struct bmedata *bmed) {
   memset(bmed, 0, sizeof &bmed);  // zero out the global data struct
   /* --------------------------------------------------------- *
    * Read the following 8 bytes from read-only data registers: *
    * 0xF7 press_msb (pressure msb)                             *
    * 0xF8 press_lsb (pressure lsb)                             *
    * 0xF9 press_xlsb (pressure xlsb, extend result to 20bit)   *
    * 0xFA temp_msb (temperature msb)                           *
    * 0xFB temp_lsb (temperature lsb)                           *
    * 0xFC temp_xlsb (temperature xlsb, extend result to 20bit) *
    * 0xFD hum_msb (humidity msb)                               *
    * 0xFB hum_lsb (humidity lsb)                               *
    * --------------------------------------------------------- */
   char reg = BME280_PRES_DATA_MSB_ADDR; // register 0xF7
   char buf[8] = {0};
   if(write(i2cfd, &reg, 1) != 1) {
      printf("Error: I2C write failure for register 0x%02X\n", reg);
   }

   if(read(i2cfd, buf, 8) != 8) {
      printf("Error: I2C read failure for register 0x%02X\n", reg);
   }
   /* ------------------------------------------------------------ *
    * Convert temperature and pressure data (20 bit)               *
    * ------------------------------------------------------------ */
   long adc_p = ((long)(buf[0] * 65536 + ((long)(buf[1] * 256) + (long)(buf[2] & 0xF0)))) / 16;
   long adc_t = ((long)(buf[3] * 65536 + ((long)(buf[4] * 256) + (long)(buf[5] & 0xF0)))) / 16;

   /* ------------------------------------------------------------ *
    * Convert the humidity data (16 bit)                           *
    * ------------------------------------------------------------ */
   long adc_h = (buf[6] * 256 + buf[7]);

   /* ------------------------------------------------------------ *
    * Temperature offset calculations                              *
    * ------------------------------------------------------------ */
   float var1 = (((float)adc_t)/16384.0 - ((float)bmec->dig_T1)/1024.0)*((float)bmec->dig_T2);
   float var2 = ((((float)adc_t)/131072.0 - ((float)bmec->dig_T1)/8192.0) *
                (((float)adc_t)/131072.0 - ((float)bmec->dig_T1)/8192.0)) * ((float)bmec->dig_T3);
   float t_fine = (long)(var1 + var2);

   /* ------------------------------------------------------------ *
    * temp_c = Temperature, temp_f = Fahrenheit                    *
    * ------------------------------------------------------------ */
   bmed->temp_c = (var1 + var2)/5120.0;
   if(verbose == 1) printf("Debug: Temperature: [%.2f*C]\n", bmed->temp_c);
   bmed->temp_f = bmed->temp_c * 1.8 + 32;

   /* ------------------------------------------------------------ *
    * Pressure offset calculations                                 *
    * ------------------------------------------------------------ */
   var1 = ((float)t_fine / 2.0) - 64000.0;
   var2 = var1 * var1 * ((float)bmec->dig_P6) / 32768.0;
   var2 = var2 + var1 * ((float)bmec->dig_P5) * 2.0;
   var2 = (var2 / 4.0) + (((float)bmec->dig_P4) * 65536.0);
   var1 = (((float)bmec->dig_P3) * var1 * var1/524288.0 + ((float)bmec->dig_P2) * var1)/524288.0;
   var1 = (1.0 + var1 / 32768.0) * ((float)bmec->dig_P1);
   float p = 1048576.0 - (float)adc_p;
   p = (p - (var2/4096.0)) * 6250.0/var1;
   var1 = ((float)bmec->dig_P9) * p * p/2147483648.0;
   var2 = p * ((float)bmec->dig_P8) / 32768.0;

   /* ------------------------------------------------------------ *
    * Pressure in Pascal (divide by 100 to get hPa)                *
    * ------------------------------------------------------------ */
   bmed->pres_p = (p + (var1+var2 + ((float)bmec->dig_P7))/16.0);
   if(verbose == 1) printf("Debug: Pressure: [%.2fPa]\n", bmed->pres_p);

   /* ------------------------------------------------------------ *
    * Humidity offset calculations
    * ------------------------------------------------------------ */
   float var_H = (((float)t_fine) - 76800.0);
   var_H = (adc_h - (bmec->dig_H4 * 64.0 + bmec->dig_H5 / 16384.0 * var_H)) *
   (bmec->dig_H2 / 65536.0 * (1.0 + bmec->dig_H6 / 67108864.0 * var_H *
   (1.0 + bmec->dig_H3 / 67108864.0 * var_H)));
   bmed->humi_p = var_H * (1.0 -  bmec->dig_H1 * var_H / 524288.0);

   if(bmed->humi_p > 100.0) bmed->humi_p = 100.0;
   else if(bmed->humi_p < 0.0) bmed->humi_p = 0.0;

   if(verbose == 1) printf("Debug: Rel Humidity: [%.2f%%]\n", bmed->humi_p);
}
