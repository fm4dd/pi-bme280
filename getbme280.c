/* ------------------------------------------------------------ *
 * file:        getbme280.c                                     *
 * purpose:     Sensor control and data extraction program      *
 *              for the Bosch BME280 Pressure, Humidity and     *
 *              Temperature Sensor                              *
 *                                                              *
 * return:      0 on success, and -1 on errors.                 *
 *                                                              *
 * requires:	I2C headers, e.g. sudo apt install libi2c-dev   *
 *                                                              *
 * compile:	gcc -o getbme280 i2c_bme280.c getbme280.c       *
 *                                                              *
 * example:	./getbme280 -t -o bme280.htm                    *
 *                                                              *
 * author:      03/10/2020 Frank4DD                             *
 * ------------------------------------------------------------ */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include "getbme280.h"

/* ------------------------------------------------------------ *
 * Global variables and defaults                                *
 * ------------------------------------------------------------ */
int verbose = 0;
int outflag = 0;
int argflag = 0; // 1=dump, 2=info, 3=reset, 4=data, 5=continuous
char osrs_mode[7] = {0};  // oversampling mode
char pwr_mode[7]  = {0};  // power mode
char iir_mode[4]  = {0};  // IIR filter mode
char stby_time[5] = {0};  // standby time
char senaddr[256] = BME280_ADDR;
char i2c_bus[256] = I2CBUS;
char htmfile[256] = {0};

/* ------------------------------------------------------------ *
 * print_usage() prints the programs commandline instructions.  *
 * ------------------------------------------------------------ */
void usage() {
   static char const usage[] = "Usage: getbme280 [-a hex i2c-addr] [-b i2c-bus] [-d] [-i] [-m osrs_mode] [-p pwrmode] [-t] [-c] [-r] [-o htmlfile] [-v]\n\
\n\
Command line parameters have the following format:\n\
   -a   sensor I2C bus address in hex, Example: -a 0x76 (default)\n\
   -b   I2C bus to query, Example: -b /dev/i2c-1 (default)\n\
   -d   dump the complete sensor register map content\n\
   -f   set sensor IIR filter mode. arguments: <coefficient>. examples:\n\
              off = disabled, 1 sample to reach >=75%% of step response\n\
                2 = 2 samples to reach >= 75%% of step response\n\
                4 = 5 samples to reach >= 75%% of step response\n\
          valid settings: off, 2, 4, 8, 16\n\
   -i   print sensor information (config and calibration)\n\
   -m   set sensor oversampling mode. arguments: <type>-<rate>. examples:\n\
          t-skip  = disable the temperature measurement\n\
             t-1  = temperature 1x oversampling\n\
             h-2  = humidity 2x oversampling\n\
             p-4  = pressure 4x oversampling\n\
          valid types: t=temperature, h=humidity, p=pressure\n\
          valid oversampling rates: skip, 1, 2, 4, 8, 16\n\
   -p   set sensor power mode. arguments:\n\
          normal  = cycle between measuring and standby\n\
          forced  = take a single measurement and return to sleep\n\
          sleep   = no measurements (default after power-up)\n\
   -r   reset sensor\n\
   -s   set sensor standby time for power mode normal. arguments: <ms>\n\
          valid ms settings: 0.5, 10, 20, 62.5, 125, 250, 500, 1000\n\
   -t   read and output single measurement (power mode forced)\n\
   -c   read and output continuous measurements (power mode normal, 1sec interval)\n\
   -o   output data to HTML table file (requires -t/-c), example: -o ./bme280.html\n\
   -h   display this message\n\
   -v   enable debug output\n\
\n\
\n\
Usage examples:\n\
./getbme280 -a 0x77 -b /dev/i2c-0 -i\n\
./getbme280 -t -v\n\
./getbme280 -c\n\
./getbme280 -t -o ./bme280.html\n\n";
   printf(usage);
}

/* ------------------------------------------------------------ *
 * parseargs() checks the commandline arguments with C getopt   *
 * ------------------------------------------------------------ */
