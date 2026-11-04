#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>

// ESP32-C3 SuperMini + NV3007 142x428 SPI module
// Logical layout is 428x142 landscape.

static constexpr int PIN_SCK  = 5;
static constexpr int PIN_MOSI = 4;
static constexpr int PIN_CS   = 7;
static constexpr int PIN_DC   = 6;
static constexpr int PIN_RST  = 3;
static constexpr int PIN_BL   = 10;

static constexpr uint16_t PANEL_W = 142;
static constexpr uint16_t PANEL_H = 428;
static constexpr uint16_t X_OFFSET = 12;
static constexpr int16_t W = 428;
static constexpr int16_t H = 142;

static constexpr uint16_t BLACK = 0x0000;
static constexpr uint16_t WHITE = 0xFFFF;
static constexpr uint16_t BLUE = 0x05FF;
static constexpr uint16_t YELLOW = 0xFFE0;
static constexpr uint16_t ORANGE = 0xFD20;
static constexpr uint16_t GREY = 0x7BEF;

SPISettings lcdSPI(20000000, MSBFIRST, SPI_MODE0);
static uint16_t fb[W * H];

class Canvas : public Adafruit_GFX {
public:
  Canvas() : Adafruit_GFX(W, H) {}
  void drawPixel(int16_t x, int16_t y, uint16_t c) override {
    if (x >= 0 && y >= 0 && x < W && y < H) fb[(size_t)y * W + x] = c;
  }
};
Canvas gfx;

static void cmd(uint8_t c, const uint8_t *d = nullptr, size_t n = 0) {
  SPI.beginTransaction(lcdSPI);
  digitalWrite(PIN_CS, LOW);
  digitalWrite(PIN_DC, LOW);
  SPI.transfer(c);
  if (d && n) {
    digitalWrite(PIN_DC, HIGH);
    while (n--) SPI.transfer(*d++);
  }
  digitalWrite(PIN_CS, HIGH);
  SPI.endTransaction();
}

struct InitCmd { uint8_t cmd, len, data[4]; };

