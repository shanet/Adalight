/*

Colorswirl

Author: Shane Tully

Source:      https://github.com/shanet/Adalight
Forked from: https://github.com/adafruit/Adalight

This is the host PC-side code written in C; intended for use with a 
USB-connected Arduino microcontroller running the accompanying LED 
streaming code.  Requires one strand of  Digital RGB LED Pixels 
(Adafruit product ID #322, specifically the newer WS2801-based 
type, strand of 25) and a 5 Volt power supply (such as 
Adafruit #276).  You may need to adapt the code and  the hardware 
arrangement for your specific configuration.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <pthread.h>
#include <signal.h>
#include <mqueue.h>
#include <fcntl.h>
#include <termios.h>

#include <time.h>
#include <math.h>
#include <getopt.h>


#define NUM_LEDS       25
#define DEFAULT_DEVICE "/dev/ttyACM0"

#define NORMAL_EXIT   0
#define ABNORMAL_EXIT 1

// Verbosity options
#define NO_VERBOSE  0
#define VERBOSE     1
#define DBL_VERBOSE 2
#define TPL_VERBOSE 3

// Color options
#define MULTI  0
#define RED    1
#define ORANGE 2
#define YELLOW 3
#define GREEN  4
#define BLUE   5
#define PURPLE 6
#define WHITE  7

// Rotation speed options
#define ROT_NONE      0
#define ROT_VERY_SLOW 1
#define ROT_SLOW      2
#define ROT_NORMAL    3
#define ROT_FAST      4
#define ROT_VERY_FAST 5

// Rotation direction options
#define ROT_CW  0
#define ROT_CCW 1

// Shadow length options
#define SDW_NONE       0
#define SDW_VERY_SMALL 1
#define SDW_SMALL      2
#define SDW_NORMAL     3
#define SDW_LONG       4
#define SDW_VERY_LONG  5


char *prog;                // Name of the program
int verbose;               // Verbosity level
static int color;          // Selected color
static int rotationSpeed;  // Selected rotation speed
static int rotationDir;    // Selected rotation direction
static int shadowLength;   // Selected shadow length
static time_t curTime;     // The current time
static time_t startTime;   // Time of program start
static time_t prevTime;    // Previous current time


int processArgs(int argc, char **argv, char **device);
void* messageLoop(void*);
int openTTY(char *device);
void updateColor(unsigned char *r, unsigned char *g, unsigned char *b, int curHue);
void updateLightPosition(double *lightPosition);
void updateShadowPosition(double *shadowPosition);
void updateHue(int *curHue);
void sendBuffer(unsigned char *buffer, size_t bufLen, int fd);
void sigHandler(int sig);
int installSigHandler(int sig, sighandler_t func);
void printUsage(void);
void printVersion(void);