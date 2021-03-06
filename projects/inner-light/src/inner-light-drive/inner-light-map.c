/*
 * 
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 */

//
// test program to touch the innerlight.led memory mapped file for debugging
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/mman.h>

#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#include <vector>
#include <string>


#define INNER_LIGHT_DRIVER_DEFAULT_MAP_FILE "/tmp/innerlight.led"
#define _DEFAULT_NUM_LED 180
#define _VERSION "0.1.0"


struct option _longopt[] = {
  {"help", no_argument, 0, 'h'},

  {0,0,0,0}
};

void show_help_and_exit(FILE *fp) {
  fprintf(fp, "usage:  inner-light-map [-i led] [-n nled] [-s start] [-d del] [-h] [-C hexrgb] [-v]\n\n");
  if (fp==stdout) { exit(0); }
  exit(1);
}

void show_version_and_exit(FILE *fp) {
  fprintf(fp, "%s\n", _VERSION);
  if (fp==stdout) { exit(0); }
  exit(1);
}



int main(int argc, char **argv) {
  int i;
  int led_map_fd;
  std::string led_fn;
  std::string hexrgb;
  size_t n_led=0, led_map_len;

  int clear = 1;

  int start=-1, del=-1;
  int ch, opt_idx;
  unsigned char *led_map;

  char _r = 0xff, _g = 0xff, _b = 0xff;

  while ((ch = getopt_long(argc, argv, "hvi:n:s:d:C:", _longopt, &opt_idx)) >= 0) {
    switch (ch) {
      case 0:
        break;
      case 'h':
        show_help_and_exit(stdout);
        break;
      case 'v':
        show_version_and_exit(stdout);
        break;
      case 'n':
        n_led = atoi(optarg);
        break;
      case 's':
        start = atoi(optarg);
        break;
      case 'd':
        del = atoi(optarg);
        break;
      case 'C':
        hexrgb = optarg;
        break;
      case 'i':
        led_fn = optarg;
        break;
      default:
        show_help_and_exit(stderr);
        break;
    }
  }

  if (led_fn.size() == 0) {
    if (optind < argc) {
      led_fn = argv[optind];

      printf("using: %s\n", led_fn.c_str());
    }
    else {
      led_fn = INNER_LIGHT_DRIVER_DEFAULT_MAP_FILE;
    }
  }

  if (n_led==0) {
    n_led = _DEFAULT_NUM_LED;
  }

  if (hexrgb.size() == 6) {
    for (i=0; i<hexrgb.size(); i++) {
      if ( ((hexrgb[i] >= '0') && (hexrgb[i] <= '9')) ||
           ((hexrgb[i] >= 'a') && (hexrgb[i] <= 'f')) ||
           ((hexrgb[i] >= 'A') && (hexrgb[i] <= 'F')) ) {
        //pass
      }
      else { break; }
    }

    if (i==hexrgb.size()) {
      for (i=0; i<hexrgb.size(); i++) {
        if ((hexrgb[i] >= 'A') && (hexrgb[i] <= 'F')) {
          hexrgb[i] -= 'A';
          hexrgb[i] += 'a';
        }
        if ((hexrgb[i] >= '0') && (hexrgb[i] <= '9')) {
          hexrgb[i] -= '0';
        }
        else {
          hexrgb[i] -= 'a';
          hexrgb[i] += 10;
        }
      }
      _r = 0;
      _r = ((int)hexrgb[0])*16 + (int)hexrgb[1];
      _g = 0;
      _g = ((int)hexrgb[2])*16 + (int)hexrgb[3];
      _b = 0;
      _b = ((int)hexrgb[4])*16 + (int)hexrgb[5];
    }
  }

  if (start<0) { start = 0; }
  if (del<0) { del = n_led; }
  if (start>=n_led) { start = n_led-1; }
  if ((start+del)>=n_led) { del = n_led - start; }

  led_map_len = n_led*3+1;

  led_map_fd = open(led_fn.c_str(), O_RDWR);
  if (led_map_fd < 0) {
    fprintf(stderr, "error opening %s, errno:%i\n", led_fn.c_str(), errno);
    exit(-1);
  }

  led_map = (unsigned char *)mmap(NULL, led_map_len, PROT_NONE | PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, led_map_fd, 0);

  if (led_map == MAP_FAILED) { fprintf(stderr, "failed\n"); exit(-1); }
  if (led_map == NULL) {
    fprintf(stderr, "ERROR: could not map file (fd:%i), got errno:%i, exiting\n",
      led_map_fd, errno);
    exit(-1);
  }

  if (clear) {
    for (i=0; i<n_led; i++) {
      led_map[3*i+1] = 0;
      led_map[3*i+2] = 0;
      led_map[3*i+3] = 0;
    }
  }

  led_map[0] = 1;
  for (i=start; i<(start+del); i++) {
    /*
    led_map[3*i+1] = (unsigned char)(i%255);
    led_map[3*i+2] = (unsigned char)(i%255);
    led_map[3*i+3] = (unsigned char)(i%255);
    */
    led_map[3*i+1] = _r;
    led_map[3*i+2] = _g;
    led_map[3*i+3] = _b;
  }


  printf(">>> %s, %i\n", led_fn.c_str(), (int)n_led);


  munmap(led_map, led_map_len);
  close(led_map_fd);
}
