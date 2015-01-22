#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <assert.h>


/* 
** Scenario identification for flood decision support
**
** 1. Reads all files from 'data_dir'
**    - Files 'scenario*.csv' contain scenarios data
**    - File  'sample.csv' contains 'real' time series to compare with
** 2. Computes the scenario ranking
** 3. Writes output to file 'data_dir/output.txt'
**
** Limitations:
** - Assumes the same number of lines in each 'scenario*.csv'
** - All files also must have the same number of columns
** - Max line length (2000) and column count (30) are also assumed
**
*/


const char *data_dir = "./data";
const char *work_dir = "./work";

int nScenarios = 0,
    nSteps = 0,
    nParams = 0,
    realLen = 0;

// Read files START

char buf[2000];	 /* input line buffer */
float field[30]; /* fields (different params being compared, max 30) */

char *unquote(char *);

/* read and parse line, return field count */
int csvgetline(FILE *fin)
{	
  int nfield;
  char *p, *q;
  if (fgets(buf, sizeof(buf), fin) == NULL)
    return -1;
  nfield = 0;
  for (q = buf; (p=strtok(q, ",\n\r")) != NULL; q = NULL)
    field[nfield++] = atof(unquote(p));
  return nfield;
}

/* remove leading and trailing quote */
char *unquote(char *p)
{
  if (p[0] == '"') {
    if (p[strlen(p)-1] == '"')
      p[strlen(p)-1] = '\0';
    p++;
  }
  return p;
}

// read scenarios to array 'db'
// return the number of scenarios
// FIXME: sets global variables nSteps and nParams
int read_scenarios(float ***db) {
  int nScenarios = 0;
  char *scenario_filenames[2000]; // max 2000 scenarios
  DIR *dir;
  struct dirent *ent;

  if ((dir = opendir (data_dir)) != NULL) {

    // create a list of scenario file names
    while ((ent = readdir (dir)) != NULL) {
      if (ent->d_type == DT_REG) {
        if (!strncmp(ent->d_name, "scenario", 8)) {
          scenario_filenames[nScenarios++] = strdup(ent->d_name);
        }
        //printf ("%s/%s\n", data_dir, ent->d_name);
      }
    }
    printf ("nScenarios=%d\n", nScenarios);
    closedir (dir);
  } else {
    /* could not open directory */
    perror ("");
    return EXIT_FAILURE;
  }

  int scenario_id, param_id, nf;
  for (scenario_id=0; scenario_id<nScenarios; ++scenario_id) {
    //int nParams = 0, nSteps = 0;
    char file[100];
    sprintf(file, "%s/%s", data_dir, scenario_filenames[scenario_id]);
    FILE *f = fopen(file, "r");

    // first count lines to determine the scenario length!
    // wasteful because of the file format...
    // Only do it ONCE (number of rows is assumed to be the same for all files)
    if (nSteps == 0) {
      int ch, lines = 0;
      while(!feof(f)) {
        ch = fgetc(f);
        if(ch == '\n') {
          lines++;
        }
      }
      nSteps = lines-1; // don't count the first line 
    }

    // now read data from the file
    rewind(f);
    nParams = csvgetline(f); // the first line with column numbers
    db[scenario_id] = (float **) malloc(nParams * sizeof(float **));
    int i;
    for (i=0; i<nParams; i++) {
      db[scenario_id][i] = (float *) malloc(nSteps * sizeof(float));
    }

    int row_number = -1;
    while ((nf = csvgetline(f)) != -1) {
      ++row_number;
      assert (nf == nParams); // every row has the same number of columns
      for (param_id = 0; param_id < nf; param_id++) {
        db[scenario_id][param_id][row_number] = field[param_id];
        //printf("field[%d] = %.2f\n", param_id, field[param_id]);
      }
    }

    fclose(f);
  }

  return nScenarios;

}

// read 'real' time series to 'real'
int read_sample(float **real) {
  char file[100];

  sprintf(file, "%s/%s", work_dir, "sample.csv");
  FILE *f = fopen(file, "r");

  // first count lines to determine the scenario length!
  // wasteful because of the file format...
  int ch, lines = 0;
  while(!feof(f)) {
    ch = fgetc(f);
    if(ch == '\n') {
      lines++;
    }
  }
  realLen = lines-1; // don't count the first line 

  // now read data from the file
  rewind(f);
  int i, nf, param_id;
  nParams = csvgetline(f); // the first line with column numbers
  for (i=0; i<nParams; i++) {
    real[i] = (float *) malloc(realLen * sizeof(float));
  }

  int row_number = -1;
  while ((nf = csvgetline(f)) != -1) {
    ++row_number;
    assert (nf == nParams); // every row has the same number of columns
    for (param_id = 0; param_id < nf; param_id++) {
      real[param_id][row_number] = field[param_id];
      //printf("field[%d] = %.2f\n", param_id, field[param_id]);
    }
  }
  /*
     int j;
     for (i=0; i<nParams; ++i) 
     for (j=0; j<realLen; ++j)
     printf("sample[%d][%d]=%.2f\n", i, j, real[i][j]);
   */

  fclose(f);
  return realLen;
}

// Read files END

typedef struct rank rank;

struct rank {
  float value;
  int index;
};


int compare_ranks(const void * a, const void * b) {
  const rank * pa = a;
  const rank * pb = b;
  if ( (*pa).value < (*pb).value ) {
    return -1;
  } else if ( (*pa).value > (*pb).value ) {
    return 1;
  } else {
    return 0;
  }
}

float **scenario_db[2000]; // max 2000 scenarios
float *real[30]; // 'real' time series to compare with (max 30 params)
rank *ranks;

int main(int argc, char **argv) {
  int i, j, k, l, nf;

  nScenarios = read_scenarios(scenario_db);
  realLen = read_sample(real);
  ranks = (rank *) malloc(nScenarios * sizeof(rank));
  for (i=0; i<nScenarios; i++) {
    ranks[i] = (rank) { -1.0, -1 };
  }

  int nJobs = nSteps - realLen + 1;
  printf("nJobs=%d\n", nJobs);
  printf("nScenarios=%d\n", nScenarios);
  printf("nParams=%d\n", nParams);
  printf("nSteps=%d\n", nSteps);
  printf("realLen=%d\n", realLen);

  // Obliczenia!!!

  for (i=0; i<nScenarios; i++) {
    for (j=0; j<nParams; j++) {
      for (k=0; k<nJobs; k++) {
        float tmpRank = 0.0;
        for (l=k; l<k+realLen; l++) {
          float diff = scenario_db[i][j][l] - real[j][l-k];
          //printf("(%d,%d,%d)=%.2f\n", i, j, l, scenario_db[i][j][l]);
          //printf("real(%d,%d)=%.2f\n", j, l-k, real[j][l-k]);
          tmpRank += abs(diff);
        }
        tmpRank = tmpRank / realLen;
        if (ranks[i].value < 0 || ranks[i].value < tmpRank) {
          ranks[i].value = tmpRank;
          ranks[i].index = l;
        }
      }
    }
  }

  qsort((void *)ranks, nScenarios, sizeof(rank), compare_ranks);

  // write output file
  char file[100];
  sprintf(file, "%s/%s", work_dir, "output.txt");
  FILE *f = fopen(file, "w");

  for (i=0; i<nScenarios; i++) {
    fprintf(f, "%d %.2f %d\n", i, ranks[i].value, ranks[i].index);
  }
  fclose(f);
}
