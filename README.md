Colorswirl
==========

#### shane tully (shanetully.com)
#### Forked from https://github.com/adafruit/Adalight

This is a fork of the Colorswirl demo for the Adalight project which has been extended to support many more options than the original Colorswirl program. It was created due to the screen sampling program in the original repo being too slow to use on Linux (blame Java, not the author of the program).

There are two flavors included:

* Coupled: This version uses a host computer to generate the LEDs values and are constantly sent to the Arduino for display. Color patterns can be updated dynamically through the `colorswirl_update` program.
* Standalone: This version generates a static pattern without the need for a connected host computer. Changes to the pattern require recompiling and uploading the program to the Arduino.

## Prerequisites

To use it you'll need to follow the instructions at http://learn.adafruit.com/adalight-diy-ambient-tv-lighting to get your Arduino wiring set up.

## Compiling

### Coupled

* Arduino: Compile and upload `arduino/coupled/coupled.ino` like any other sketch.
* Desktop: `cd desktop; make; make install`

Also included is a systemd script which is installed as part of the `make install` step.

### Standalone

0. Add the "Fast LED" library through the Arduino IDE (Sketch > Include Library)
0. Compile and upload `arduino/standalone/standalone.ino` like any other sketch.

Note: It may be necessary to modify `arduino/standalone/standalone.h` to set the pin number and number of LEDs are appropriate. By default, these are pin 6 and 50 LEDs. The selected pattern can also be modified here.

## Usage

Usage for the coupled flavor is well documented in the help text available via the `--help` option, but is also available below.

The `colorswirl_update` program allows for dynamic changing of LED behavior while colorswirl is running. To use it simply run it with the same arguments as colorswirl and those arguments will be passed to colorswirl and updated appropriately. While you can specify a different device to send LED info to this way, it will have no effect. This is useful if you want colorswirl to start when you log in to your desktop and bind shortcuts to different effects without restarting the program.

```
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

    Device is the path to the block device to write data to. If not specified, it defaults to /dev/ttyACM0

    Options are parsed from left to right. For example, specifying --solid and then --shadow will NOT result in a solid color.

    If all this seems confusing, just play with the options and try triple verbose.
```

## License

Adalight is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Adalight is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Adalight.  If not, see <http://www.gnu.org/licenses/>.