// Same NV3007 init sequence as the previously working color-test sketch.
static const InitCmd nv3007InitSequence[] = {
  {0xFF,1,{0xA5}}, {0x9A,1,{0x08}}, {0x9B,1,{0x08}}, {0x9C,1,{0xB0}},
  {0x9D,1,{0x16}}, {0x9E,1,{0xC4}}, {0x8F,2,{0x55,0x04}}, {0x84,1,{0x90}},
  {0x83,1,{0x7B}}, {0x85,1,{0x33}},
  {0x60,1,{0x00}}, {0x70,1,{0x00}}, {0x61,1,{0x02}}, {0x71,1,{0x02}},
  {0x62,1,{0x04}}, {0x72,1,{0x04}}, {0x6C,1,{0x29}}, {0x7C,1,{0x29}},
  {0x6D,1,{0x31}}, {0x7D,1,{0x31}}, {0x6E,1,{0x0F}}, {0x7E,1,{0x0F}},
  {0x66,1,{0x21}}, {0x76,1,{0x21}}, {0x68,1,{0x3A}}, {0x78,1,{0x3A}},
  {0x63,1,{0x07}}, {0x73,1,{0x07}}, {0x64,1,{0x05}}, {0x74,1,{0x05}},
  {0x65,1,{0x02}}, {0x75,1,{0x02}}, {0x67,1,{0x23}}, {0x77,1,{0x23}},
  {0x69,1,{0x08}}, {0x79,1,{0x08}}, {0x6A,1,{0x13}}, {0x7A,1,{0x13}},
  {0x6B,1,{0x13}}, {0x7B,1,{0x13}}, {0x6F,1,{0x00}}, {0x7F,1,{0x00}},
  {0x50,1,{0x00}}, {0x52,1,{0xD6}}, {0x53,1,{0x08}}, {0x54,1,{0x08}},
  {0x55,1,{0x1E}}, {0x56,1,{0x1C}},
  {0xA0,3,{0x2B,0x24,0x00}}, {0xA1,1,{0x87}}, {0xA2,1,{0x86}},
  {0xA5,1,{0x00}}, {0xA6,1,{0x00}}, {0xA7,1,{0x00}}, {0xA8,1,{0x36}},
  {0xA9,1,{0x7E}}, {0xAA,1,{0x7E}},
  {0xB9,1,{0x85}}, {0xBA,1,{0x84}}, {0xBB,1,{0x83}}, {0xBC,1,{0x82}},
  {0xBD,1,{0x81}}, {0xBE,1,{0x80}}, {0xBF,1,{0x01}}, {0xC0,1,{0x02}},
  {0xC1,1,{0x00}}, {0xC2,1,{0x00}}, {0xC3,1,{0x00}}, {0xC4,1,{0x33}},
  {0xC5,1,{0x7E}}, {0xC6,1,{0x7E}}, {0xC8,2,{0x33,0x33}}, {0xC9,1,{0x68}},
  {0xCA,1,{0x69}}, {0xCB,1,{0x6A}}, {0xCC,1,{0x6B}}, {0xCD,2,{0x33,0x33}},
  {0xCE,1,{0x6C}}, {0xCF,1,{0x6D}}, {0xD0,1,{0x6E}}, {0xD1,1,{0x6F}},
  {0xAB,2,{0x03,0x67}}, {0xAC,2,{0x03,0x6B}}, {0xAD,2,{0x03,0x68}},
  {0xAE,2,{0x03,0x6C}},
  {0xB3,1,{0x00}}, {0xB4,1,{0x00}}, {0xB5,1,{0x00}}, {0xB6,1,{0x32}},
  {0xB7,1,{0x7E}}, {0xB8,1,{0x7E}},
  {0xE0,1,{0x00}}, {0xE1,2,{0x03,0x0F}}, {0xE2,1,{0x04}}, {0xE3,1,{0x01}},
  {0xE4,1,{0x0E}}, {0xE5,1,{0x01}}, {0xE6,1,{0x19}}, {0xE7,1,{0x10}},
  {0xE8,1,{0x10}}, {0xEA,1,{0x12}}, {0xEB,1,{0xD0}}, {0xEC,1,{0x04}},
  {0xED,1,{0x07}}, {0xEE,1,{0x07}}, {0xEF,1,{0x09}}, {0xF0,1,{0xD0}},
  {0xF1,1,{0x0E}}, {0xF9,1,{0x17}}, {0xF2,4,{0x2C,0x1B,0x0B,0x20}},
  {0xE9,1,{0x29}}, {0xEC,1,{0x04}}, {0x35,1,{0x00}},
  {0x44,2,{0x00,0x10}}, {0x46,1,{0x10}},
  {0xFF,1,{0x00}}, {0x3A,1,{0x05}}, {0x00,0,{0x00}}
};

static void lcdInit() {
  digitalWrite(PIN_RST, HIGH); delay(100);
  digitalWrite(PIN_RST, LOW);  delay(120);
  digitalWrite(PIN_RST, HIGH); delay(120);

  for (const auto &e : nv3007InitSequence) {
    if (!e.len) break;
    cmd(e.cmd, e.data, e.len);
  }
  cmd(0x11); delay(200); // Sleep out
  cmd(0x29); delay(150); // Display on
}

static void setWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  x1 += X_OFFSET; x2 += X_OFFSET;
  uint8_t d[4] = {(uint8_t)(x1 >> 8), (uint8_t)x1,
                  (uint8_t)(x2 >> 8), (uint8_t)x2};
  cmd(0x2A, d, 4);
  d[0] = y1 >> 8; d[1] = y1; d[2] = y2 >> 8; d[3] = y2;
  cmd(0x2B, d, 4);
}

// Rotate logical 428x142 landscape framebuffer to physical 142x428 panel.
static void lcdFlush() {
  setWindow(0, 0, PANEL_W - 1, PANEL_H - 1);
  uint8_t line[PANEL_W * 2];

  SPI.beginTransaction(lcdSPI);
  digitalWrite(PIN_CS, LOW);
  digitalWrite(PIN_DC, LOW); SPI.transfer(0x2C);
  digitalWrite(PIN_DC, HIGH);

  for (int py = 0; py < PANEL_H; ++py) {
    int logicalX = W - 1 - py;
    for (int px = 0; px < PANEL_W; ++px) {
      uint16_t c = fb[(size_t)px * W + logicalX];
      line[px * 2] = c >> 8;
      line[px * 2 + 1] = c & 0xFF;
    }
    SPI.transferBytes(line, nullptr, sizeof(line));
  }
  digitalWrite(PIN_CS, HIGH);
  SPI.endTransaction();
}

