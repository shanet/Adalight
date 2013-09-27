/*
 *
 * Colorswirl
 *
 * Author: Shane Tully
 *
 * Source:      https://github.com/shanet/Adalight
 * Forked from: https://github.com/adafruit/Adalight
 *
 * This is the host PC-side code written in C; intended for use with a 
 * USB-connected Arduino microcontroller running the accompanying LED 
 * streaming code.  Requires one strand of  Digital RGB LED Pixels 
 * (Adafruit product ID #322, specifically the newer WS2801-based 
 * type, strand of 25) and a 5 Volt power supply (such as 
 * Adafruit #276).  You may need to adapt the code and  the hardware 
 * arrangement for your specific configuration.
 *
 */

#include "colorswirl.h"
#include "usage.h"

int main(int argc, char **argv) {
    int deviceDescriptor;
    char *device = NULL;
    pthread_t threadID;

    // LED color info to send to the device
    // Size is 6 byte header + 3 bytes per LED
    unsigned char ledData[6 + (NUM_LEDS * 3)]; 
    unsigned char prevLedData[6 + (NUM_LEDS * 3)]; 

    // Init globals
    prog             = argv[0];
    noFork           = 0;
    isScreenSampling = 0;
    XDisplay         = NULL;
    startTime        = prevTime = time(NULL);
    color            = MULTI;
    rotationSpeed    = ROT_NORMAL;
    rotationDir      = ROT_CW;
    shadowLength     = SDW_NORMAL;
    fadeSpeed        = FADE_NONE;

    installSigHandler(SIGINT, sigHandler);
    installSigHandler(SIGTERM, sigHandler);

    if(processArgs(argc, argv, &device) == -1) {
        exit(ABNORMAL_EXIT);
    }

    // This is a system service. Fork and return control to whatever started us unless requested otherwise
    pid_t pid;
    if(!noFork) {
        if((pid = fork()) == -1) {
            fprintf(stderr, "%s: Failed to fork on startup\n", prog);
            return ABNORMAL_EXIT;
        } else if(pid != 0) {
            return NORMAL_EXIT;
        }
    }

    startMessageThread(&threadID);

    deviceDescriptor = openDevice(device);

    getLedDataHeader(ledData);

    if(isScreenSampling) {
        memset(prevLedData, 0, sizeof(prevLedData));
        openXDisplay();
        getScreenResolution();
        calculateSamplePoints();
        calculateGammaTable();
    }

    while(1) {
        if(isScreenSampling) {
            getSampledLedData(ledData, prevLedData);
            updatePrevLedData(ledData, prevLedData, sizeof(ledData));
        } else {
            getCalculatedLedData(ledData, sizeof(ledData));
        }

        sendLedDataToDevice(ledData, sizeof(ledData), deviceDescriptor);
    }

    // Close the device and unlink the message queue
    close(deviceDescriptor);
    mq_unlink(MQ_NAME);

    return 0;
}


void startMessageThread(pthread_t *threadID) {
    pthread_create(threadID, NULL, messageLoop, NULL);
}


