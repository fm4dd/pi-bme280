/* ------------------------------------------------------------ *
 * file:        getbme280.h                                     *
 * purpose:     header file for getbme280.c and i2c_bme280.c    *
 *                                                              *
 * author:      05/04/2018 Frank4DD                             *
 * ------------------------------------------------------------ */

#define I2CBUS        "/dev/i2c-1" // Raspi default I2C bus
#define BME280_ADDR        "0x76"  // The sensor default I2C addr
#define CHIP_ID              0x60  // BME280 responds with 0x60
#define POWER_MODE_NORMAL    0x00  // sensor default power mode
/* ------------------------------------------------------------ *
 * Calibration data 16 bytes 0xE1..0xF0, 26 bytes 0x88..0xA1    *
 * ------------------------------------------------------------ */
#define CALIB_BYTECOUNT      42      
#define REGISTERMAP_END      0x7F

/* ------------------------------------------------------------ *
 * Sensor register address information                          *
 * ------------------------------------------------------------ */
/* Humidity data register (read-only) */
#define BME280_HUMI_DATA_LSB_ADDR    0xFE
#define BME280_HUMI_DATA_MSB_ADDR    0xFD
/* Temperature data register (read-only) */
#define BME280_TEMP_DATA_XLSB_ADDR   0xFC
#define BME280_TEMP_DATA_LSB_ADDR    0xFB
#define BME280_TEMP_DATA_MSB_ADDR    0xFA
/* Pressure data register (read-only) */
#define BME280_PRES_DATA_XLSB_ADDR   0xF9
#define BME280_PRES_DATA_LSB_ADDR    0xF8
#define BME280_PRES_DATA_MSB_ADDR    0xF7
/* Config data register (read-write) */
#define BME280_CONFIG_ADDR           0xF5
/* ctrl-meas register (read-write) */
#define BME280_CTRL_MEAS_ADDR        0xF4
/* Status register (read-only) */
#define BME280_STATUS_ADDR           0xF3
/* ctrl-hum register (read-write) */
#define BME280_CTRL_HUM_ADDR         0xF2
/* Reset register (write-only) */
#define BME280_RESET_ADDR            0xE0
/* Chip ID register  (read-only) */
#define BME280_CHIP_ID_ADDR          0xD0
/* Calibration registers (read-only) */
#define BME280_CALIB_00_ADDR         0x88
#define BME280_CALIB_01_ADDR         0x89
#define BME280_CALIB_02_ADDR         0x8A
#define BME280_CALIB_03_ADDR         0x8B
#define BME280_CALIB_04_ADDR         0x8C
#define BME280_CALIB_05_ADDR         0x8D
#define BME280_CALIB_06_ADDR         0x8E
#define BME280_CALIB_07_ADDR         0x8F
#define BME280_CALIB_08_ADDR         0x90
#define BME280_CALIB_09_ADDR         0x91
#define BME280_CALIB_10_ADDR         0x92
#define BME280_CALIB_11_ADDR         0x93
#define BME280_CALIB_12_ADDR         0x94
#define BME280_CALIB_13_ADDR         0x95
#define BME280_CALIB_14_ADDR         0x96
#define BME280_CALIB_15_ADDR         0x97
#define BME280_CALIB_16_ADDR         0x98
#define BME280_CALIB_17_ADDR         0x99
#define BME280_CALIB_18_ADDR         0x9A
#define BME280_CALIB_19_ADDR         0x9B
#define BME280_CALIB_20_ADDR         0x9C
#define BME280_CALIB_21_ADDR         0x9D
#define BME280_CALIB_22_ADDR         0x9E
#define BME280_CALIB_23_ADDR         0x9F
#define BME280_CALIB_24_ADDR         0xA0
#define BME280_CALIB_25_ADDR         0xA1
/* Calibration registers 2nd set */
#define BME280_CALIB_26_ADDR         0xE1
#define BME280_CALIB_27_ADDR         0xE2
#define BME280_CALIB_28_ADDR         0xE3
#define BME280_CALIB_29_ADDR         0xE4
#define BME280_CALIB_30_ADDR         0xE5
#define BME280_CALIB_31_ADDR         0xE6
#define BME280_CALIB_32_ADDR         0xE7
#define BME280_CALIB_33_ADDR         0xE8
#define BME280_CALIB_34_ADDR         0xE9
#define BME280_CALIB_35_ADDR         0xEA
#define BME280_CALIB_36_ADDR         0xEB
#define BME280_CALIB_37_ADDR         0xEC
#define BME280_CALIB_38_ADDR         0x9D
#define BME280_CALIB_39_ADDR         0xEE
#define BME280_CALIB_40_ADDR         0xEF
#define BME280_CALIB_41_ADDR         0xF0

/* ------------------------------------------------------------ *
 * global variables                                             *
 * ------------------------------------------------------------ */
int i2cfd;       // I2C file descriptor
int verbose;     // debug flag, 0 = normal, 1 = debug mode

/* ------------------------------------------------------------ *
 * BME280 version, status and control data structure            *
 * ------------------------------------------------------------ */
