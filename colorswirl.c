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

#include "colorswirl.h"

int main(int argc, char **argv) {
    int brightness;           // Current brightness
    double shadowPosition;    // Current shadow position
    double curLightPosition;  // Current light position (related to rotation speed)
    int curHue = 0;           // Current hue
    unsigned char r;          // Current red value
    unsigned char g;          // Current green value
    unsigned char b;          // Current blue value
        
    int fd;                   // File descriptor of the open device
    char *device = NULL;      // The device to send LED info to

    int _argc = 0;
    char **_argv = NULL;

    unsigned char buffer[6 + (NUM_LEDS * 3)]; // Header + 3 bytes per LED

    // Init globals
    prog = argv[0];
    startTime = prevTime = time(NULL);
    color = MULTI;
    rotationSpeed = ROT_NORMAL;
    rotationDir = ROT_CW;
    shadowLength = SDW_NORMAL;

    // Install SIGINT and SIGTERM handlers
    installSigHandler(SIGINT, sigHandler);
    installSigHandler(SIGTERM, sigHandler);

    // Process command line args
    device = processArgs(argc, argv);

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

    // Clean up
    close(fd);
    mq_unlink(MQ_NAME);
    free(_argv);
    _argv = NULL;

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
            printf("          RED   |  GREEN  |  BLUE\n");

            for(unsigned int i=6; i<bufLen; i++) {
                // Print the LED number every 3 loop iterations
                if(i%3 == 0) {
                    printf("LED %2d:   ", i/3 - 1);
                }

                // Print the value in the buffer
                printf("%3d   ", buffer[i]);

                // Print column separators for the first two columns and a newline for the third
                if((i-2)%3 != 0) {
                    printf("|   ");
                } else {
                    printf("\n");
                }
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


void processArgs(int argc, char **argv) {
    char c;                   // Char for processing command line args
    int optIndex;             // Index of long opts for processing command line args
    char *device = NULL;      // The device to send LED info to

    // In order to call getopt() more than once, optind must be reset to 1
    optind = 1;

    // Valid long options
    static struct option longOpts[] = {
        {"color", required_argument, NULL, 'c'},
        {"rotation", required_argument, NULL, 'r'},
        {"direction", required_argument, NULL, 'd'},
        {"shadow", required_argument, NULL, 's'},
        {"fade", optional_argument, NULL, 'f'},
        {"solid", no_argument, NULL, 'o'},
        {"verbose", no_argument, NULL, 'v'},
        {"version", no_argument, NULL, 'V'},
        {"help", no_argument, NULL, 'h'}
    };

    // Parse the command line args
    while((c = getopt_long(argc, argv, "c:r:d:s:f::ohvVp:", longOpts, &optIndex)) != -1) {
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
                    exit(ABNORMAL_EXIT);
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
                    exit(ABNORMAL_EXIT);
                }
                break;
            // Rotation direction
            case 'd':
                if(strcmp(optarg, "cw") == 0) rotationDir = ROT_CW;
                else if(strcmp(optarg, "ccw") == 0) rotationDir = ROT_CCW;
                else {
                    printUsage();
                    exit(ABNORMAL_EXIT);
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
                    exit(ABNORMAL_EXIT);
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
                    exit(ABNORMAL_EXIT);
                }
                shadowLength = SDW_NONE;
                break;
            // Solid
            case 'o':
                rotationSpeed = ROT_NONE;
                shadowLength = SDW_NONE;
                break;
            // Print help
            case 'h':
                printUsage();
                exit(NORMAL_EXIT);
            // Print version
            case 'V':
                printVersion();
                exit(NORMAL_EXIT);
            // Set verbosity level
            case 'v':
                verbose++;
                break;
            case '?':
            default:
                fprintf(stderr, "%s: Use \"%s -h\" for usage information.\n", prog, prog);
                exit(ABNORMAL_EXIT);
        }
    }

    // Get the device we're using
    if(argc == optind+1) {
        device = argv[optind];
    } else if(argc > optind) {
        fprintf(stderr, "%s: Too many arguments specified.\n", prog);
        printUsage();
        exit(ABNORMAL_EXIT);
    } else {
        // If a device wasn't specified, try a common one
        printf("%s: Device not specified. Defaulting to \"%s\".\n", prog, DEFAULT_DEVICE);
        device = DEFAULT_DEVICE;
    }
}


