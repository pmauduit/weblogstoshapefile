#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <gdal.h>
#include <GeoIP.h>

int main(int argc, char **argv) {

  if (argc < 1) {
    fprintf(stdout, "Usage: %s access.log [access.log.1 ...]\n", argv[0]);
    return 0;
  }

  int i;

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

      do {
        //fprintf(stdout, "token: \t%s\n", token);
      } while ((token = strtok_r(NULL, " ", &saveptr)) != NULL);
    }

    free(line);
    gettimeofday(&tval_after, NULL);
    timersub(&tval_after, &tval_before, &tval_result);

    fprintf(stdout, "%d lines parsed in %ld.%06ld seconds\n", numline, (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
    fclose(curfile);
  }

  return 0;
}

