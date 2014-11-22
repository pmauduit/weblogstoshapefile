#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <glib.h>

#include <gdal.h>
#include <GeoIP.h>
#include <GeoIPCity.h>

/* inspired from the example
 * see https://github.com/maxmind/geoip-api-c/blob/master/test/test-geoip-city.c */

// Debian provided file is not the City one, see README.md
// #define GEOIP_PATH "/usr/share/GeoIP/GeoIP.dat"
#define GEOIP_PATH "GeoLiteCity.dat"

struct ght_data_struct {
  char * ip; // also in the key
  char * city; // provided by MaxMind db
  unsigned int frequency; // number of hits
  float lat,lon;
};

void ght_print_elem(gpointer k, gpointer v, gpointer user_data) {
  struct ght_data_struct * d = (struct ght_data_struct *) v;
  fprintf(stdout, "Element\n\tip: %s\n\tcity: %s\n\tfreq: %d\n\tcoords: (%.4f:%.4f)\n\n",
      d->ip, d->city, d->frequency, d->lat, d->lon);
}

void ght_destroy_key(gpointer data) {
  // key is a simple char * allocated using strdup()
  //free(data);
}

// value can contain more attributes in the future
void ght_destroy_value(gpointer data) {
  struct ght_data_struct * d = (struct ght_data_struct *) data;
  free(d->ip);
  free(d->city);
  free(data);
}

int main(int argc, char **argv) {

  if (argc <= 1) {
    fprintf(stdout, "Usage: %s access.log [access.log.1 ...]\n", argv[0]);
    return 0;
  }

  int i;
  GeoIP *gi;

  GHashTable* ip_hash = NULL;

  ip_hash = g_hash_table_new_full(g_str_hash, g_str_equal,
      ght_destroy_key, ght_destroy_value);

  gi = GeoIP_open(GEOIP_PATH, GEOIP_INDEX_CACHE);
  GeoIP_set_charset(gi, GEOIP_CHARSET_UTF8);


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
            struct ght_data_struct * data = g_hash_table_lookup(ip_hash,
                token);
            if (data == NULL) {
              struct ght_data_struct * nd = malloc(sizeof(struct ght_data_struct));
              nd->ip = strdup(token);
              nd->city = strdup(gir->city != NULL ? gir->city : "N/A");
              nd->lat = gir->latitude;
              nd->lon = gir->longitude;
              nd->frequency = 1;
              g_hash_table_insert(ip_hash, token, nd);
            }
            else data->frequency++;

            GeoIPRecord_delete(gir);
          }
        }
        current_token ++;
      } while ((token = strtok_r(NULL, " ", &saveptr)) != NULL);
    }

    free(line);
    gettimeofday(&tval_after, NULL);
    timersub(&tval_after, &tval_before, &tval_result);

    fprintf(stdout, "%d lines parsed in %ld.%06ld seconds\n",
        numline, (long int)tval_result.tv_sec,
        (long int)tval_result.tv_usec);
    fclose(curfile);


  }

  // Releasing GeoIP resources
  GeoIP_delete(gi);

  g_hash_table_foreach(ip_hash, ght_print_elem, NULL);
  g_hash_table_destroy(ip_hash);
  return 0;
}