void parseargs(int argc, char* argv[]) {
   int arg;
   opterr = 0;

   if(argc == 1) { usage(); exit(-1); }

   while ((arg = (int) getopt (argc, argv, "a:b:cdf:im:p:rs:to:hv")) != -1) {
      switch (arg) {
         // arg -v verbose, type: flag, optional
         case 'v':
            verbose = 1; break;

         // arg -a + sensor address, type: string
         // mandatory, example: 0x76
         case 'a':
            if(verbose == 1) printf("Debug: arg -a, value %s\n", optarg);
            if (strlen(optarg) != 4) {
               printf("Error: Cannot get valid -a sensor address argument.\n");
               exit(-1);
            }
            strncpy(senaddr, optarg, sizeof(senaddr));
            break;

         // arg -b + I2C bus, type: string
         // optional, example: "/dev/i2c-1"
         case 'b':
            if(verbose == 1) printf("Debug: arg -b, value %s\n", optarg);
            if (strlen(optarg) >= sizeof(i2c_bus)) {
               printf("Error: I2C bus argument to long.\n");
               exit(-1);
            }
            strncpy(i2c_bus, optarg, sizeof(i2c_bus));
            break;

         // arg -d
         // optional, dumps the complete register map data
         case 'd':
            if(verbose == 1) printf("Debug: arg -d\n");
            argflag = 1;
            break;

         // arg -f + IIR filter mode, type: string off,2,4,8, or 16
         case 'f':
            if(verbose == 1) printf("Debug: arg -f, value %s\n", optarg);
            if (strlen(optarg) >= sizeof(iir_mode)) {
               printf("Error: IIR filter argument to long.\n");
               exit(-1);
            }
            strncpy(iir_mode, optarg, sizeof(iir_mode));
            break;

         // arg -i
         // optional, prints sensor information
         case 'i':
            if(verbose == 1) printf("Debug: arg -i\n");
            argflag = 2;
            break;

         // arg -m sets operations mode, type: string
         case 'm':
            if(verbose == 1) printf("Debug: arg -m, value %s\n", optarg);
            if (strlen(optarg) >= sizeof(osrs_mode)) {
               printf("Error: oversampling argument to long.\n");
               exit(-1);
            }
            if(optarg[0] != 't' && optarg[0] != 'h' && optarg[0] != 'p') {
               printf("Error: oversampling arg should start with t,h, or p.\n");
               exit(-1);
            }
            if(optarg[1] != '-') {
               printf("Error: oversampling arg should be t-,h-, or p-.\n");
               exit(-1);
            }
            strncpy(osrs_mode, optarg, sizeof(osrs_mode));
            break;

         // arg -p sets power mode, type: string
         case 'p':
            if(verbose == 1) printf("Debug: arg -p, value %s\n", optarg);
            if (strlen(optarg) >= sizeof(pwr_mode)) {
               printf("Error: power mode argument to long.\n");
               exit(-1);
            }
            strncpy(pwr_mode, optarg, sizeof(pwr_mode));
            break;

         // arg -r
         // optional, resets sensor
         case 'r':
            if(verbose == 1) printf("Debug: arg -r\n");
            argflag = 3;
            break;

         // arg -s sets standby time, type: string
         case 's':
            if(verbose == 1) printf("Debug: arg -s, value %s\n", optarg);
            if (strlen(optarg) >= sizeof(stby_time)) {
               printf("Error: standby time argument to long.\n");
               exit(-1);
            }
            strncpy(stby_time, optarg, sizeof(stby_time));
            break;

         // arg -t reads the sensor data
         case 't':
            if(verbose == 1) printf("Debug: arg -t\n");
            argflag = 4;
            break;

         // arg -c reads sensor data continuously
         case 'c':
            if(verbose == 1) printf("Debug: arg -c\n");
            argflag = 5;
            break;

         // arg -o + dst HTML file, type: string, requires -t
         // writes the sensor output to file. example: /tmp/sensor.htm
         case 'o':
            outflag = 1;
            if(verbose == 1) printf("Debug: arg -o, value %s\n", optarg);
            if (strlen(optarg) >= sizeof(htmfile)) {
               printf("Error: html file argument to long.\n");
               exit(-1);
            }
            strncpy(htmfile, optarg, sizeof(htmfile));
            break;

         // arg -h usage, type: flag, optional
         case 'h':
            usage(); exit(0);
            break;

         case '?':
            if(isprint (optopt))
               printf ("Error: Unknown option `-%c'.\n", optopt);
            else
               printf ("Error: Unknown option character `\\x%x'.\n", optopt);
            usage();
            exit(-1);
            break;

         default:
            usage();
            break;
      }
   }
}