// ---------------------- NV3007 climate dashboard ----------------------
// Reference composition, 428x142 landscape.
// The large asymmetric outline is the boundary of the house: outside left,
// indoor right. The small house+thermometer is a separate indoor icon.
// No OUTDOOR/INDOOR text labels.

static const uint8_t SEGMENTS[10] = {
  0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F
};

static constexpr uint16_t CYAN   = 0x4E9F;
static constexpr uint16_t ICE    = 0xDFFF;
static constexpr uint16_t PALE   = 0xBFFF;
static constexpr uint16_t MUTED  = 0x4208;
static constexpr uint16_t RED    = 0xF800;
static constexpr uint16_t CLOUD  = 0xC618;

static void segH(int x,int y,int w,uint16_t c) {
  gfx.fillRoundRect(x,y,w,4,2,c);
}
static void segV(int x,int y,int h,uint16_t c) {
  gfx.fillRoundRect(x,y,4,h,2,c);
}

// 18x36 seven-segment glyph, with generous spacing and consistent geometry.
static void drawDigit(int x,int y,uint8_t n,uint16_t c) {
  const uint8_t s=SEGMENTS[n];
  if(s&0x01) segH(x+4,y,11,c);
  if(s&0x02) segV(x+15,y+4,13,c);
  if(s&0x04) segV(x+15,y+20,13,c);
  if(s&0x08) segH(x+4,y+33,11,c);
  if(s&0x10) segV(x,y+20,13,c);
  if(s&0x20) segV(x,y+4,13,c);
  if(s&0x40) segH(x+4,y+17,11,c);
}
static int drawNumber(int x,int y,const char *s,uint16_t c) {
  for(;*s;++s) {
    if(*s>='0'&&*s<='9') { drawDigit(x,y,(uint8_t)(*s-'0'),c); x+=21; }
    else if(*s=='-') { gfx.fillRoundRect(x+2,y+17,12,4,2,c); x+=16; }
    else if(*s=='+') {
      gfx.fillRoundRect(x+2,y+17,12,4,2,c);
      gfx.fillRoundRect(x+6,y+13,4,12,2,c); x+=16;
    } else if(*s=='.') { gfx.fillCircle(x+2,y+36,2,c); x+=7; }
  }
  return x;
}
static void drawSmallText(int x,int baseline,const char *s,uint16_t c) {
  gfx.setFont(&FreeSansBold9pt7b);
  gfx.setTextColor(c);
  gfx.setCursor(x,baseline);
  gfx.print(s);
  gfx.setFont();
}
static void drawDegreeC(int x,int y,uint16_t c) {
  gfx.drawCircle(x+3,y+5,3,c);
  drawSmallText(x+11,y+13,"C",c);
}

// OUTSIDE icon: filled cloud with a clean scalloped silhouette and rain.
static void drawCloudRain(int cx,int cy) {
  gfx.fillCircle(cx-10,cy+1,9,CLOUD);
  gfx.fillCircle(cx+1,cy-6,12,ICE);
  gfx.fillCircle(cx+13,cy+1,9,CLOUD);
  gfx.fillRoundRect(cx-17,cy-1,36,13,6,CLOUD);
  gfx.fillCircle(cx-3,cy-8,5,WHITE);
  gfx.drawLine(cx-10,cy+16,cx-14,cy+23,CYAN);
  gfx.drawLine(cx,cy+16,cx-4,cy+23,CYAN);
  gfx.drawLine(cx+10,cy+16,cx+6,cy+23,CYAN);
}

// Filled water drop with a dark inner crescent/highlight.
static void drawDrop(int cx,int cy,uint16_t color) {
  gfx.fillTriangle(cx,cy-15,cx-11,cy+2,cx+11,cy+2,color);
  gfx.fillCircle(cx,cy+3,11,color);
  gfx.fillCircle(cx-1,cy+2,6,BLACK);
  gfx.drawLine(cx-5,cy+5,cx-2,cy+10,ICE);
}

