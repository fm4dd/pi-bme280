CC=gcc
CFLAGS= -O3 -Wall -g
LIBS= -lm
AR=ar

ALLBIN=getbme280

all: ${ALLBIN}

clean:
	rm -f *.o ${ALLBIN}

getbme280: i2c_bme280.o getbme280.o
	$(CC) i2c_bme280.o getbme280.o -o getbme280 ${LIBS}

