/*
"Colorswirl" LED demo.  This is the host PC-side code written in C;
intended for use with a USB-connected Arduino microcontroller running the
accompanying LED streaming code.  Requires one strand of Digital RGB LED
Pixels (Adafruit product ID #322, specifically the newer WS2801-based type,
strand of 25) and a 5 Volt power supply (such as Adafruit #276).  You may
need to adapt the code and the hardware arrangement for your specific
configuration.

This is a command-line program.  It expects a single parameter, which is
the serial port device name, e.g.:

	./colorswirl /dev/tty.usbserial-A60049KO

*/

#include "colorswirl.h"

int main(int argc, char *argv[]) {
    int brightness;
    double shadowPosition;
    double curLightPosition;
    int curHue = 0;
    unsigned char r;
    unsigned char g;
    unsigned char b;
        
    int fd;                // File descriptor of the open device
    char *device = NULL;   // The device to send LED info to

    char c;                // Char for processing command line args
    int optIndex;          // Index of long opts for processing command line args

    unsigned char buffer[6 + (NUM_LEDS * 3)]; // Header + 3 bytes per LED

    prog = argv[0];
    startTime = prevTime = time(NULL);
    color = MULTI;
    rotationSpeed = ROT_NORMAL;
    rotationDir = ROT_CW;
    shadowLength = SDW_NORMAL;

    // Valid long options
    static struct option longOpts[] = {
        {"color", required_argument, NULL, 'c'},
        {"rotation", required_argument, NULL, 'r'},
        {"direction", required_argument, NULL, 'd'},
        {"shadow", required_argument, NULL, 's'},
        {"fade", optional_argument, NULL, 'f'},
        {"verbose", no_argument, NULL, 'v'},
        {"version", no_argument, NULL, 'V'},
        {"help", no_argument, NULL, 'h'}
    };

    // Parse the command line args
    while((c = getopt_long(argc, argv, "c:r:d:s:f::hvVp:", longOpts, &optIndex)) != -1) {
        switch (c) {
            // Color
            case 'c':
                if(strcmp(optarg, "multi") == 0) color = MULTI;
                else if(strcmp(optarg, "red") == 0) color = RED;
                else if(strcmp(optarg, "orange") == 0) color = ORANGE;
                else if(strcmp(optarg, "yellow") == 0) color = YELLOW;
                else if(strcmp(optarg, "green") == 0) color = GREEN;
                else if(strcmp(optarg, "blue") == 0) color = BLUE;
                else if(strcmp(optarg, "purple") == 0) color = PURPLE;
                else if(strcmp(optarg, "white") == 0) color = WHITE;
                else {
                    printUsage();
                    return ABNORMAL_EXIT;
                }
                break;
            // Rotation speed
            case 'r':
                if(strcmp(optarg, "none") == 0 || strcmp(optarg, "n") == 0) rotationSpeed = ROT_NONE;
                else if(strcmp(optarg, "very_slow") == 0 || strcmp(optarg, "vs") == 0) rotationSpeed = ROT_VERY_SLOW;
                else if(strcmp(optarg, "slow") == 0 || strcmp(optarg, "s") == 0) rotationSpeed = ROT_SLOW;
                else if(strcmp(optarg, "normal") == 0) rotationSpeed = ROT_NORMAL;
                else if(strcmp(optarg, "fast") == 0 || strcmp(optarg, "f") == 0) rotationSpeed = ROT_FAST;
                else if(strcmp(optarg, "very_fast") == 0 || strcmp(optarg, "vf") == 0) rotationSpeed = ROT_VERY_FAST;
                else {
                    printUsage();
                    return ABNORMAL_EXIT;
                }
                break;
            // Rotation direction
            case 'd':
                if(strcmp(optarg, "cw") == 0) rotationDir = ROT_CW;
                else if(strcmp(optarg, "ccw") == 0) rotationDir = ROT_CCW;
                else {
                    printUsage();
                    return ABNORMAL_EXIT;
                }
                break;
            // Shadow length
            case 's':
                if(strcmp(optarg, "none") == 0 || strcmp(optarg, "n") == 0) shadowLength = SDW_NONE;
                else if(strcmp(optarg, "very_small") == 0 || strcmp(optarg, "vs") == 0) shadowLength = SDW_VERY_SMALL;
                else if(strcmp(optarg, "small") == 0 || strcmp(optarg, "s") == 0) shadowLength = SDW_SMALL;
                else if(strcmp(optarg, "normal") == 0) shadowLength = SDW_NORMAL;
                else if(strcmp(optarg, "long") == 0 || strcmp(optarg, "l") == 0) shadowLength = SDW_LONG;
                else if(strcmp(optarg, "very_long") == 0 || strcmp(optarg, "vl") == 0) shadowLength = SDW_VERY_LONG;
                else {
                    printUsage();
                    return ABNORMAL_EXIT;
                }
                break;
            // Fade
            case 'f':
                if(optarg == NULL || strcmp(optarg, "slow") == 0 || strcmp(optarg, "s") == 0) rotationSpeed = ROT_SLOW;
                else if(strcmp(optarg, "very_slow") == 0 || strcmp(optarg, "vs") == 0) rotationSpeed = ROT_VERY_SLOW;
                else if(strcmp(optarg, "normal") == 0 || strcmp(optarg, "n") == 0) rotationSpeed = ROT_NORMAL;
                else if(strcmp(optarg, "fast") == 0 || strcmp(optarg, "f") == 0) rotationSpeed = ROT_FAST;
                else if(strcmp(optarg, "very_fast") == 0 || strcmp(optarg, "vf") == 0) rotationSpeed = ROT_VERY_FAST;
                else {
                    printUsage();
                    return ABNORMAL_EXIT;
                }
                shadowLength = SDW_NONE;
                break;
            // Print help
            case 'h':
                printUsage();
                return NORMAL_EXIT;
            // Print version
            case 'V':
                printVersion();
                return NORMAL_EXIT;
            // Set verbosity level
            case 'v':
                verbose++;
                break;
            case '?':
            default:
                fprintf(stderr, "%s: Try \"%s -h\" for usage information.\n", prog, prog);
                return ABNORMAL_EXIT;
        }
    }

    // Get the device we're using
    if(argc == optind+1) {
    	device = argv[optind];
	} else if(argc > optind) {
        fprintf(stderr, "%s: Too many arguments specified.\n", prog);
		printUsage();
		return ABNORMAL_EXIT;
	} else {
        // If a device wasn't specified, try a common one
        printf("%s: Device not specified. Defaulting to \"%s\".\n", prog, DEFAULT_DEVICE);
        device = DEFAULT_DEVICE;
    }

    // Open the device
    if((fd = openTTY(device)) == -1) {
        exit(ABNORMAL_EXIT);
    }

    // Clear LED buffer
    memset(buffer, 0, sizeof(buffer));

    // Header only needs to be initialized once, not
    // inside rendering loop -- number of LEDs is constant:
    buffer[0] = 'A';                            // Magic word
    buffer[1] = 'd';
    buffer[2] = 'a';
    buffer[3] = (NUM_LEDS - 1) >> 8;            // LED count high byte
    buffer[4] = (NUM_LEDS - 1) & 0xff;          // LED count low byte
    buffer[5] = buffer[3] ^ buffer[4] ^ 0x55;   // Checksum

    while(1) {
        shadowPosition = curLightPosition;

        // Start at position 6, after the LED header/magic word
        unsigned int i = 6;
        while(i < sizeof(buffer)) {
            updateColor(&r, &g, &b, curHue);

            // Resulting hue is multiplied by brightness in the
            // range of 0 to 255 (0 = off, 255 = brightest).
            // Gamma corrrection (the 'pow' function here) adjusts
            // the brightness to be more perceptually linear.
            brightness = (shadowLength != SDW_NONE || rotationSpeed != ROT_NONE) ? (int)(pow(0.5 + sin(shadowPosition) * 0.5, 3.0) * 255.0) : 255;

            buffer[i++] = (r * brightness) / 255;
            buffer[i++] = (g * brightness) / 255;
            buffer[i++] = (b * brightness) / 255;

            // Each pixel is offset in both hue and brightness
            updateShadowPosition(&shadowPosition);
        }

        // Slowly rotate hue and brightness in opposite directions
        updateHue(&curHue);
        updateLightPosition(&curLightPosition);

        // Send the data to the buffer
        sendBuffer(buffer, sizeof(buffer), fd);
    }

    close(fd);
    return 0;
}


