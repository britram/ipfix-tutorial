//  Output temperature and relative humidity read from DHT11/22/AM2302
//  connected via Raspberry Pi GPIO.
//  Brian Trammell - July 2013

//  Adapted from Adafruit example code:
//  How to access GPIO registers from C-code on the Raspberry-Pi
//  Example program
//  15-January-2012
//  Dom and Gert
//


// Access from ARM Running Linux

#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <bcm2835.h>
#include <unistd.h>

#define MAXTIMINGS 100

//#define DEBUG

#define DHT11 11
#define DHT22 22
#define AM2302 22

int bits[250], data[100];
int bitidx = 0;

int readDHT(int type, int pin, float *degc, float *rh) {
  int counter = 0;
  int laststate = HIGH;
  int j=0;

  // Set GPIO pin to output
  bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);

  bcm2835_gpio_write(pin, HIGH);
  usleep(500000);  // 500 ms
  bcm2835_gpio_write(pin, LOW);
  usleep(20000);

  bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);

  data[0] = data[1] = data[2] = data[3] = data[4] = 0;

  // wait for pin to drop
  while (bcm2835_gpio_lev(pin) == 1) {
    usleep(1);
  }

  // read data!
  for (int i=0; i< MAXTIMINGS; i++) {
    counter = 0;
    while ( bcm2835_gpio_lev(pin) == laststate) {
    counter++;
    //nanosleep(1);     // overclocking might change this?
        if (counter == 1000)
      break;
    }
    laststate = bcm2835_gpio_lev(pin);
    if (counter == 1000) break;
    bits[bitidx++] = counter;

    if ((i>3) && (i%2 == 0)) {
      // shove each bit into the storage bytes
      data[j/8] <<= 1;
      if (counter > 200)
        data[j/8] |= 1;
      j++;
    }
  }


#ifdef DEBUG
  for (int i=3; i<bitidx; i+=2) {
    fprintf(stderr, "bit %d: %d\n", i-3, bits[i]);
    fprintf(stderr, "bit %d: %d (%d)\n", i-2, bits[i+1], bits[i+1] > 200);
  }
  fprintf(stderr, "Data (%d): 0x%x 0x%x 0x%x 0x%x 0x%x\n", j, data[0], data[1], data[2], data[3], data[4]);
#endif

  if ((j >= 39) &&
      (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) ) 
  {
      // yay!
      if (type == DHT11) {
          *degc = data[2];
          *rh = data[1];
          return 1;
      } else if (type == DHT22) {
          float f, h;
          h = data[0] * 256 + data[1];
          h /= 10.0;

          f = (data[2] & 0x7F)* 256 + data[3];
          f /= 10.0;
          if (data[2] & 0x80)  f *= -1;
          
          *degc = f;
          *rh = h;
          
          return 1;
      } else {
          // logic error: bad type
          return 0;
      }
  } else {
      // checksum failure?
      return 0;      
  }

}


int main(int argc, char **argv)
{
  int type, dhtpin;
  float degc, rh;
    
  if (!bcm2835_init())
        return 1;

  if (argc != 3) {
    fprintf(stderr, "usage: %s [11|22|2302] GPIOpin#\n", argv[0]);
    fprintf(stderr, "example: %s 2302 4 - Read from an AM2302 connected to GPIO #4\n", argv[0]);
    return 2;
  }
  
  type = 0;
  if (strcmp(argv[1], "11") == 0) type = DHT11;
  if (strcmp(argv[1], "22") == 0) type = DHT22;
  if (strcmp(argv[1], "2302") == 0) type = AM2302;
  if (type == 0) {
    fprintf(stderr, "Select 11, 22, 2302 as type!\n");
    return 3;
  }
  
  dhtpin = atoi(argv[2]);
  if (dhtpin <= 0) {
    fprintf(stderr, "Please select a valid GPIO pin #\n");
    return 3;
  }

  while (1) {
      if (readDHT(type, dhtpin, &degc, &rh)) {
          fprintf(stdout, "%.1f %.1f\n", degc, rh);
          fflush(stdout);
      }
      sleep(5);
  }

} // main