int openDevice(char *device) {
    int deviceDescriptor = -1;
    struct termios tty;

    // Try to open the device
    if((deviceDescriptor = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1) {
        fprintf(stderr, "%s: Error opening device \"%s\": %s\n", prog, device, strerror(errno));
        exit(ABNORMAL_EXIT);
    }

    // Serial port config swiped from RXTX library (rxtx.qbang.org):
    tcgetattr(deviceDescriptor, &tty);
    tty.c_iflag     = INPCK;
    tty.c_lflag     = 0;
    tty.c_oflag     = 0;
    tty.c_cflag     = CREAD | CS8 | CLOCAL;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tcsetattr(deviceDescriptor, TCSANOW, &tty);

    return deviceDescriptor;
}


void getLedDataHeader(unsigned char *ledData) {
    // Define the header of the LED data to be sent to the Arduino each loop iteration
    ledData[0] = 'A';                            // Magic word
    ledData[1] = 'd';
    ledData[2] = 'a';
    ledData[3] = (NUM_LEDS - 1) >> 8;            // LED count high byte
    ledData[4] = (NUM_LEDS - 1) & 0xff;          // LED count low byte
    ledData[5] = ledData[3] ^ ledData[4] ^ 0x55; // Checksum
}


void updatePrevLedData(unsigned char *ledData, unsigned char *prevLedData, int ledDataLen) {
    for(int i=0; i<ledDataLen; i++) {
        prevLedData[i] = ledData[i];
    }
}


void openXDisplay() {
    if(XDisplay == NULL) {
        XDisplay = XOpenDisplay(NULL);

        if(XDisplay == NULL) {
            fprintf(stderr, "%s: Could not open X display.\n", prog);
            exit(ABNORMAL_EXIT);
        }
    }
}


void getScreenResolution() {
    XWindowAttributes attrs;
    XGetWindowAttributes(XDisplay, DefaultRootWindow(XDisplay), &attrs);

    screenWidth = attrs.width - 1440;
    screenHeight = attrs.height;
}


void calculateSamplePoints() {
    // Determine the width and height of a box
    int samplePointOffset = screenWidth / NUM_LEDS;

    int curX = 0;
    for(int i=0; i<NUM_LEDS; i++) {
        Point *samplePoint = malloc(sizeof(Point));
        
        if(samplePoint == NULL) {
            fprintf(stderr, "%s: Failed to allocate memory.\n", prog);
            exit(ABNORMAL_EXIT);
        }

        samplePoint->x = curX;
        samplePoint->y = 0;

        samplePoints[i] = samplePoint;

        curX += samplePointOffset;
    }
}


void calculateGammaTable() {
    double gamma;
    for(int i=0; i<256; i++) {
        gamma = pow((float)i / 255, 2.8);
        gammaCorrection[i][0] = gamma * 255;
        gammaCorrection[i][1] = gamma * 240;
        gammaCorrection[i][2] = gamma * 220;
    }
}


void getCalculatedLedData(unsigned char *ledData, size_t ledDataLen) {
    static int brightness        = 0;
    static double shadowPosition = 0;
    static double lightPosition  = 0;
    static int hue               = 0;

    static unsigned char red;
    static unsigned char green;
    static unsigned char blue;

    shadowPosition = lightPosition;

    // Start at position 6, after the LED header/magic word
    unsigned int i = 6;
    while(i < ledDataLen) {
        getLedColor(&red, &green, &blue, hue);

        // Resulting hue is multiplied by brightness in the
        // range of 0 to 255 (0 = off, 255 = brightest).
        // Gamma corrrection (the 'pow' function here) adjusts
        // the brightness to be more perceptually linear.
        brightness = (shadowLength != SDW_NONE || rotationSpeed != ROT_NONE) ? (int)(pow(0.5 + sin(shadowPosition) * 0.5, 3.0) * 255.0) : 255;

        ledData[i++] = (red   * brightness) / 255;
        ledData[i++] = (green * brightness) / 255;
        ledData[i++] = (blue  * brightness) / 255;

        // Each pixel is offset in both hue and brightness
        updateShadowPosition(&shadowPosition);
    }

    // If color is multi and fade flag was selected, do a slow fade between colors with the rot speed
    if(fadeSpeed != FADE_NONE && color == MULTI) {
        switch(fadeSpeed) {
            case FADE_VERY_SLOW:
                usleep(1000*180);
                break;
            case FADE_SLOW:
                usleep(1000*130);
                break;
            default:
            case FADE_NORMAL:
                usleep(1000*90);
                break;
            case FADE_FAST:
                usleep(1000*30);
                break;
            case FADE_VERY_FAST:
                usleep(1000*10);
                break;
        }
    }

    // Slowly rotate hue and brightness in opposite directions
    updateHue(&hue);
    updateLightPosition(&lightPosition);
}


void getSampledLedData(unsigned char *ledData, unsigned char *prevLedData) {
    // For the LED data index (j), start at position 6, after the LED header/magic word
    for(unsigned int i=0, j=NUM_LEDS*3; i<NUM_LEDS && j > 6; i++) {
        XColor *color = getSamplePointColor(*(samplePoints[i]));
           
        blendPrevColors(color, i, prevLedData);
        correctBrightness(color);
        //correctGamma(color);

        ledData[--j] = color->red;
        ledData[--j] = color->green;
        ledData[--j] = color->blue;

        XFree(color);
    }
}


XColor* getSamplePointColor(Point sampleBoxTopRightPoint) {
    int boxSize = screenWidth / NUM_LEDS;
    XImage *samplePointImage = getSamplePointImage(sampleBoxTopRightPoint, boxSize, screenHeight);

    XColor *color = malloc(sizeof(XColor));
    if(color == NULL) {
        fprintf(stderr, "%s: Failed to allocate memory.\n", prog);
        exit(ABNORMAL_EXIT);
    }

    int *bucketsRed = calloc(255, sizeof(int));
    int *bucketsGreen = calloc(255, sizeof(int));
    int *bucketsBlue = calloc(255, sizeof(int));

    for(int i=0; i<boxSize; i++) {
        for(int j=0; j<screenHeight; j+=10) {
            unsigned long pixel = XGetPixel(samplePointImage, i, j);

            int red = (pixel >> 16) & 0xff;
            int green = (pixel >> 8)  & 0xff;
            int blue  = (pixel >> 0)  & 0xff;

            bucketsRed[red]++;
            bucketsGreen[green]++;
            bucketsBlue[blue]++;
        }
    }

    color->red   = getModeOfColor(bucketsRed, 255);
    color->green = getModeOfColor(bucketsGreen, 255);
    color->blue  = getModeOfColor(bucketsBlue, 255);

    free(bucketsRed);
    free(bucketsGreen);
    free(bucketsBlue);

    XDestroyImage(samplePointImage);

    return color;
}

XImage* getSamplePointImage(Point sampleBoxTopRightPoint, int width, int height) {
    return XGetImage(XDisplay, RootWindow(XDisplay, DefaultScreen(XDisplay)), sampleBoxTopRightPoint.x, sampleBoxTopRightPoint.y, width, height, AllPlanes, ZPixmap);
}


int getModeOfColor(int *buckets, size_t numBuckets) {
    int max = buckets[0];
    int maxIndex = 0;

    for(unsigned int i=0; i<numBuckets; i++) {
        if(buckets[i] > max) {
            max = buckets[i];
            maxIndex = i;
        }
    }

    return maxIndex;
}


void blendPrevColors(XColor *color, int ledNum, unsigned char *prevLedData) {
    color->red   = (color->red   * BLEND_WEIGHT + prevLedData[ledNum*3]   * FADE) >> 8;
    color->green = (color->green * BLEND_WEIGHT + prevLedData[ledNum*3+1] * FADE) >> 8;
    color->blue  = (color->blue  * BLEND_WEIGHT + prevLedData[ledNum*3+2] * FADE) >> 8;
}


void correctBrightness(XColor *color) {
    // Boost pixels that fall below the minimum brightness
    int brightnessDeficit;
    int colorSum = color->red + color->green + color->blue;
    
    if(colorSum < MIN_BRIGHTNESS) {
        // If all colors are 0, we'd divide by 0 so spread out the deficit equally instead
        if(colorSum == 0) {
            // Spread equally to R,G,B
            brightnessDeficit = MIN_BRIGHTNESS / 3;

            color->red   += brightnessDeficit;
            color->green += brightnessDeficit;
            color->blue  += brightnessDeficit;
        } else {
            // Spread the "brightness deficit" back into R,G,B in proportion to
            // their individual contribition to that deficit.  Rather than simply
            // boosting all pixels at the low end, this allows deep (but saturated)
            // colors to stay saturated...they don't "pink out."
            brightnessDeficit = MIN_BRIGHTNESS - colorSum;
    
            color->red   += brightnessDeficit * (colorSum - color->red)   / (colorSum*2);
            color->green += brightnessDeficit * (colorSum - color->green) / (colorSum*2);
            color->blue  += brightnessDeficit * (colorSum - color->blue)  / (colorSum*2);
        }
    }
}


void correctGamma(XColor *color) {
    color->red   = gammaCorrection[color->red][0];
    color->green = gammaCorrection[color->green][1];
    color->blue  = gammaCorrection[color->blue][2];
}


void getLedColor(unsigned char *r, unsigned char *g, unsigned char *b, int curHue) {
    static unsigned char _r;
    static unsigned char _g;
    static unsigned char _b;
    static unsigned char lo;

    switch(color) {
        case MULTI:
            // Fixed-point hue-to-RGB conversion.  'hue2' is an
            // integer in the range of 0 to 1535, where 0 = red,
            // 256 = yellow, 512 = green, etc.  The high byte
            // (0-5) corresponds to the sextant within the color
            // wheel, while the low byte (0-255) is the
            // fractional part between primary/secondary colors.
            lo = curHue & 255;

            switch((curHue >> 8) % 6) {
                case 0:
                    _r = 255;
                    _g = lo;
                    _b = 0;
                    break;
                case 1:
                    _r = 255 - lo;
                    _g = 255;
                    _b = 0;
                    break;
                case 2:
                    _r = 0;
                    _g = 255;
                    _b = lo;
                    break;
                case 3:
                    _r = 0;
                    _g = 255 - lo;
                    _b = 255;
                    break;
                case 4:
                    _r = lo;
                    _g = 0;
                    _b = 255;
                    break;
                case 5:
                    _r = 255;
                    _g = 0;
                    _b = 255 - lo;
                    break;
            }

            curHue += 40;
            break;
        case RED:
            _r = 255;
            _g = 0;
            _b = 0;
            break;
        case ORANGE:
            _r = 255;
            _g = 165;
            _b = 0;
            break;
        case YELLOW:
            _r = 255;
            _g = 255;
            _b = 0;
            break;
        case GREEN:
            _r = 0;
            _g = 255;
            _b = 0;
            break;
        case BLUE:
            _r = 0;
            _g = 0;
            _b = 255;
            break;
        case PURPLE:
            _r = 128;
            _g = 0;
            _b = 128;
            break;
        case COOL:
            lo = curHue & 255;

            switch((curHue >> 8) % 6) {
                case 0:
                    _r = 0;
                    _g = lo;
                    _b = 0;
                    break;
                case 1:
                    _r = 0;
                    _g = 255;
                    _b = 0;
                    break;
                case 2:
                    _r = 0;
                    _g = 255;
                    _b = lo;
                    break;
                case 3:
                    _r = 0;
                    _g = 255 - lo;
                    _b = 255;
                    break;
                case 4:
                    _r = 0;
                    _g = 0;
                    _b = 255;
                    break;
                case 5:
                    _r = 0;
                    _g = 0;
                    _b = 255 - lo;
                    break;
            }

            curHue += 40;
            break;

        case WHITE:
        default:
            _r = 255;
            _g = 255;
            _b = 255;
    }

    *r = _r;
    *g = _g;
    *b = _b;
}


void updateLightPosition(double *lightPosition) {
    switch(rotationSpeed) {
        case ROT_NONE:
            *lightPosition = 0;
            break;
        case ROT_VERY_SLOW:
            *lightPosition += (rotationDir == ROT_CW) ? -.007 : .007;
            break;
        case ROT_SLOW:
            *lightPosition += (rotationDir == ROT_CW) ? -.015 : .015;
            break;
        case ROT_NORMAL:
        default:
            *lightPosition += (rotationDir == ROT_CW) ? -.03 : .03;
            break;
        case ROT_FAST:
            *lightPosition += (rotationDir == ROT_CW) ? -.045 : .045;
            break;
        case ROT_VERY_FAST:
            *lightPosition += (rotationDir == ROT_CW) ? -.07 : .07;
            break;
    }
}


void updateShadowPosition(double *shadowPosition) {
    switch(shadowLength) {
        case SDW_NONE:
            *shadowPosition += 0;
            break;
        case SDW_VERY_SMALL:
            *shadowPosition += 0.9;
            break;
        case SDW_SMALL:
            *shadowPosition += 0.6;
            break;
        case SDW_NORMAL:
        default:
            *shadowPosition += 0.3;
            break;
        case SDW_LONG:
            *shadowPosition += 0.2;
            break;
        case SDW_VERY_LONG:
            *shadowPosition += 0.08;
            break;
    }
}


void updateHue(int *curHue) {
    static int hue = 0;
    *curHue = hue = (hue + 5) % 1536;
}


void sendLedDataToDevice(unsigned char *ledData, size_t ledDataLen, int deviceDescriptor) {
    static int frame = 0;
    static int totalBytesSent = 0;
    int bytesWritten = 0;

    // Issue color data to LEDs.  Each OS is fussy in different
    // ways about serial output.  This arrangement of drain-and-
    // write-loop seems to be the most relable across platforms:
    tcdrain(deviceDescriptor);
    for(int bytesSent = 0, bytesToGo = ledDataLen; bytesToGo > 0;) {
        // If triple verbose, print out the contents of the LED data in pretty columns
        if(verbose >= TPL_VERBOSE) {
            printf("%s: Sending bytes:\n", prog);
            printf("Magic Word: %c%c%c (%d %d %d)\n", *ledData, *(ledData+1), *(ledData+2), *ledData, *(ledData+1), *(ledData+2));
            printf("LED count high/low byte: %d,%d\n", *(ledData+3), *(ledData+4));
            printf("Checksum: %d\n", *(ledData+5));
            printf("          RED   |  GREEN  |  BLUE\n");

            for(unsigned int i=6; i<ledDataLen; i++) {
                // Print the LED number every 3 loop iterations
                if(i%3 == 0) {
                    printf("LED %2d:   ", i/3 - 1);
                }

                // Print the value in the current index
                printf("%3d   ", ledData[i]);

                // Print column separators for the first two columns and a newline for the third
                if((i-2)%3 != 0) {
                    printf("|   ");
                } else {
                    printf("\n");
                }
            }
            printf("\n\n");
        }

        if((bytesWritten = write(deviceDescriptor, &ledData[bytesSent], bytesToGo)) > 0) {
            bytesToGo -= bytesWritten;
            bytesSent += bytesWritten;
        }
    }

    // Keep track of byte and frame counts for statistics
    totalBytesSent += sizeof(ledData);
    frame++;

    // Update statistics once per second
    if(verbose >= VERBOSE && (curTime = time(NULL)) != prevTime) {
        printf("Average frames/sec: %d, bytes/sec: %d\n", (int)((float)frame / (float)(curTime - startTime)), (int)((float)totalBytesSent / (float)(curTime - startTime)));
        prevTime = curTime;
    }
}


int processArgs(int argc, char **argv, char **device) {
    char c;                   // Char for processing command line args
    int optIndex;             // Index of long opts for processing command line args

    // In order to call getopt() more than once, optind must be reset to 1
    optind = 1;

    // Valid long options
    static struct option longOpts[] = {
        {"color",    required_argument, NULL, 'c'},
        {"rotation", required_argument, NULL, 'r'},
        {"direction",required_argument, NULL, 'd'},
        {"shadow",   required_argument, NULL, 's'},
        {"fade",     optional_argument, NULL, 'f'},
        {"solid",    optional_argument, NULL, 'o'},
        {"sample",   no_argument,       NULL, 'm'},
        {"no-fork",  no_argument,       NULL, 'F'},
        {"verbose",  no_argument,       NULL, 'v'},
        {"version",  no_argument,       NULL, 'V'},
        {"help",     no_argument,       NULL, 'h'},
        {NULL,       0,                 0,      0}
    };

    // Parse the command line args
    while((c = getopt_long(argc, argv, "c:r:d:s:f::o::mFhvVp:", longOpts, &optIndex)) != -1) {
        switch (c) {
            // Color
            case 'c':
                if     (strcmp(optarg, "multi")  == 0) color = MULTI;
                else if(strcmp(optarg, "red")    == 0) color = RED;
                else if(strcmp(optarg, "orange") == 0) color = ORANGE;
                else if(strcmp(optarg, "yellow") == 0) color = YELLOW;
                else if(strcmp(optarg, "green")  == 0) color = GREEN;
                else if(strcmp(optarg, "blue")   == 0) color = BLUE;
                else if(strcmp(optarg, "purple") == 0) color = PURPLE;
                else if(strcmp(optarg, "white")  == 0) color = WHITE;
                else if(strcmp(optarg, "cool")   == 0) color = COOL;
                else {
                    printUsage(prog);
                    return -1;
                }
                break;
            // Rotation speed
            case 'r':
                if     (strcmp(optarg, "none")      == 0 || strcmp(optarg, "n")  == 0) rotationSpeed = ROT_NONE;
                else if(strcmp(optarg, "very_slow") == 0 || strcmp(optarg, "vs") == 0) rotationSpeed = ROT_VERY_SLOW;
                else if(strcmp(optarg, "slow")      == 0 || strcmp(optarg, "s")  == 0) rotationSpeed = ROT_SLOW;
                else if(strcmp(optarg, "normal")    == 0)                              rotationSpeed = ROT_NORMAL;
                else if(strcmp(optarg, "fast")      == 0 || strcmp(optarg, "f")  == 0) rotationSpeed = ROT_FAST;
                else if(strcmp(optarg, "very_fast") == 0 || strcmp(optarg, "vf") == 0) rotationSpeed = ROT_VERY_FAST;
                else {
                    printUsage(prog);
                    return -1;
                }
                break;
            // Rotation direction
            case 'd':
                if     (strcmp(optarg, "cw")  == 0) rotationDir = ROT_CW;
                else if(strcmp(optarg, "ccw") == 0) rotationDir = ROT_CCW;
                else {
                    printUsage(prog);
                    return -1;
                }
                break;
            // Shadow length
            case 's':
                if     (strcmp(optarg, "none")       == 0 || strcmp(optarg, "n")  == 0) shadowLength = SDW_NONE;
                else if(strcmp(optarg, "very_small") == 0 || strcmp(optarg, "vs") == 0) shadowLength = SDW_VERY_SMALL;
                else if(strcmp(optarg, "small")      == 0 || strcmp(optarg, "s")  == 0) shadowLength = SDW_SMALL;
                else if(strcmp(optarg, "normal")     == 0)                              shadowLength = SDW_NORMAL;
                else if(strcmp(optarg, "long")       == 0 || strcmp(optarg, "l")  == 0) shadowLength = SDW_LONG;
                else if(strcmp(optarg, "very_long")  == 0 || strcmp(optarg, "vl") == 0) shadowLength = SDW_VERY_LONG;
                else {
                    printUsage(prog);
                    return -1;
                }
                break;
            // Fade
            case 'f':
                if     (optarg == NULL || strcmp(optarg, "slow") == 0 || strcmp(optarg, "s") == 0) rotationSpeed = ROT_SLOW;
                else if(strcmp(optarg, "very_slow") == 0 || strcmp(optarg, "vs") == 0) rotationSpeed = ROT_VERY_SLOW;
                else if(strcmp(optarg, "normal")    == 0 || strcmp(optarg, "n")  == 0) rotationSpeed = ROT_NORMAL;
                else if(strcmp(optarg, "fast")      == 0 || strcmp(optarg, "f")  == 0) rotationSpeed = ROT_FAST;
                else if(strcmp(optarg, "very_fast") == 0 || strcmp(optarg, "vf") == 0) rotationSpeed = ROT_VERY_FAST;
                else {
                    printUsage(prog);
                    return -1;
                }
                shadowLength = SDW_NONE;
                break;
            // Solid
            case 'o':
                if     (optarg == NULL || strcmp(optarg, "slow") == 0 || strcmp(optarg, "s") == 0) fadeSpeed = FADE_SLOW;
                else if(strcmp(optarg, "very_slow") == 0 || strcmp(optarg, "vs") == 0) fadeSpeed = FADE_VERY_SLOW;
                else if(strcmp(optarg, "normal")    == 0 || strcmp(optarg, "n")  == 0) fadeSpeed = FADE_NORMAL;
                else if(strcmp(optarg, "fast")      == 0 || strcmp(optarg, "f")  == 0) fadeSpeed = FADE_FAST;
                else if(strcmp(optarg, "very_fast") == 0 || strcmp(optarg, "vf") == 0) fadeSpeed = FADE_VERY_FAST;
                else {
                    printUsage(prog);
                    return -1;
                }
                rotationSpeed = ROT_NONE;
                shadowLength = SDW_NONE;
                break;
            // Screen sampling
            case 'm':
                isScreenSampling = 1;
                fprintf(stderr, "%s: WARNING: Screen sampling does not work very well. Feel free to improve it and submit a pull request. :)\n", prog);
                break;
            // No fork
            case 'F':
                noFork = 1;
                break;
            // Print help
            case 'h':
                printUsage(prog);
                exit(NORMAL_EXIT);
            // Print version
            case 'V':
                printVersion(prog);
                exit(NORMAL_EXIT);
            // Set verbosity level
            case 'v':
                verbose++;
                break;
            case '?':
            default:
                printUsage(prog);
                return -1;
        }
    }

    // Get the device we're using (if device is NULL don't worry about it; this happens if updating from a message)
    if(argc == optind+1 && device != NULL) {
        *device = argv[optind];
    } else if(argc > optind) {
        fprintf(stderr, "%s: Too many arguments specified.\n", prog);
        printUsage(prog);
        return -1;
    } else if(device != NULL) {
        // If a device wasn't specified, try a common one
        printf("%s: Device not specified. Defaulting to \"%s\".\n", prog, DEFAULT_DEVICE);
        *device = DEFAULT_DEVICE;
    }

    return 0;
}


void* messageLoop(void *threadID) {
    mqd_t mqd = -1;
    int argc;
    int msgLen = 0;

    // Do something with threadID to make GCC happy and get rid of the unused parameter warning
    threadID++;

    // Set the max message length as MAX_MSG_LEN
    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_msgsize = MAX_MSG_LEN,
        .mq_curmsgs = 0
    };

    // Open the message queue if it isn't already open
    if((mqd = mq_open(MQ_NAME, O_RDONLY | O_CREAT, 0600, &attr)) == -1) {
        fprintf(stderr, "Error opening queue: %s. Exiting message queue thread.\n", strerror(errno));
        pthread_exit(NULL);
    }

    if(verbose >= DBL_VERBOSE) {
        printf("%s: Connected to message queue. Waiting for messages...\n", prog);
    }

    while(1) {
        // Queue attributes
        if(mq_getattr(mqd, &attr) == -1) {
            fprintf(stderr, "Error getting message queue attributes: %s. Exiting message queue thread.\n", strerror(errno));
            pthread_exit(NULL);
        }

        // Create the buffer for the queue message from the max message size of the queue
        char *message = malloc(attr.mq_msgsize);
        if(message == NULL) {
            fprintf(stderr, "Failed to allocate memory for message queue buffer. Exiting message queue thread.\n");
            pthread_exit(NULL);
        }

        // Get the first message which is the argument count
        if((msgLen = mq_receive(mqd, message, attr.mq_msgsize, 0)) == -1) {
            fprintf(stderr, "Failed to recieve message in queue: %s\n", strerror(errno));
            continue;
        }
        message[msgLen] = '\0';

        if(verbose >= DBL_VERBOSE) {
            fprintf(stderr, "%s: Got message (should be number of arguments): %s\n", prog, message);
        }

        // Convert argc message to an int
        sscanf(message, "%d", &argc);

        // Get the arguments in a flattened string
        if((msgLen = mq_receive(mqd, message, attr.mq_msgsize, 0)) == -1) {
            fprintf(stderr, "Failed to recieve message in queue: %s\n", strerror(errno));
            continue;
        }
        message[msgLen] = '\0';

        if(verbose >= DBL_VERBOSE) {
            fprintf(stderr, "%s: Got message (should be %d arguments): %s\n", prog, argc, message);
        }

        // Tokenize the buffer back to an array
        char **argv = malloc(attr.mq_msgsize);
        char *arg = strtok(message, " ");
        int i = 0;
        while(arg != NULL) {
            argv[i] = arg;
            arg = strtok(NULL, " ");
            i++;
        }

        // Pass on the new argumentsto the process args function to update the global behavior variables
        processArgs(argc, argv, NULL);

        free(argv);
        free(message);
        argv = NULL;
        message = NULL;
    }

    pthread_exit(NULL);
}


void sigHandler(int sig) {
    if(sig != SIGINT && sig != SIGTERM) {
        fprintf(stderr, "%s: Signal handler recieved incorrect signal", prog);
        return;
    }

    // Delete the message queue
    if(verbose >= DBL_VERBOSE) {
        printf("%s: Deleting message queue\n", prog);
    }
    mq_unlink(MQ_NAME);

    exit(0);
}


int installSigHandler(int sig, sighandler_t func) {
    struct sigaction sigact;
    
    sigact.sa_handler = func;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESTART;

    if(sigaction(sig, &sigact, NULL) == -1) {
        fprintf(stderr, "%s: Failed to install signal hander for signal %d: %s\n", prog, sig, strerror(errno));
        return -1;
    }

    return 0;
}