int openTTY(char *device) {
    int fd = -1;
    struct termios tty;

    // Try to open the device
    if((fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1) {
        fprintf(stderr, "%s: Error opening device \"%s\": %s\n", prog, device, strerror(errno));
        return -1;
    }

    // Serial port config swiped from RXTX library (rxtx.qbang.org):
    tcgetattr(fd, &tty);
    tty.c_iflag = INPCK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cflag = CREAD | CS8 | CLOCAL;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tcsetattr(fd, TCSANOW, &tty);

    return fd;
}


void updateColor(unsigned char *r, unsigned char *g, unsigned char *b, int curHue) {
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

void sendBuffer(unsigned char *buffer, size_t bufLen, int fd) {
    static int frame = 0;
    static int totalBytesSent = 0;
    int bytesWritten = 0;

    // Issue color data to LEDs.  Each OS is fussy in different
    // ways about serial output.  This arrangement of drain-and-
    // write-loop seems to be the most relable across platforms:
    tcdrain(fd);
    for(int bytesSent = 0, bytesToGo = bufLen; bytesToGo > 0;) {
        // If triple verbose, print out the contents of buffer in pretty columns
        if(verbose >= TPL_VERBOSE) {
            printf("%s: Sending bytes:\n", prog);
            printf("Magic Word: %c%c%c (%d %d %d)\n", *buffer, *(buffer+1), *(buffer+2), *buffer, *(buffer+1), *(buffer+2));
            printf("LED count high/low byte: %d,%d\n", *(buffer+3), *(buffer+4));
            printf("Checksum: %d\n", *(buffer+5));
            printf("          RED   |  GREEN  |   BLUE\n");
            for(unsigned int i=6; i<bufLen; i++) {
                if(i%3 == 0) printf("LED %2d:   ", i/3 - 1);
                printf("%3d   |   ", buffer[i]);
                if((i-2)%3 == 0) printf("\n");
            }
            printf("\n\n");
        }

        if((bytesWritten = write(fd, &buffer[bytesSent], bytesToGo)) > 0) {
            bytesToGo -= bytesWritten;
            bytesSent += bytesWritten;
        }
    }

    // Keep track of byte and frame counts for statistics
    totalBytesSent += sizeof(buffer);
    frame++;

    // Update statistics once per second
    if(verbose >= VERBOSE && (curTime = time(NULL)) != prevTime) {
        printf("Average frames/sec: %d, bytes/sec: %d\n", (int)((float)frame / (float)(curTime - startTime)), (int)((float)totalBytesSent / (float)(curTime - startTime)));
        prevTime = curTime;
    }
}


void printUsage(void) {
	printf("Usage: %s device\n", prog);
}


void printVersion(void) {
	printf("%s: Version %s\n", prog, VERSION);
}