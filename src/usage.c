/*
 *
 * Colorswirl
 *
 * Author: Shane Tully
 *
 * Source:      https://github.com/shanet/Adalight
 * Forked from: https://github.com/adafruit/Adalight
 *
 */

#include "usage.h"

void printUsage(char *prog) {
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
    printf("\t\tSimply shows the selected color at full brightness. Takes an optional fade speed for fading between colors if multi color is selected.\n\n");
    printf("\t\tSupported fade speeds:\n\t\t  vs\tvery_slow\n\t\t  s\tslow\n\t\t  \tnormal (default)\n\t\t  f\tfast\n\t\t  vf\tvery_fast\n\n");
    
    printf("\t--verbose\t-v\t\tIncrease verbosity. Can be specified multiple times.\n");
    printf("\t\tSingle verbose will show \"frame rate\" and bytes/sec. Double verbose is \n\t\tshows message queue info. Triple verbose will show all info\n\t\t\
being sent to the device. This is useful for visualizing how the options\n\t\tabove affect what data is sent to the device.\n\n");

    printf("\t--version\t-V\t\tDisplay version and exit\n");
    printf("\t--help\t\t-h\t\tDisplay this message and exit\n\n");
    
    printf("\tDevice is the path to the block device to write data to. If not specified,\n\tdefaults to \"%s\"\n\n", DEFAULT_DEVICE);

    printf("\tOptions are parsed from left to right. For example, specifying --solid and then\n\t--shadow will NOT result in a solid color.\
            \n\n\tIf all this seems confusing, just play with the options and try triple verbose.\n");
}


void printVersion(char *prog) {
    printf("%s: Version %s\n", prog, VERSION);
}