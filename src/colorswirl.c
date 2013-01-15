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
    int brightness;           // Current brightness
    double shadowPosition;    // Current shadow position
    double curLightPosition;  // Current light position (related to rotation speed)
    int curHue = 0;           // Current hue
    unsigned char r;          // Current red value
    unsigned char g;          // Current green value
    unsigned char b;          // Current blue value
        
    int fd;                   // File descriptor of the open device
    char *device = NULL;      // The device to send LED info to
    pthread_t tid;            // Thread ID of the message thread


    unsigned char buffer[6 + (NUM_LEDS * 3)]; // Header + 3 bytes per LED

    // Init globals
    prog          = argv[0];
    noFork        = 0;
    startTime     = prevTime = time(NULL);
    color         = MULTI;
    rotationSpeed = ROT_NORMAL;
    rotationDir   = ROT_CW;
    shadowLength  = SDW_NORMAL;
    fadeSpeed     = FADE_NONE;

    // Install SIGINT and SIGTERM handlers
    installSigHandler(SIGINT, sigHandler);
    installSigHandler(SIGTERM, sigHandler);

    // Process command line args
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

    // Start the message thread
    pthread_create(&tid, NULL, messageLoop, NULL);

    // Open the device
    if((fd = openDevice(device)) == -1) {
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
            getLedColor(&r, &g, &b, curHue);

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
        updateHue(&curHue);
        updateLightPosition(&curLightPosition);

        // Send the data to the buffer
        sendBufferToDevice(buffer, sizeof(buffer), fd);
    }

    // Close the device
    close(fd);

    // Unlink the message queue
    mq_unlink(MQ_NAME);

    return 0;
}


int openDevice(char *device) {
    int fd = -1;
    struct termios tty;

    // Try to open the device
    if((fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1) {
        fprintf(stderr, "%s: Error opening device \"%s\": %s\n", prog, device, strerror(errno));
        return -1;
    }

    // Serial port config swiped from RXTX library (rxtx.qbang.org):
    tcgetattr(fd, &tty);
    tty.c_iflag     = INPCK;
    tty.c_lflag     = 0;
    tty.c_oflag     = 0;
    tty.c_cflag     = CREAD | CS8 | CLOCAL;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tcsetattr(fd, TCSANOW, &tty);

    return fd;
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


void sendBufferToDevice(unsigned char *buffer, size_t bufLen, int fd) {
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
        {"no-fork",  no_argument,       NULL, 'F'},
        {"verbose",  no_argument,       NULL, 'v'},
        {"version",  no_argument,       NULL, 'V'},
        {"help",     no_argument,       NULL, 'h'},
        {NULL,      0,                  0,      0}
    };

    // Parse the command line args
    while((c = getopt_long(argc, argv, "c:r:d:s:f::o::hvVp:", longOpts, &optIndex)) != -1) {
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


void* messageLoop(void *tid) {
    mqd_t mqd = -1;
    int argc;
    int msgLen = 0;

    // Do something with tid to make GCC happy and get rid of the unused parameter warning
    tid++;

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