# Colorswirl
## Author: Shane Tully
### Forked from: https://github.com/adafruit/Adalight

This is a fork of the Colorswirl demo for the Adalight project which has been extended to support many more options than the original Colorswirl program. It was created due to the screen sampling program in the original repo being too slow to use on Linux (blame Java, not the author of the program).

## Prerequisties

To use it you'll need to follow the instructions at http://ladyada.net/make/adalight/ to get your Arduino set up and then compile and run the colorswirl program.

## Compiling

Just run the usual "make" and "make install".
This program was written and tested on a Debian-based version of Linux. Your mileage may vary when compiling on other flavors of Linux. An effort to adhear as close as possible to standards was made so any problems encountered should be easily fixed.
Due to the use of GNU functions, namely getopt, this will not compile on Windows. Plans to support Windows are nonexistant.

## Usage

Usage is very well documented in the help text available via the --help option, but also available below.

The colorswirl_update program allows for dynamic changing of LED behavior while colorswirl is running. To use it simply run it with the same arguments as colorswirl and those arguments will be passed to colorswirl and updated appropriately. While you can specifiy a different device to send LED info to this way, it will have no effect. This is useful if you want colorswirl to start when you log in to your desktop and bind shortcuts to different effects without restarting the program.


    Usage: colorswirl [options] [device]

        --color       -c    Color to use   
            Supported colors:
               multi (default)
               red
               orange
               yellow
               green
               blue
               purple
               white

        --rotation    -r    Rotation speed   
            Supported rotation speeds:
               n    none
               vs   very_slow
               s    slow
                    normal (default)
               f    fast
               vf   very_fast

        --direction   -d    Rotation direction   
            Supported directions:
               cw    Clockwise (default)
               ccw   Counter-clockwise

        --shadow      -s    Shadow length   
            Supported shadow lengths:
               n    none
               vs   very_small
               s    small
                    normal (default)
               l    long
               vl   very_long

        --fade        -f    Fade speed (shorthand for no rotation, slow shadow)   
            This option takes an optional argument. Arguments must be specified via the form "--fade=[speed]" or "-f[speed]"
            Supported fade speeds:
               vs   very_slow
               s    slow (default)
               n    normal
               f    fast
               vf   very_fast   

        --solid       -o    Shorthand for no rotation, no shadow   
            Takes no arguments. Simply shows the selected color at full brightness   

        --verbose     -v    Increase verbosity. Can be specified multiple times.   
            Single verbose will show "frame rate" and bytes/sec. Double verbose is currently the same as single verbose. Triple verbose will show all info being sent to the device. This is useful for visualizing how the options above affect what data is sent to the device.   

        --version    -V     Display version and exit   
        --help       -h     Display this message and exit   

        Device is the path to the block device to write data to. If not specified, defaults to /dev/ttyACM0

        Options are parsed from left to right. For example, specifying --solid and then --shadow will NOT result in a solid color.
        
        If all this seems confusing, just play with the options and try triple verbose.   