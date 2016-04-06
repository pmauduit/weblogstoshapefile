#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <glib.h>

#include <ogr_api.h>
#include <ogr_srs_api.h>

#include <GeoIP.h>
#include <GeoIPCity.h>

/* inspired from the example
 * see https://github.com/maxmind/geoip-api-c/blob/master/test/test-geoip-city.c */

// Debian provided file is not the City one, see README.md
// #define GEOIP_PATH "/usr/share/GeoIP/GeoIP.dat"
#define GEOIP_PATH "GeoLiteCity.dat"

// OGR driver name (by default ESRI shapefile)
#define OGR_DRIVER "ESRI Shapefile"

struct ght_data_struct {
  char * ip; // also in the key
  char * city; // provided by MaxMind db
  char * user_agent;
  unsigned int frequency; // number of hits
  float lat,lon;
};

// Used for debugging purposes
// Just prints out the ght_data_struct
void ght_print_elem(gpointer k, gpointer v, gpointer user_data) {
  struct ght_data_struct * d = (struct ght_data_struct *) v;
  fprintf(stdout, "Element\n\tip: %s\n\tcity: %s\n\tuser_agent: %s\n\tfreq: %d\n\tcoords: (%.4f:%.4f)\n\n",
      d->ip, d->city, d->user_agent, d->frequency, d->lat, d->lon);
}

// Builds the features given the elements of the list
void ght_generate_feature_set(gpointer k, gpointer v, gpointer user_data) {

  // user_data is our layer
  OGRLayerH l = (OGRLayerH) user_data;
  // v is our value
  struct ght_data_struct * value = (struct ght_data_struct *) v;

  OGRFeatureH feat;
  OGRGeometryH geompoint;

  feat = OGR_F_Create(OGR_L_GetLayerDefn(l));
  OGR_F_SetFieldString(feat, OGR_F_GetFieldIndex(feat, "ip"), value->ip);
  OGR_F_SetFieldString(feat, OGR_F_GetFieldIndex(feat, "city"), value->city);
  OGR_F_SetFieldString(feat, OGR_F_GetFieldIndex(feat, "user_agent"), value->user_agent);
  OGR_F_SetFieldInteger(feat, OGR_F_GetFieldIndex(feat, "frequency"), value->frequency);

  geompoint = OGR_G_CreateGeometry(wkbPoint);
  OGR_G_SetPoint_2D(geompoint, 0, value->lon, value->lat);

  OGR_F_SetGeometry(feat, geompoint);
  OGR_G_DestroyGeometry(geompoint);

  if( OGR_L_CreateFeature(l, feat) != OGRERR_NONE)
  {
    fprintf(stderr,"Failed to create feature in shapefile.\n");
    abort();
  }
  OGR_F_Destroy(feat);
}

void ght_destroy_key(gpointer data) {
  // key is a simple char * allocated using strdup()
  //free(data);
}

// value can contain more attributes in the future
void ght_destroy_value(gpointer data) {
  struct ght_data_struct * d = (struct ght_data_struct *) data;
  free(d->ip);
  free(d->user_agent);
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

  // init GDAL/OGR
  OGRRegisterAll();
  //for (i = 0; i < OGRGetDriverCount(); ++i) {
  // OGRSFDriverH d = OGRGetDriver(i);
  // fprintf(stdout, "driver name %s\n", OGR_Dr_GetName(d));
  //}


  if (gi == NULL) {
    fprintf(stderr, "Unable to open GeoIP database\n");
    return 1;
  }

  for (i = 1; i < argc ; i++) {
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    FILE  * curfile = NULL;

    fprintf(stdout, "Parsing %s ...\n", argv[i]);

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
        //fprintf(stdout, "current token %d: %s\n", current_token, token);
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
        // TODO: the UA seems to begin at token 11
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

  //g_hash_table_foreach(ip_hash, ght_print_elem, NULL);

  // Clearly not portable, but bleh ...
  system("rm -rf ./out/");

  OGRSFDriverH driver = OGRGetDriverByName(OGR_DRIVER);
  if(driver == NULL)
  {
    fprintf(stderr, "%s driver not available.\n", OGR_DRIVER);
    abort();
  }

  OGRDataSourceH datasource = OGR_Dr_CreateDataSource(driver, "out", NULL);
  if(datasource == NULL)
  {
    fprintf(stderr, "Creation of output file failed.\n");
    abort();
  }

  OGRLayerH layer = OGR_DS_CreateLayer(datasource, "out", NULL, wkbPoint, NULL );
  if(layer == NULL)
  {
    fprintf(stderr, "Layer creation failed.\n");
    abort();
  }

  OGRFieldDefnH ipfield = OGR_Fld_Create("ip", OFTString);
  OGRFieldDefnH cityfield = OGR_Fld_Create("city", OFTString);
  OGRFieldDefnH freqfield = OGR_Fld_Create("frequency", OFTInteger);

  if ((OGR_L_CreateField(layer, ipfield,   TRUE) != OGRERR_NONE) ||
      (OGR_L_CreateField(layer, cityfield, TRUE) != OGRERR_NONE) ||
      (OGR_L_CreateField(layer, freqfield, TRUE) != OGRERR_NONE))
  {
    fprintf(stderr,"Creating fields failed.\n");
    abort();
  }

  OGR_Fld_Destroy(ipfield);
  OGR_Fld_Destroy(cityfield);
  OGR_Fld_Destroy(freqfield);

  g_hash_table_foreach(ip_hash, ght_generate_feature_set, layer);


  OGR_DS_SyncToDisk(datasource);
  OGRReleaseDataSource(datasource);

  // generates the prj
  char * prjContent = NULL;
  FILE * prjFile = NULL;
  OGRSpatialReferenceH srs = OSRNewSpatialReference(NULL);
  OSRImportFromEPSG(srs, 4326);
  OSRMorphToESRI(srs);
  OSRExportToWkt(srs, &prjContent);
  prjFile = fopen("out/out.prj", "w");
  fwrite(prjContent, strlen(prjContent), 1, prjFile);
  fclose(prjFile);
  free(prjContent);


  // Releasing elements from the hashmap
  g_hash_table_destroy(ip_hash);

  // Releasing GeoIP resources
  GeoIP_delete(gi);
  // de-register ogr
  for (i = 0; i < OGRGetDriverCount(); ++i) {
    OGRSFDriverH d = OGRGetDriver(i);
    OGRDeregisterDriver(d);
  }

  return 0;
}