char** getMessage(int *argc) {

            // Check for new messages
        if((_argv = getMessage(&_argc)) != NULL) {
            // If there was a message, pass it on the process args to update the global behavior variables
            processArgs(_argc, _argv);
        }

    static mqd_t mqd = -1;
    struct mq_attr attr;

    // Open the message queue if it isn't already open
    if(mqd == -1 && (mqd = mq_open(MQ_NAME, O_RDONLY | O_NONBLOCK)) == -1) {
        // Only display an error if the error opening the queue was something other
        // than the queue not existing
        if(errno != ENOENT) {
            fprintf(stderr, "Error opening queue: %s\n", strerror(errno));
        }
        return NULL;
    }

    if(verbose >= TPL_VERBOSE) {
        printf("%s: Connected to message queue. Checking for messages...\n", prog);
    }

    while(1) {
    // Queue attributes
    if(mq_getattr(mqd, &attr) == -1) {
        fprintf(stderr, "Error getting message queue attributes: %s\n", strerror(errno));
        return NULL;
    }

    // Create the buffer for the queue message from the max message size of the queue
    char *buf = malloc(attr.mq_msgsize);
    if(buf == NULL) {
        fprintf(stderr, "Failed to allocate memory for message queue buffer.\n");
        return NULL;
    }

    if(verbose >= DBL_VERBOSE) {
        printf("%s: Number of messages in queue: %d\n", prog, (int)attr.mq_curmsgs);
    }

    // Check that there are at least two messages in the queue
    if(attr.mq_curmsgs < 2) return NULL;

    // Get the first message which is the argument count
    if(mq_receive(mqd, buf, attr.mq_msgsize, 0) == -1) {
        fprintf(stderr, "Failed to recieve message in queue: %s\n", strerror(errno));
    }

    if(verbose >= DBL_VERBOSE) {
        fprintf(stderr, "%s: Got message: %s\n", prog, buf);
    }

    // Convert argc message to an int
    sscanf(buf, "%d", argc);

    // Get the arguments in a flattened string
    if(mq_receive(mqd, buf, attr.mq_msgsize, 0) == -1) {
        fprintf(stderr, "Failed to recieve message in queue: %s\n", strerror(errno));
    }

    if(verbose >= DBL_VERBOSE) {
        fprintf(stderr, "%s: Got message: %s\n", prog, buf);
    }

    // Tokenize the buffer back to an array
    char **argv = malloc(attr.mq_msgsize);
    char *arg = strtok(buf, " ");
    int i = 0;
    while(arg != NULL) {
        argv[i] = arg;
        arg = strtok(NULL, " ");
        i++;
    }

    return argv;
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


void printUsage(void) {
	printf("Usage: %s [options] [device]\n", prog);
    
    printf("\t--color\t\t-c\t\tColor to use\n");
    printf("\t\tSupported colors:\n\t\t  multi (default)\n\t\t  red\n\t\t  orange\n\t\t  yellow\n\t\t  green\n\t\t  blue\n\t\t  purple\n\t\t  white\n\n");
    
    printf("\t--rotation\t-r\t\tRotation speed\n");
    printf("\t\tSupported rotation speeds:\n\t\t  n\tnone\n\t\t  vs\tvery_slow\n\t\t  s\tslow\n\t\t  \tnormal (default)\n\t\t  f\tfast\n\t\t  vf\tvery_fast\n\n");
    
    printf("\t--direction\t-d\t\tRotation direction\n");
    printf("\t\tSupported directions:\n\t\t  cw\tClockwise (default)\n\t\t  ccw\tCounter-clockwise\n\n");
    
    printf("\t--shadow\t-s\t\tShadow length\n");
    printf("\t\tSupported shadow lengths:\n\t\t  n\tnone\n\t\t  vs\tvery_small\n\t\t  s\tsmall\n\t\t  \tnormal (default)\n\t\t  l\tlong\n\t\t  vl\tvery_long\n\t\t\n\n");

    printf("\t--fade\t\t-f\t\tFade speed (shorthand for no rotation, slow shadow)\n");
    printf("\t\tThis option takes an optional argument. Arguments must be specified \n\t\tvia the form \"--fade=[speed]\" or \"-f[speed]\"\n");
    printf("\t\tSupported fade speeds:\n\t\t  vs\tvery_slow\n\t\t  s\tslow (default)\n\t\t  n\tnormal\n\t\t  f\tfast\n\t\t  vf\tvery_fast\n\n");
    
    printf("\t--solid\t\t-o\t\tShorthand for no rotation, no shadow\n");
    printf("\t\tTakes no arguments. Simply shows the selected color at full brightness\n\n");
    
    printf("\t--verbose\t-v\t\tIncrease verbosity. Can be specified multiple times.\n");
    printf("\t\tSingle verbose will show \"frame rate\" and bytes/sec. Double verbose is \n\t\tshows message queue info. Triple verbose will show all info\n\t\t\
being sent to the device. This is useful for visualizing how the options\n\t\tabove affect what data is sent to the device.\n\n");

    printf("\t--version\t-V\t\tDisplay version and exit\n");
    printf("\t--help\t\t-h\t\tDisplay this message and exit\n\n");
    
    printf("\tDevice is the path to the block device to write data to. If not specified,\n\tdefaults to \"%s\"\n\n", DEFAULT_DEVICE);

    printf("\tOptions are parsed from left to right. For example, specifying --solid and then\n\t--shadow will NOT result in a solid color.\
            \n\n\tIf all this seems confusing, just play with the options and try triple verbose.\n");
}


void printVersion(void) {
	printf("%s: Version %s\n", prog, VERSION);
}