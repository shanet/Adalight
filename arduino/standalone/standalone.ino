#include "standalone.h"
#include <FastLED.h>

CRGB leds[NUM_LEDS];

void setup() {
  FastLED.addLeds<WS2811, LED_PIN, RGB>(leds, NUM_LEDS);
}

void loop() {
    getCalculatedLedData(NUM_LEDS);
    FastLED.show();
}

void getCalculatedLedData(unsigned int numLeds) {
    static int brightness = 0;
    static double shadowPosition = 0;
    static double lightPosition = 0;
    static int hue = 0;

    static unsigned char red;
    static unsigned char green;
    static unsigned char blue;

    shadowPosition = lightPosition;

    for(unsigned int i=0; i<numLeds; i++) {
        getLedColor(&red, &green, &blue, hue);

        // Resulting hue is multiplied by brightness in the
        // range of 0 to 255 (0 = off, 255 = brightest).
        // Gamma corrrection (the 'pow' function here) adjusts
        // the brightness to be more perceptually linear.
        brightness = (shadowLength != SDW_NONE || rotationSpeed != ROT_NONE) ? (int)(pow(0.5 + sin(shadowPosition) * 0.5, 3.0) * 255.0) : 255;
        leds[i] = CRGB(red * brightness /255, green * brightness /255, blue * brightness /255);

        // Each pixel is offset in both hue and brightness
        updateShadowPosition(&shadowPosition);
    }

    // If color is multi and fade flag was selected, do a slow fade between colors with the rotation speed
    if(fadeSpeed != FADE_NONE && color == MULTI) {
        switch(fadeSpeed) {
            case FADE_VERY_SLOW:
                delay(300);
                break;
            case FADE_SLOW:
                delayMicroseconds(1000*250);
                break;
            default:
            case FADE_NORMAL:
                delayMicroseconds(1000*150);
                break;
            case FADE_FAST:
                delayMicroseconds(1000*100);
                break;
            case FADE_VERY_FAST:
                delayMicroseconds(1000*50);
                break;
        }
    }

    // Slowly rotate hue and brightness in opposite directions
    updateHue(&hue);
    updateLightPosition(&lightPosition);
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