// Barometer: proper circular instrument, tick marks and a needle.
static void drawBarometer(int cx,int cy) {
  gfx.fillCircle(cx,cy,16,ORANGE);
  gfx.fillCircle(cx,cy,12,0x2104);
  gfx.drawCircle(cx,cy,10,ICE);
  gfx.drawLine(cx,cy-8,cx,cy-5,ICE);
  gfx.drawLine(cx-8,cy,cx-5,cy,ICE);
  gfx.drawLine(cx+8,cy,cx+5,cy,ICE);
  gfx.drawLine(cx,cy,cx+6,cy-6,YELLOW);
  gfx.fillCircle(cx,cy,2,WHITE);
}

// Small indoor house icon with thermometer INSIDE it.
static void drawIndoorHouseIcon(int cx,int cy) {
  // Roof and walls
  gfx.drawLine(cx-17,cy-1,cx,cy-18,YELLOW);
  gfx.drawLine(cx,cy-18,cx+17,cy-1,YELLOW);
  gfx.drawLine(cx-13,cy-3,cx-13,cy+19,YELLOW);
  gfx.drawLine(cx+13,cy-3,cx+13,cy+19,YELLOW);
  gfx.drawFastHLine(cx-13,cy+19,26,YELLOW);
  // Thermometer, centred inside
  gfx.drawRoundRect(cx-3,cy+1,6,13,3,ICE);
  gfx.drawFastVLine(cx,cy+4,8,RED);
  gfx.drawCircle(cx,cy+15,4,ICE);
  gfx.fillCircle(cx,cy+15,2,RED);
}

// Thermometer pictogram for the indoor temperature row.
static void drawThermometer(int cx,int cy) {
  gfx.drawRoundRect(cx-5,cy-16,10,25,5,YELLOW);
  gfx.fillRoundRect(cx-2,cy-10,4,18,2,RED);
  gfx.drawCircle(cx,cy+11,7,YELLOW);
  gfx.fillCircle(cx,cy+11,4,RED);
}

// This is the large house boundary from the reference, NOT a centred icon.
// The long roof runs from the left upper field to a peak, slopes down to
// the right-side wall; the wall continues to the bottom sill.
static void drawHouseBoundary() {
  const uint16_t c=CYAN;
  // roof: outdoor side rises to ridge, then long descending indoor-side slope
  gfx.drawLine(68,39,151,8,c);
  gfx.drawLine(151,8,235,60,c);
  // right wall and bottom boundary
  gfx.drawFastVLine(235,60,72,c);
  gfx.drawFastHLine(235,132,185,c);
}

static void drawDashboard() {
  gfx.fillScreen(BLACK);

  // House boundary first, so foreground readings/icons stay crisp.
  drawHouseBoundary();

  // LEFT: outdoor temperature
  drawCloudRain(34,35);
  int x=drawNumber(83,24,"-5.2",ICE);
  drawDegreeC(x+3,24,ICE);

  // Left row separators, confined to the outdoor data column.
  gfx.drawFastHLine(12,68,184,MUTED);
  gfx.drawFastHLine(12,108,184,MUTED);

  // LEFT: humidity
  drawDrop(34,86,CYAN);
  x=drawNumber(83,72,"64",ICE);
  drawSmallText(x+7,99,"% ",CYAN);

  // LEFT: pressure
  drawBarometer(34,123);
  x=drawNumber(83,108,"1013",ICE);
  drawSmallText(x+4,135,"hPa",ORANGE);

  // RIGHT: indoor temperature, with a small house/thermometer pictogram.
  drawIndoorHouseIcon(266,47);
  x=drawNumber(300,34,"22.4",ICE);
  drawDegreeC(x+2,34,ICE);

  gfx.drawFastHLine(260,83,157,MUTED);

  // RIGHT: indoor humidity
  drawDrop(277,111,YELLOW);
  x=drawNumber(310,96,"45",ICE);
  drawSmallText(x+7,123,"% ",YELLOW);

  lcdFlush();
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_DC, OUTPUT);
  pinMode(PIN_RST, OUTPUT);
  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_CS, HIGH);
  digitalWrite(PIN_DC, HIGH);
  digitalWrite(PIN_BL, HIGH);

  SPI.begin(PIN_SCK, -1, PIN_MOSI, PIN_CS);
  lcdInit();

  Serial.println("NV3007 weather dashboard");
  drawDashboard();
  Serial.println("Dashboard rendered");
}

void loop() {
  delay(1000);
}
