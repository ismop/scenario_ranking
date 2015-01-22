#include <stdlib.h>
#include <stdio.h>
#include <math.h>

typedef struct rank rank;

struct rank {
  float value;
  int index;
};

int nScenarios = 1000,
    nSteps = 1400,
    nParams = 10,
    realLen = 100;

float ***db, **real;
rank *ranks;

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

int main(int argc, char **argv) {
  int i, j, k, l;
  srand((unsigned int)time(NULL));
  ranks = (rank *) malloc(nScenarios * sizeof(rank));
  db = malloc(nScenarios * sizeof(float **));
  for (i=0; i<nScenarios; i++) {
    ranks[i] = (rank) { -1.0, -1 };
    db[i] = malloc(nParams * sizeof(float *));
    for (j=0; j<nParams; j++) {
      db[i][j] = malloc(nSteps * sizeof(float));
      for (k=0; k<nSteps; k++) {
        db[i][j][k] = (float)rand()/(float)(RAND_MAX/100.0);
      }
    }
  }

  real = (float **) malloc(nParams * sizeof(float *));

  for (i=0; i<nParams; i++) {
    real[i] = (float *) malloc(realLen * sizeof(float));
    for (j=0; j<realLen; j++) {
      real[i][j] = (float)rand()/(float)(RAND_MAX/100.0);
    }
  }

  int nJobs = nSteps - realLen + 1;
  printf("nJobs=%d\n", nJobs);

  // Obliczenia!!!

  for (i=0; i<nScenarios; i++) {
    for (j=0; j<nParams; j++) {
      for (k=0; k<nJobs; k++) {
        float tmpRank = 0.0;
        //printf("scenario=%d, param=%d, index=%d\n", i, j, l);
        for (l=k; l<k+realLen; l++) {
          float diff = db[i][j][l] - real[j][l-k];
          tmpRank += abs(diff);
        }
        tmpRank = tmpRank / realLen;
        if (ranks[i].value < 0 || ranks[i].value > tmpRank) {
          ranks[i].value = tmpRank;
          ranks[i].index = l;
        }
      }
    }
  }

  qsort((void *)ranks, nScenarios, sizeof(rank), compare_ranks);

  for (i=0; i<nScenarios; i++) {
    printf("%d. rank=%.2f, index=%d\n", i, ranks[i].value, ranks[i].index);
  }
}
