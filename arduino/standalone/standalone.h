#define LED_PIN  6
#define NUM_LEDS 50

// Color options
#define MULTI  0
#define RED    1
#define ORANGE 2
#define YELLOW 3
#define GREEN  4
#define BLUE   5
#define PURPLE 6
#define WHITE  7
#define COOL   8

// Rotation speed options
#define ROT_NONE      0
#define ROT_VERY_SLOW 1
#define ROT_SLOW      2
#define ROT_NORMAL    3
#define ROT_FAST      4
#define ROT_VERY_FAST 5

// Fade speed options
#define FADE_NONE      0
#define FADE_VERY_SLOW 1
#define FADE_SLOW      2
#define FADE_NORMAL    3
#define FADE_FAST      4
#define FADE_VERY_FAST 5

// Rotation direction options
#define ROT_CW  0
#define ROT_CCW 1

// Shadow length options
#define SDW_NONE       0
#define SDW_VERY_SMALL 1
#define SDW_SMALL      2
#define SDW_NORMAL     3
#define SDW_LONG       4
#define SDW_VERY_LONG  5

// Default pattern options
int color = MULTI;
int rotationSpeed = ROT_NONE;
int rotationDir = ROT_CW;
int shadowLength = SDW_NONE;
int fadeSpeed = FADE_VERY_SLOW;


void getCalculatedLedData(unsigned int ledDataLen);
void getLedColor(unsigned char *r, unsigned char *g, unsigned char *b, int curHue);
void updateLightPosition(double *lightPosition);
void updateShadowPosition(double *shadowPosition);
void updateHue(int *curHue);
