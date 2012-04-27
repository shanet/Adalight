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
    int hue1;
    int hue2;
    int brightness;
    unsigned char lo;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    
    double sine1;
    double sine2;
    
    int fd;                // File descriptor of the open device
    char *device;          // The device to send LED info to

    char c;                // Char for processing command line args
    int optIndex;          // Index of long opts for processing command line args

    unsigned char buffer[6 + (N_LEDS * 3)]; // Header + 3 bytes per LED

    prog = argv[0];
    startTime = prevTime = time(NULL);

    // Valid long options
    static struct option longOpts[] = {
        {"verbose", no_argument, NULL, 'v'},
        {"version", no_argument, NULL, 'V'},
        {"help", no_argument, NULL, 'h'}
    };

    // Parse the command line args
    while((c = getopt_long(argc, argv, "hvVp:", longOpts, &optIndex)) != -1) {
        switch (c) {
            // Print help
            case 'h':
                printUsage();
                return NORMAL_EXIT;
            // Print version
            case 'V':
                printVersion();
                return NORMAL_EXIT;
            // Set verbose level
            case 'v':
                verbose++;
                break;
            case '?':
                fprintf(stderr, "%s: Try \"%s -h\" for usage information.\n", prog, prog);
            default:
                return ABNORMAL_EXIT;
        }
    }

    // Get the device we're using
    if(argc >= optind) {
    	device = argv[optind];
	} else {
		printUsage();
		return ABNORMAL_EXIT;
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
    buffer[3] = (N_LEDS - 1) >> 8;              // LED count high byte
    buffer[4] = (N_LEDS - 1) & 0xff;            // LED count low byte
    buffer[5] = buffer[3] ^ buffer[4] ^ 0x55;   // Checksum

    sine1 = 0.0;
    hue1 = 0;

    while(1) {
        sine2 = sine1;
        hue2 = hue1;

        // Start at position 6, after the LED header/magic word
        for(unsigned int i=6; i<sizeof(buffer);) {
            // Fixed-point hue-to-RGB conversion.  'hue2' is an
            // integer in the range of 0 to 1535, where 0 = red,
            // 256 = yellow, 512 = green, etc.  The high byte
            // (0-5) corresponds to the sextant within the color
            // wheel, while the low byte (0-255) is the
            // fractional part between primary/secondary colors.
            lo = hue2 & 255;

            //r = 0;
            //g = 255;
            //b = 0;

            switch ((hue2 >> 8) % 6) {
                case 0:
                    r = 255;
                    g = lo;
                    b = 0;
                    break;
                case 1:
                    r = 255 - lo;
                    g = 255;
                    b = 0;
                    break;
                case 2:
                    r = 0;
                    g = 255;
                    b = lo;
                    break;
                case 3:
                    r = 0;
                    g = 255 - lo;
                    b = 255;
                    break;
                case 4:
                    r = lo;
                    g = 0;
                    b = 255;
                    break;
                case 5:
                    r = 255;
                    g = 0;
                    b = 255 - lo;
                    break;
            }

            // Resulting hue is multiplied by brightness in the
            // range of 0 to 255 (0 = off, 255 = brightest).
            // Gamma corrrection (the 'pow' function here) adjusts
            // the brightness to be more perceptually linear.
            brightness = (int)(pow(0.5 + sin(sine2) * 0.5, 3.0) * 255.0);
            buffer[i++] = (r * brightness) / 255;
            buffer[i++] = (g * brightness) / 255;
            buffer[i++] = (b * brightness) / 255;

            // Each pixel is offset in both hue and brightness
            hue2 += 40;
            sine2 += 0.3;
        }

        // Slowly rotate hue and brightness in opposite directions
        hue1 = (hue1 + 5) % 1536;
        sine1 -= .03;

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
            for(unsigned int i=3; i<bufLen; i++) {
                if(i%3 == 0) printf("LED %d:\t", i/3 - 1);
                printf("%d\t|\t", buffer[i]);
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
	printf("%s: Version %s\n", prog, VERSION_STRING);
}