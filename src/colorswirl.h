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
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <pthread.h>
#include <signal.h>
#include <mqueue.h>
#include <fcntl.h>
#include <termios.h>

#include <time.h>
#include <math.h>
#include <getopt.h>


#define NUM_LEDS 25

#define NORMAL_EXIT   0
#define ABNORMAL_EXIT 1

#define TRUE  1
#define FALSE 0

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
#define COOL   8

// Rotation speed options
#define ROT_NONE      0
#define ROT_VERY_SLOW 1
#define ROT_SLOW      2
#define ROT_NORMAL    3
#define ROT_FAST      4
#define ROT_VERY_FAST 5

// Fade speed options
#define FADE_NONE      0
#define FADE_VERY_SLOW 1
#define FADE_SLOW      2
#define FADE_NORMAL    3
#define FADE_FAST      4
#define FADE_VERY_FAST 5

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

// Sample options
#define MIN_BRIGHTNESS 120
#define FADE           75
#define BLEND_WEIGHT   (257 - FADE)


typedef struct {
    int x;
    int y;
} Point;


char *prog;                    // Name of the program
int verbose;                   // Verbosity level
int screenWidth;               // Width of the screen
int screenHeight;              // Height of the screen
Point *samplePoints[NUM_LEDS]; // Pixel locations of the edges of each sample box
Display *XDisplay;             // Connection to X11
char gammaCorrection[256][3];  // Gamma correction table for sampled RGB values

int noFork;           // Flag for not forking on startup
int isScreenSampling; // Flag for sampling screen colors for LED color data
int color;            // Selected color
int rotationSpeed;    // Selected rotation speed
int rotationDir;      // Selected rotation direction
int shadowLength;     // Selected shadow length
int fadeSpeed;        // If the solid flag was selected

time_t curTime;   // The current time
time_t startTime; // Time of program start
time_t prevTime;  // Previous current time


int processArgs(int argc, char **argv, char **device);
void* messageLoop(void*);
void startMessageThread(pthread_t *threadID);

int openDevice(char *device);
void getLedDataHeader(unsigned char *ledData);
void sendLedDataToDevice(unsigned char *ledData, size_t ledDataLen, int deviceDescriptor);
void updatePrevLedData(unsigned char *ledData, unsigned char *prevLedData, int ledDataLen);

void getCalculatedLedData(unsigned char *ledData, size_t ledDataLen);
void getSampledLedData(unsigned char *ledData, unsigned char *prevLedData);
void getLedColor(unsigned char *r, unsigned char *g, unsigned char *b, int curHue);

void openXDisplay();
void getScreenResolution();
void calculateSamplePoints();
XColor* getSamplePointColor(Point sampleBoxTopRightPoint);
XImage* getSamplePointImage(Point sampleBoxTopRightPoint);

void calculateGammaTable();
void blendPrevColors(XColor *color, int ledNum, unsigned char *prevLedData);
void correctBrightness(XColor *color);
void correctGamma(XColor *color);

void updateLightPosition(double *lightPosition);
void updateShadowPosition(double *shadowPosition);
void updateHue(int *curHue);

void sigHandler(int sig);
int installSigHandler(int sig, sighandler_t func);
