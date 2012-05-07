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


#define N_LEDS 25 // Max of 65536

#define NORMAL_EXIT   0
#define ABNORMAL_EXIT 1

#define NO_VERBOSE  0
#define VERBOSE     1
#define DBL_VERBOSE 2
#define TPL_VERBOSE 3

#define MULTI  0
#define RED    1
#define ORANGE 2
#define YELLOW 3
#define GREEN  4
#define BLUE   5
#define PURPLE 6
#define WHITE  7

#define ROT_NONE      0
#define ROT_VERY_SLOW 1
#define ROT_SLOW      2
#define ROT_NORMAL    3
#define ROT_FAST      4
#define ROT_VERY_FAST 5


char *prog;              // Name of the program
int verbose;             // Verbosity level
static int curHue;
static int color;
static int rotationSpeed;
static double curRotationSpeed;
static time_t curTime;
static time_t startTime;
static time_t prevTime;


int openTTY(char *device);
void getColor(unsigned char *r, unsigned char *g, unsigned char *b);
void updateRotationSpeed(void);
void sendBuffer(unsigned char *buffer, size_t bufLen, int fd);
void printUsage(void);
void printVersion(void);