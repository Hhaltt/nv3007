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

// ---------------------- Reference-style climate dashboard ----------------------
// 428x142 landscape. Cyan technical display, 7-segment numerals,
// outdoor weather on the left, indoor climate on the right.

static const uint8_t SEGMENTS[10] = {
  0b00111111, // 0
  0b00000110, // 1
  0b01011011, // 2
  0b01001111, // 3
  0b01100110, // 4
  0b01101101, // 5
  0b01111101, // 6
  0b00000111, // 7
  0b01111111, // 8
  0b01101111  // 9
};

static void thickLine(int x1, int y1, int x2, int y2, uint16_t color, int t=3) {
  for (int i = -(t/2); i <= t/2; ++i) {
    if (abs(x2-x1) > abs(y2-y1)) gfx.drawLine(x1, y1+i, x2, y2+i, color);
    else gfx.drawLine(x1+i, y1, x2+i, y2, color);
  }
}

static void draw7Digit(int x, int y, int digit, uint16_t color, int scale=1) {
  const int w=15*scale, h=27*scale, t=3*scale;
  const int mx=x+w/2, my=y+h/2;
  const uint8_t s=SEGMENTS[digit];
  if (s & 0x01) thickLine(x+3*scale,y,mx-2*scale,y,color,t);
  if (s & 0x02) thickLine(x+w,y+3*scale,x+w,my-2*scale,color,t);
  if (s & 0x04) thickLine(x+w,my+2*scale,x+w,y+h-3*scale,color,t);
  if (s & 0x08) thickLine(x+3*scale,y+h,mx-2*scale,y+h,color,t);
  if (s & 0x10) thickLine(x,y+my+2*scale,x,y+h-3*scale,color,t);
  if (s & 0x20) thickLine(x,y+3*scale,x,my-2*scale,color,t);
  if (s & 0x40) thickLine(x+3*scale,my,mx-2*scale,my,color,t);
}

static int draw7Text(int x, int y, const char *str, uint16_t color, int scale=1) {
  const int advance=21*scale;
  for (const char *p=str; *p; ++p) {
    if (*p >= '0' && *p <= '9') {
      draw7Digit(x,y,*p-'0',color,scale);
      x += advance;
    } else if (*p == '-') {
      thickLine(x+2*scale,y+14*scale,x+12*scale,y+14*scale,color,3*scale);
      x += advance;
    } else if (*p == '+') {
      thickLine(x+2*scale,y+14*scale,x+12*scale,y+14*scale,color,3*scale);
      thickLine(x+7*scale,y+9*scale,x+7*scale,y+19*scale,color,3*scale);
      x += advance;
    } else if (*p == '.') {
      gfx.fillCircle(x+2*scale,y+27*scale,2*scale,color);
      x += 7*scale;
    }
  }
  return x;
}

static void drawDegreeC(int x, int baseline, uint16_t color) {
  gfx.drawCircle(x+3, baseline-24, 3, color);
  gfx.setFont(&FreeSansBold9pt7b);
  gfx.setTextColor(color);
  gfx.setCursor(x+10, baseline-8);
  gfx.print("C");
  gfx.setFont();
}

static void drawDropIcon(int cx, int cy, uint16_t color) {
  gfx.drawLine(cx,cy-15,cx-10,cy+2,color);
  gfx.drawLine(cx,cy-15,cx+10,cy+2,color);
  gfx.drawLine(cx-10,cy+2,cx-9,cy+10,color);
  gfx.drawLine(cx+10,cy+2,cx+9,cy+10,color);
  gfx.drawCircle(cx,cy+5,9,color);
}

static void drawThermoIcon(int cx, int cy, uint16_t color) {
  gfx.drawRoundRect(cx-5,cy-16,10,27,5,color);
  gfx.drawFastVLine(cx,cy-9,23,color);
  gfx.drawCircle(cx,cy+14,7,color);
  gfx.fillCircle(cx,cy+14,3,color);
}

