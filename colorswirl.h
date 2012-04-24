#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <math.h>
#include <getopt.h>

#define N_LEDS 25 // Max of 65536

#define NORMAL_EXIT 0
#define ABNORMAL_EXIT 1

#define NO_VERBOSE 0
#define VERBOSE 1
#define DBL_VERBOSE 2
#define TPL_VERBOSE 3

char *prog;              // Name of the program
int verbose;             // The verbose level

void printUsage(void);
void printVersion(void);