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

// ---------------------- Clean reference-style climate dashboard ----------------------
// All coordinates are for the 428x142 logical landscape canvas.
// Keep the roof graphic in the central gap: it must never cross text or icons.

static const uint8_t SEGMENTS[10] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66,
  0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

// A legible custom 7-segment digit, 22x38 px.
static void segH(int x, int y, int len, uint16_t color) {
  gfx.fillRoundRect(x, y, len, 4, 2, color);
}
static void segV(int x, int y, int len, uint16_t color) {
  gfx.fillRoundRect(x, y, 4, len, 2, color);
}
static void drawDigit(int x, int y, uint8_t n, uint16_t color) {
  const uint8_t s = SEGMENTS[n];
  if (s & 0x01) segH(x+4,  y,    14, color); // a
  if (s & 0x02) segV(x+18, y+4,  14, color); // b
  if (s & 0x04) segV(x+18, y+21, 14, color); // c
  if (s & 0x08) segH(x+4,  y+36, 14, color); // d
  if (s & 0x10) segV(x,    y+21, 14, color); // e
  if (s & 0x20) segV(x,    y+4,  14, color); // f
  if (s & 0x40) segH(x+4,  y+18, 14, color); // g
}

static int drawSegmentNumber(int x, int y, const char *s, uint16_t color) {
  for (; *s; ++s) {
    if (*s >= '0' && *s <= '9') {
      drawDigit(x, y, (uint8_t)(*s-'0'), color);
      x += 28;
    } else if (*s == '-') {
      gfx.fillRoundRect(x+2, y+18, 14, 4, 2, color);
      x += 20;
    } else if (*s == '+') {
      gfx.fillRoundRect(x+2, y+18, 14, 4, 2, color);
      gfx.fillRoundRect(x+7, y+13, 4, 14, 2, color);
      x += 20;
    } else if (*s == '.') {
      gfx.fillCircle(x+2, y+39, 2, color);
      x += 8;
    }
  }
  return x;
}

static void drawUnitC(int x, int y, uint16_t color) {
  gfx.drawCircle(x+3, y+5, 3, color);
  gfx.setFont(&FreeSansBold9pt7b);
  gfx.setTextColor(color);
  gfx.setCursor(x+10, y+11);
  gfx.print("C");
  gfx.setFont();
}

static void drawDropIcon(int cx, int cy, uint16_t color) {
  // Clean outline drop, symmetric and centered on cx/cy.
  gfx.drawLine(cx,cy-14,cx-10,cy+2,color);
  gfx.drawLine(cx,cy-14,cx+10,cy+2,color);
  gfx.drawLine(cx-10,cy+2,cx-9,cy+9,color);
  gfx.drawLine(cx+10,cy+2,cx+9,cy+9,color);
  gfx.drawCircle(cx,cy+4,9,color);
  gfx.drawFastHLine(cx-5,cy+13,10,color);
}

static void drawThermoIcon(int cx, int cy, uint16_t color) {
  gfx.drawRoundRect(cx-5,cy-15,10,25,5,color);
  gfx.drawFastVLine(cx,cy-8,20,color);
  gfx.drawCircle(cx,cy+13,7,color);
  gfx.fillCircle(cx,cy+13,3,color);
}

static void drawPressureIcon(int cx, int cy, uint16_t color) {
  gfx.drawCircle(cx,cy,15,color);
  gfx.drawLine(cx,cy-10,cx,cy-6,color);
  gfx.drawLine(cx-10,cy,cx-6,cy,color);
  gfx.drawLine(cx+10,cy,cx+6,cy,color);
  gfx.drawLine(cx,cy,cx+6,cy-7,color);
  gfx.fillCircle(cx,cy,2,color);
}

static void drawRainCloud(int cx, int cy, uint16_t color) {
  // Simple cloud silhouette with three separate rain strokes.
  gfx.drawRoundRect(cx-18,cy-4,36,19,9,color);
  gfx.drawCircle(cx-8,cy-5,9,color);
  gfx.drawCircle(cx+5,cy-10,12,color);
  gfx.drawCircle(cx+16,cy-3,8,color);
  gfx.drawFastHLine(cx-15,cy+4,30,BLACK);
  gfx.drawLine(cx-10,cy+19,cx-14,cy+25,color);
  gfx.drawLine(cx,cy+19,cx-4,cy+25,color);
  gfx.drawLine(cx+10,cy+19,cx+6,cy+25,color);
}

static void drawHouseThermometer(int cx, int cy, uint16_t color) {
  // Small house icon, mirrored roof orientation from the earlier draft.
  gfx.drawLine(cx-17,cy-2,cx,cy-19,color);
  gfx.drawLine(cx,cy-19,cx+17,cy-2,color);
  gfx.drawFastHLine(cx-17,cy-2,34,color);
  gfx.drawFastVLine(cx-13,cy-2,25,color);
  gfx.drawFastVLine(cx+13,cy-2,25,color);
  gfx.drawFastHLine(cx-13,cy+23,26,color);
  gfx.drawRoundRect(cx-3,cy+3,6,13,3,color);
  gfx.drawFastVLine(cx,cy+6,10,color);
  gfx.drawCircle(cx,cy+18,4,color);
}

static void drawHouseDivider() {
  // House roof is restricted to the empty center corridor.
  // No line runs over either data column.
  gfx.drawLine(194,42,214,23,BLUE);
  gfx.drawLine(214,23,234,42,BLUE);
  gfx.drawFastVLine(214,42,91,BLUE);
  gfx.drawFastHLine(214,133,204,BLUE);
}

static void drawLabel(int x, int y, const char *s) {
  gfx.setFont(&FreeSansBold9pt7b);
  gfx.setTextColor(BLUE);
  gfx.setCursor(x,y);
  gfx.print(s);
  gfx.setFont();
}

static void drawDashboard() {
  gfx.fillScreen(BLACK);

  // Titles occupy their own top row.
  drawLabel(14,17,"OUTDOOR");
  drawLabel(335,17,"INDOOR");

  // The house divider is centered between columns, away from content.
  drawHouseDivider();

  // OUTDOOR column: three consistent rows.
  drawRainCloud(35,39,BLUE);
  int x = drawSegmentNumber(67,27,"-5.2",BLUE);
  drawUnitC(x+2,27,BLUE);

  gfx.drawFastHLine(12,72,187,GREY);

  drawDropIcon(35,91,BLUE);
  x = drawSegmentNumber(67,78,"64",BLUE);
  gfx.setFont(&FreeSansBold12pt7b);
  gfx.setTextColor(BLUE);
  gfx.setCursor(x+4,108); gfx.print("%");
  gfx.setFont();

  gfx.drawFastHLine(12,113,187,GREY);

  drawPressureIcon(35,126,BLUE);
  x = drawSegmentNumber(67,108,"1013",BLUE);
  gfx.setFont(&FreeSansBold9pt7b);
  gfx.setTextColor(BLUE);
  gfx.setCursor(x+3,137); gfx.print("hPa");
  gfx.setFont();

  // INDOOR column: two rows, vertically balanced.
  drawHouseThermometer(255,48,BLUE);
  x = drawSegmentNumber(286,35,"22.4",BLUE);
  drawUnitC(x+2,35,BLUE);

  gfx.drawFastHLine(248,82,168,GREY);

  drawDropIcon(265,108,BLUE);
  x = drawSegmentNumber(296,94,"45",BLUE);
  gfx.setFont(&FreeSansBold12pt7b);
  gfx.setTextColor(BLUE);
  gfx.setCursor(x+4,124); gfx.print("%");
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