static void drawPressureIcon(int cx, int cy, uint16_t color) {
  gfx.drawCircle(cx,cy,16,color);
  gfx.drawLine(cx,cy-11,cx,cy-7,color);
  gfx.drawLine(cx-11,cy,cx-7,cy,color);
  gfx.drawLine(cx+11,cy,cx+7,cy,color);
  gfx.drawLine(cx,cy,cx+7,cy-8,color);
  gfx.fillCircle(cx,cy,2,color);
}

static void drawRainCloud(int cx, int cy, uint16_t color) {
  gfx.drawRoundRect(cx-20,cy-1,40,19,9,color);
  gfx.drawCircle(cx-9,cy-4,10,color);
  gfx.drawCircle(cx+5,cy-9,13,color);
  gfx.drawCircle(cx+17,cy-2,9,color);
  // mask lower inner arcs for a clean outlined cloud
  gfx.drawFastHLine(cx-18,cy+1,36,BLACK);
  gfx.drawLine(cx-12,cy+23,cx-16,cy+29,color);
  gfx.drawLine(cx,cy+23,cx-4,cy+29,color);
  gfx.drawLine(cx+12,cy+23,cx+8,cy+29,color);
}

static void drawHouseThermometer(int cx, int cy, uint16_t color) {
  // Mirrored house icon: roof slopes are reversed relative to the first draft.
  gfx.drawLine(cx-18,cy-2,cx+1,cy-20,color);
  gfx.drawLine(cx+1,cy-20,cx+20,cy-2,color);
  gfx.drawRect(cx-14,cy-2,30,29,color);
  gfx.drawRoundRect(cx-2,cy+4,7,16,3,color);
  gfx.drawFastVLine(cx+1,cy+7,12,color);
  gfx.drawCircle(cx+1,cy+21,4,color);
}

static void drawRoofDivider() {
  // Automotive cabin outline: broad roof rises from the outdoor side,
  // slopes down to the center partition; mirrored house profile.
  gfx.drawLine(8,43,150,7,BLUE);
  gfx.drawLine(150,7,238,43,BLUE);
  gfx.drawFastHLine(226,43,194,BLUE);
  gfx.drawFastVLine(226,43,91,BLUE);
  gfx.drawFastHLine(226,134,194,BLUE);
}

static void drawDashboard() {
  gfx.fillScreen(BLACK);

  // Section captions
  gfx.setFont(&FreeSansBold9pt7b);
  gfx.setTextColor(BLUE);
  gfx.setCursor(13,16); gfx.print("OUTDOOR");
  gfx.setTextColor(BLUE);
  gfx.setCursor(337,16); gfx.print("INDOOR");
  gfx.setFont();

  drawRoofDivider();

  // Outdoor: icon column and large 7-segment values.
  drawRainCloud(39,39,BLUE);
  draw7Text(78,27,"-5.2",BLUE,1);
  drawDegreeC(164,57,BLUE);

  gfx.drawFastHLine(12,70,190,GREY);

  drawDropIcon(39,91,BLUE);
  draw7Text(78,78,"78",BLUE,1);
  gfx.setFont(&FreeSansBold9pt7b);
  gfx.setTextColor(BLUE); gfx.setCursor(124,101); gfx.print("%");
  gfx.setFont();

  gfx.drawFastHLine(12,109,190,GREY);

  drawPressureIcon(39,124,BLUE);
  draw7Text(78,111,"735",BLUE,1);
  gfx.setFont(&FreeSansBold9pt7b);
  gfx.setTextColor(BLUE); gfx.setCursor(145,132); gfx.print("hPa");
  gfx.setFont();

  // Indoor: house icon + temperature, then humidity.
  drawHouseThermometer(255,47,YELLOW);
  draw7Text(287,35,"22.4",BLUE,1);
  drawDegreeC(373,65,BLUE);

  gfx.drawFastHLine(248,83,169,GREY);

  drawDropIcon(267,108,BLUE);
  draw7Text(300,94,"45",BLUE,1);
  gfx.setFont(&FreeSansBold9pt7b);
  gfx.setTextColor(BLUE); gfx.setCursor(346,117); gfx.print("%");
  gfx.setFont();

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
