//
//  exports output of dht_temprh.c via ipfix to stdout
//

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
#include <unistd.h>
#include <arpa/inet.h>

#define LINEBUF_SZ 80

uint32_t seconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

void export_template_message()
{
    static uint8_t template_message[] =
        {0x0,0xa,0x0,0x30,0x0,0x0,0x0,0x0,
         0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x1,
         0x0,0x2,0x0,0x20,0x1,0x0,0x0,0x4,
         0x1,0x42,0x0,0x4,0x0,0x8a,0x0,0x4,
         0x80,0x2,0x0,0x4,0x0,0x0,0x8a,0xee,
         0x80,0x3,0x0,0x4,0x0,0x0,0x8a,0xee};
    
    static const size_t export_time_offset = 8;
    
    // rewrite export time in header message buffer
    uint32_t *export_time = (uint32_t *)&template_message[export_time_offset];
    *export_time = htonl(seconds());
    
    // dump buffer
    fwrite(template_message, sizeof(template_message), 1, stdout);
    fflush(stdout);
}

void export_uint32(uint32_t val)
{
    uint32_t nval = htonl(val);
    fwrite(&nval, sizeof(nval), 1, stdout);
}

void export_float(float val)
{
    uint32_t nval;
    memcpy(&nval, &val, 4);
    nval = htonl(nval);
    fwrite(&nval, sizeof(nval), 1, stdout);
}

void export_weather_message(uint32_t opid, float degc, float rh)
{
    static unsigned int weather_message_counter = 0;
    
    static uint8_t weather_message_header[] = 
        {0x0,0xa,0x0,0x24,0x0,0x0,0x0,0x0,
         0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x1,
         0x1,0x0,0x0,0x14};

    static const size_t sequence_offset = 4;
    static const size_t export_time_offset = 8;

    // rewrite sequence number in weather message buffer
    uint32_t *sequence = (uint32_t *)&weather_message_header[sequence_offset];
    *sequence = htonl(weather_message_counter++);
    
    // rewrite export time in weather message buffer
    uint32_t *export_time = (uint32_t *)&weather_message_header[export_time_offset];
    *export_time = htonl(seconds());
   
    // dump header buffer
    fwrite(weather_message_header, sizeof(weather_message_header), 1, stdout);
    
    // export observation time
    export_uint32(seconds());
    
    // export observation point ID
    export_uint32(opid);
    
    // export temperature and humidity
    export_float(degc);
    export_float(rh);
    
    fflush(stdout);
}

int main(int argc, char **argv)
{
  static char linebuf[LINEBUF_SZ];
  int opid;
  float degc, rh;

  if (argc != 2) {
      fprintf(stderr, "usage: %s OPid < instream > ipfixstream\n", argv[0]);
  }
  
  opid = atoi(argv[1]);

  // and export forever
  fprintf(stderr, "Exporting template message...\n");
  export_template_message();
  
  while (1) {
      if (fgets(linebuf, LINEBUF_SZ, stdin) &&
          (sscanf(linebuf, "%f %f", &degc, &rh) == 2)) 
      {
          fprintf(stderr, "Exporting weather message (%.1fC %.1f%%)\n", degc, rh);
          export_weather_message(opid, degc, rh);          
      } else {
          break;
      }
  }
} // main
