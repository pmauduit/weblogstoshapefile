#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <gdal.h>
#include <GeoIP.h>
#include <GeoIPCity.h>

/* inspired from the example
 * see https://github.com/maxmind/geoip-api-c/blob/master/test/test-geoip-city.c */

// Debian provided file is not the City one, see README.md
// #define GEOIP_PATH "/usr/share/GeoIP/GeoIP.dat"
#define GEOIP_PATH "GeoLiteCity.dat"

int main(int argc, char **argv) {

  if (argc < 1) {
    fprintf(stdout, "Usage: %s access.log [access.log.1 ...]\n", argv[0]);
    return 0;
  }

  int i;
  GeoIP *gi;

  gi = GeoIP_open(GEOIP_PATH, GEOIP_INDEX_CACHE);

  if (gi == NULL) {
    fprintf(stderr, "Unable to open GeoIP database\n");
    return 1;
  }

  for (i = 1; i < argc ; i++) {
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    FILE  * curfile = NULL;

    fprintf(stdout, "Parsing %s ...\n", argv[1]);

    curfile = fopen(argv[i], "r");

    if (curfile == NULL) {
      fprintf(stderr, "Unable to open %s. skipping.\n", argv[i]);
      continue;
    }

    int numline = 0;
    struct timeval tval_before, tval_after, tval_result;
    gettimeofday(&tval_before, NULL);

    while ((read = getline(&line, &len, curfile)) != -1) {
      numline++;
      char * token = NULL;
      char *saveptr = NULL;
      token = strtok_r(line, " ", &saveptr);
      if (token == NULL)
        continue;
      int current_token = 0;
      do {
        //fprintf(stdout, "token: \t%s\n", token);

        // IP address geographic lookup
        if (current_token == 0) {
          GeoIPRecord *gir = NULL;
          gir = GeoIP_record_by_addr(gi, (const char *) token);
          if (gir != NULL) {
            fprintf(stdout, "\t%s located in %f:%f\n", token, gir->latitude, gir->longitude);
            GeoIPRecord_delete(gir);
          }
        }
        current_token ++;
      } while ((token = strtok_r(NULL, " ", &saveptr)) != NULL);
    }

    free(line);
    gettimeofday(&tval_after, NULL);
    timersub(&tval_after, &tval_before, &tval_result);

    fprintf(stdout, "%d lines parsed in %ld.%06ld seconds\n", numline, (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
    fclose(curfile);
  }

  // Releasing GeoIP resources
  GeoIP_delete(gi);
  return 0;
}