struct bmeinf{
   char chip_id;     // reg 0xD0 returns 0x60 for type BME280
   char osrs_h_mode; // reg 0xF2 hum oversampling 2-0 bit 6x values
   char osrs_t_mode; // reg 0xF4 default 0x08 oversampling temp
   char osrs_p_mode; // reg 0xF4 default 0x08 oversampling press
   char power_mode;  // reg 0xF4, 0 = psleep, 1 = forced, 2 = forced, 3 = normal
   char stby_time;   // reg 0xF5 7-5 bit 8x values, range 0.5 ... 1000 ms
   char filter_mode; // reg 0xF5 4-2 bit 5x values 0 off, 1 = 2, 2 = 4, 3 = 8, 4 = 16
   char spi3we_mode; // reg 0xF5 0 bit 2 values 0 = off, 1 = on
};

/* ------------------------------------------------------------ *
 * BME280 calibration data struct. The values are set at prod.  *
 * time and cannnot be changed. Pressure and temperature have   *
 * 20bit values stored in 32bit signed int. Humidity is 16bit,  *
 * also stored in 32bit signed int.                             *
 * ------------------------------------------------------------ */
struct bmecal{
   uint16_t dig_T1;  // Temp calibration T1 reg 0x88 / 0x89
   int16_t  dig_T2;  // Temp calibration T2 reg 0x8A / 0x8B
   int16_t  dig_T3;  // Temp calibration T3 reg 0x8C / 0x8D
   uint16_t dig_P1;  // Pressure calibr. P1 reg 0x8E / 0x8F
   int16_t  dig_P2;  // Pressure calibr. P2 reg 0x90 / 0x91
   int16_t  dig_P3;  // Pressure calibr. P3 reg 0x92 / 0x93
   int16_t  dig_P4;  // Pressure calibr. P4 reg 0x94 / 0x95
   int16_t  dig_P5;  // Pressure calibr. P5 reg 0x96 / 0x97
   int16_t  dig_P6;  // Pressure calibr. P6 reg 0x98 / 0x99
   int16_t  dig_P7;  // Pressure calibr. P7 reg 0x9A / 0x9B
   int16_t  dig_P8;  // Pressure calibr. P8 reg 0x9C / 0x9D
   int16_t  dig_P9;  // Pressure calibr. P9 reg 0x9E / 0x9F
   uint8_t  dig_H1;  // Humidity calibr. H1 reg 0xA1
   int16_t  dig_H2;  // Humidity calibr. H2 reg 0xE1 / 0xE2
   uint8_t  dig_H3;  // Humidity calibr. H3 reg 0xE3
   int16_t  dig_H4;  // Humidity calibr. H4 reg 0xE4 / 0xE5
   int16_t  dig_H5;  // Humidity calibr. H5 reg 0xE5 / 0xE6
   int8_t   dig_H6;  // Humidity calibr. H6 reg 0xE7
};

/* ------------------------------------------------------------ *
 * BME280 measurement data struct.                              *
 * ------------------------------------------------------------ */
struct bmedata{
   float temp_c;   // compensated temperature in degreees Celsius
   float temp_f;   // compensated temperature in degrees Fahrenheit
   float humi_p;   // compensated humidity in percent
   float pres_p;   // compensated pressure in Pascal
};

/* ------------------------------------------------------------ *
 * Power mode name to value translation                         *
 * ------------------------------------------------------------ */
typedef enum {
   psleep = 0x00,
   forced = 0x01,
   force2 = 0x02,
   normal = 0x03
} power_t;

/* ------------------------------------------------------------ *
 * external function prototypes for I2C bus communication       *
 * ------------------------------------------------------------ */
extern void get_i2cbus(char*, char*);     // get the I2C bus file handle
extern int bme_dump();                    // dump the register map data
extern int bme_reset();                   // reset the sensor
extern void bme_info(struct bmeinf*);     // print sensor information
extern char get_chipid();                 // get the sensor chip id
extern void print_osrs(char);             // prints the oversampling rate
extern char get_power();                  // get the sensor power mode
extern int set_power(power_t);            // set the sensor power mode
extern void print_power(char);            // prints the sensor power mode
extern char get_h_osrs();                 // get humidity oversampling
extern int set_h_osrs(char*);             // set humidity oversampling
extern char get_p_osrs();                 // get pressure oversampling
extern int set_p_osrs(char*);             // set pressure oversampling
extern char get_t_osrs();                 // get temperat. oversampling
extern int set_t_osrs(char*);             // set temperat. oversampling
extern char get_stby();                   // get the sensor standby time
extern int set_stby(char*);               // set the sensor standby time
extern void print_stby(char);             // prints the sensor standby time
extern char get_filter();                 // get the IIR filter setting
extern int set_filter(char*);             // set the sensor IIR filter mode
extern void print_filter(char);           // prints the IIR filter setting
extern char get_spi3we();                 // get the SPI 3-Wire setting
extern void print_spi3we(char);           // prints the SPI 3-Wire setting
extern void get_calib();                  // get the sensor calibration data
extern void print_calib(struct bmecal*);  // prints the calibration data 
extern void get_data(struct bmecal*,      // get temp, humidity, and
                      struct bmedata*);   // pressure data
