#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <fcntl.h>
#include <termios.h>

#include <time.h>
#include <math.h>
#include <getopt.h>

// Version number is defined in the Makefile
#define VERSION_STRING TO_STRING(VERSION)
#define TO_STRING(s) TO_STRING_2(s)
#define TO_STRING_2(s) #s

#define N_LEDS 25 // Max of 65536

#define NORMAL_EXIT 0
#define ABNORMAL_EXIT 1

#define NO_VERBOSE 0
#define VERBOSE 1
#define DBL_VERBOSE 2
#define TPL_VERBOSE 3


char *prog;              // Name of the program
int verbose;             // Verbosity level
static time_t curTime;
static time_t startTime;
static time_t prevTime;


int openTTY(char *device);
void sendBuffer(unsigned char *buffer, size_t bufLen, int fd);
void printUsage(void);
void printVersion(void);