int main(int argc, char *argv[]) {
   int res = -1;       // res = function retcode: 0=OK, -1 = Error

   /* ---------------------------------------------------------- *
    * Process the cmdline parameters                             *
    * ---------------------------------------------------------- */
   parseargs(argc, argv);

   /* ----------------------------------------------------------- *
    * get current time (now), write program start if verbose      *
    * ----------------------------------------------------------- */
   time_t tsnow = time(NULL);
   if(verbose == 1) printf("Debug: ts=[%lld] date=%s", (long long) tsnow, ctime(&tsnow));

   /* ----------------------------------------------------------- *
    * "-a" open the I2C bus and connect to the sensor i2c address *
    * ----------------------------------------------------------- */
   get_i2cbus(i2c_bus, senaddr);

   /* ----------------------------------------------------------- *
    *  "-d" dump the register map content and exit the program    *
    * ----------------------------------------------------------- */
    if(argflag == 1) {
      res = bme_dump();
      if(res != 0) {
         printf("Error: could not dump the register maps.\n");
         exit(-1);
      }
      exit(0);
   }

   /* ----------------------------------------------------------- *
    *  "-i" print sensor information and exit the program         *
    * ----------------------------------------------------------- */
    if(argflag == 2) {
      struct bmeinf bmei = {0};
      struct bmecal bmec = {0};
      bme_info(&bmei);
      get_calib(&bmec);

      /* ----------------------------------------------------------- *
       * print the formatted output strings to stdout                *
       * ----------------------------------------------------------- */
      printf("----------------------------------------------\n");
      printf("BME280 Information at %s", ctime(&tsnow));
      printf("----------------------------------------------\n");
      printf("    Sensor Chip ID = 0x%02X ", bmei.chip_id);
      if(bmei.chip_id == 0x60) printf("BME280\n");
      else if(bmei.chip_id == 0x58) printf("BMP280\n");
      else if(bmei.chip_id == 0x56) printf("BMP280 Sample\n");
      else if(bmei.chip_id == 0x57) printf("BMP280\n");
      else printf("ChipID unknown\n");
      printf("     Humidity Mode = "); print_osrs(bmei.osrs_h_mode);
      printf("     Pressure Mode = "); print_osrs(bmei.osrs_p_mode);
      printf("  Temperature Mode = "); print_osrs(bmei.osrs_t_mode);
      printf("      Standby Time = "); print_stby(bmei.stby_time);
      printf("   IIR Filter Mode = "); print_filter(bmei.filter_mode);
      printf("   3-wire SPI Mode = "); print_spi3we(bmei.spi3we_mode);
      printf("        Power Mode = "); print_power(bmei.power_mode);
      printf(" Temperature Coeff = T1:%6d T2:%6d T3:%5d\n",
             bmec.dig_T1, bmec.dig_T2, bmec.dig_T3);
      printf("    Pressure Coeff = P1:%6d P2:%6d P3:%5d\n",
             bmec.dig_P1, bmec.dig_P2, bmec.dig_P3);
      printf("                     P4:%6d P5:%6d P6:%5d\n",
             bmec.dig_P4, bmec.dig_P5, bmec.dig_P6);
      printf("                     P7:%6d P8:%6d P9:%5d\n",
             bmec.dig_P7, bmec.dig_P8, bmec.dig_P9);
      printf("    Humidity Coeff = H1:%6d H2:%6d H3:%5d\n",
             bmec.dig_H1, bmec.dig_H2, bmec.dig_H3);
      printf("                     H4:%6d H5:%6d H6:%5d\n",
             bmec.dig_H4, bmec.dig_H5, bmec.dig_H6);
      exit(0);
   }

   /* ----------------------------------------------------------- *
    *  "-r" reset the sensor and exit the program                 *
    * ----------------------------------------------------------- */
    if(argflag == 3) {
      res = bme_reset();
      if(res != 0) {
         printf("Error: could not reset the sensor.\n");
         exit(-1);
      }
      exit(0);
   }

   /* ----------------------------------------------------------- *
    *  "-f" set the sensor IIR filter mode and exit the program   *
    * ----------------------------------------------------------- */
   if(strlen(iir_mode) > 0) {
      res = set_filter(iir_mode);

      if(res != 0) {
         printf("Error: could not set IIR filter mode [%s].\n", iir_mode);
         exit(-1);
      }
      exit(0);
   }

   /* ----------------------------------------------------------- *
    *  "-m" set the sensor oversampling mode and exit the program *
    * ----------------------------------------------------------- */
   if(strlen(osrs_mode) > 0) {
      char type = osrs_mode[0];

      if(verbose == 1) printf("Debug: Measuring type: [%c]\n", type);
      if(verbose == 1) printf("Debug: Set osrs value: [%s]\n", &osrs_mode[2]);

      if(type == 't') res = set_t_osrs(&osrs_mode[2]);
      if(type == 'h') res = set_h_osrs(&osrs_mode[2]);
      if(type == 'p') res = set_p_osrs(&osrs_mode[2]);

      if(res != 0) {
         printf("Error: could not set oversampling mode [%s].\n", osrs_mode);
         exit(-1);
      }
      exit(0);
   }

   /* ----------------------------------------------------------- *
    *  "-p" set the sensor power mode and exit the program        *
    * ----------------------------------------------------------- */
   if(strlen(pwr_mode) > 0) {
      power_t newmode;
      if(strcmp(pwr_mode, "normal")   == 0)     newmode = normal;
      else if(strcmp(pwr_mode, "forced")  == 0) newmode = forced;
      else if(strcmp(pwr_mode, "sleep")  == 0)  newmode = psleep;
      else {
         printf("Error: invalid power mode %s.\n", pwr_mode);
         exit(-1);
      }

      res = set_power(newmode);
      if(res != 0) {
         printf("Error: could not set power mode %s [0x%02X].\n", pwr_mode, newmode);
         exit(-1);
      }
      exit(0);
   }
   /* ----------------------------------------------------------- *
    *  "-s" set the sensor standby time and exit the program      *
    * ----------------------------------------------------------- */
   if(strlen(stby_time) > 0) {
      res = set_stby(stby_time);

      if(res != 0) {
         printf("Error: could not set standby time %s.\n", stby_time);
         exit(-1);
      }
      exit(0);
   }
   /* ----------------------------------------------------------- *
    *  "-t" reads, calculates and prints compensated sensor data  *
    * ----------------------------------------------------------- */
   if(argflag == 4) {
      struct bmecal bmec;
      struct bmedata bmed;

      /* -------------------------------------------------------- *
       * If power mode SLEEP, set power mode FORCED to read once  *
       * -------------------------------------------------------- */
      if(get_power() == 0x0) res = set_power(forced);

      get_calib(&bmec);
      get_data(&bmec, &bmed);

      /* ----------------------------------------------------------- *
       * print the formatted output string to stdout (Example below) *
       * 1584280335 Temp=22.76*C Humidity=22.30% Pressure=1002.56hPa *
       * ----------------------------------------------------------- */
         printf("%lld Temp=%3.2f*C Humidity=%3.2f%% Pressure=%3.2fhPa\n", 
                (long long) tsnow, bmed.temp_c, bmed.humi_p, bmed.pres_p/100);

      if(outflag == 1) {
         /* -------------------------------------------------------- *
          *  Open the html file for writing accelerometer data       *
          * -------------------------------------------------------- */
         FILE *html;
         if(! (html=fopen(htmfile, "w"))) {
            printf("Error open %s for writing.\n", htmfile);
            exit(-1);
         }
         fprintf(html, "<table><tr>\n");
         fprintf(html, "<td class=\"sensordata\">Temperature:<span class=\"sensorvalue\">%3.2f</span></td>\n", bmed.temp_c);
         fprintf(html, "<td class=\"sensorspace\"></td>\n");
         fprintf(html, "<td class=\"sensordata\">Humidity:<span class=\"sensorvalue\">%3.2f</span></td>\n", bmed.humi_p);
         fprintf(html, "<td class=\"sensorspace\"></td>\n");
         fprintf(html, "<td class=\"sensordata\">Pressure:<span class=\"sensorvalue\">%3.2f</span></td>\n", bmed.pres_p);
         fprintf(html, "</tr></table>\n");
         fclose(html);
      }
      exit(0);
   } /* End reading sensor data */

   /* ----------------------------------------------------------- *
    *  "-c" continuously read and output compensated sensor data  *
    * 1584280335 Temp=22.76*C Humidity=22.30% Pressure=1002.56hPa *
    * ----------------------------------------------------------- */
   if(argflag == 5) {
      struct bmecal bmec;
      struct bmedata bmed;
      get_calib(&bmec);

      /* -------------------------------------------------------- *
       * If power mode != NORMAL, set NORMAL for continuous reads *
       * -------------------------------------------------------- */
      if(get_power() != 0x3) res = set_power(normal);

      while(1){
         time_t tsnow = time(NULL);
         get_data(&bmec, &bmed);
   
         /* ----------------------------------------------------------- *
          * print the formatted output string to stdout (Example below) *
          * ----------------------------------------------------------- */
         printf("%lld Temp=%3.2f*C Humidity=%3.2f%% Pressure=%3.2fhPa\n", 
                (long long) tsnow, bmed.temp_c, bmed.humi_p, bmed.pres_p/100);
   
         if(outflag == 1) {
            /* -------------------------------------------------------- *
             *  Open the html file for writing accelerometer data       *
             * -------------------------------------------------------- */
            FILE *html;
            if(! (html=fopen(htmfile, "w"))) {
               printf("Error open %s for writing.\n", htmfile);
               exit(-1);
            }
            fprintf(html, "<table><tr>\n");
            fprintf(html, "<td class=\"sensordata\">Temperature:<span class=\"sensorvalue\">%3.2f</span></td>\n", bmed.temp_c);
            fprintf(html, "<td class=\"sensorspace\"></td>\n");
            fprintf(html, "<td class=\"sensordata\">Humidity:<span class=\"sensorvalue\">%3.2f</span></td>\n", bmed.humi_p);
            fprintf(html, "<td class=\"sensorspace\"></td>\n");
            fprintf(html, "<td class=\"sensordata\">Pressure:<span class=\"sensorvalue\">%3.2f</span></td>\n", bmed.pres_p);
            fprintf(html, "</tr></table>\n");
            fclose(html);
         }
         sleep(1);
      }
   } /* End reading continuous data */
}
