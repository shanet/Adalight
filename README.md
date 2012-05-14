This is a fork of the Colorswirl demo for the Adalight project which has been extended to support many more options than the original Colorswirl program. It was created due to the screen sampling program described in the link below is too slow to use on Linux. Thus, it due to the use of GNU functions (namely getopt), it will only compile and run under Linux.

To use it you'll need to follow the instructions at http://ladyada.net/make/adalight/.

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