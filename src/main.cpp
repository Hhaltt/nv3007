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

// ---------------------- Automotive-style climate dashboard ----------------------
// Compact, uncluttered layout for 428x142 landscape screen.
// No left-side frame. The roof contour itself separates outside and cabin.

static void drawThermometer(int x, int y, uint16_t color) {
  gfx.drawRoundRect(x + 4, y, 8, 22, 4, color);
  gfx.drawLine(x + 8, y + 5, x + 8, y + 19, color);
  gfx.drawCircle(x + 8, y + 24, 6, color);
  gfx.fillCircle(x + 8, y + 24, 3, color);
}

static void drawDrop(int x, int y, uint16_t color) {
  gfx.drawLine(x + 8, y, x + 1, y + 11, color);
  gfx.drawLine(x + 8, y, x + 15, y + 11, color);
  gfx.drawRoundRect(x + 2, y + 8, 13, 15, 6, color);
}

static void drawBarometer(int x, int y, uint16_t color) {
  gfx.drawCircle(x + 9, y + 10, 8, color);
  gfx.drawFastHLine(x + 4, y + 16, 10, color);
  gfx.drawLine(x + 9, y + 10, x + 14, y + 5, color);
  gfx.fillCircle(x + 9, y + 10, 2, color);
}

static void drawCabinLamp(int cx, int y, uint16_t color) {
  gfx.drawFastVLine(cx, y, 8, color);
  gfx.drawRoundRect(cx - 11, y + 7, 22, 9, 4, color);
  gfx.drawFastHLine(cx - 7, y + 11, 14, color);
  gfx.drawLine(cx - 7, y + 18, cx - 10, y + 22, color);
  gfx.drawLine(cx, y + 18, cx, y + 23, color);
  gfx.drawLine(cx + 7, y + 18, cx + 10, y + 22, color);
}

static void drawRoofAndDivider() {
  // Straight outside roof edge, then a peaked cabin roof.
  gfx.drawFastHLine(7, 38, 194, WHITE);
  gfx.drawLine(201, 38, 285, 5, WHITE);
  gfx.drawLine(285, 5, 420, 48, WHITE);

  // Central wall only; no vertical line at the left edge.
  gfx.drawFastVLine(214, 48, 82, WHITE);
}

static void drawTemperature(int iconX, int baseline, const char *value,
                            uint16_t iconColor, int textX) {
  drawThermometer(iconX, baseline - 23, iconColor);

  gfx.setFont(&FreeSansBold12pt7b);
  gfx.setTextColor(WHITE);
  gfx.setCursor(textX, baseline);
  gfx.print(value);

  int16_t x1, y1;
  uint16_t w, h;
  gfx.getTextBounds(value, textX, baseline, &x1, &y1, &w, &h);

  int degreeX = textX + w + 3;
  gfx.drawCircle(degreeX + 3, baseline - 15, 3, WHITE);
  gfx.setFont(&FreeSansBold9pt7b);
  gfx.setTextColor(WHITE);
  gfx.setCursor(degreeX + 10, baseline);
  gfx.print("C");
  gfx.setFont();
}

static void drawMetric(int iconX, int baseline, const char *value,
                       const char *unit, uint16_t iconColor,
                       uint8_t iconType, int textX) {
  if (iconType == 0) drawDrop(iconX, baseline - 20, iconColor);
  else drawBarometer(iconX, baseline - 20, iconColor);

  gfx.setFont(&FreeSansBold12pt7b);
  gfx.setTextColor(WHITE);
  gfx.setCursor(textX, baseline);
  gfx.print(value);

  int16_t x1, y1;
  uint16_t w, h;
  gfx.getTextBounds(value, textX, baseline, &x1, &y1, &w, &h);

  gfx.setFont(&FreeSansBold9pt7b);
  gfx.setTextColor(iconColor);
  gfx.setCursor(textX + w + 6, baseline);
  gfx.print(unit);
  gfx.setFont();
}

static void drawDashboard() {
  gfx.fillScreen(BLACK);

  drawRoofAndDivider();
  drawCabinLamp(285, 7, YELLOW);

  // OUTSIDE: three evenly spaced rows.
  drawTemperature(17, 69, "-5.2", BLUE, 47);
  drawMetric(17, 101, "78", "% RH", BLUE, 0, 47);
  drawMetric(17, 130, "735", "mmHg", WHITE, 1, 47);

  // INSIDE: two larger, vertically balanced rows.
  drawTemperature(230, 75, "+22.4", YELLOW, 260);
  drawMetric(230, 116, "30", "% RH", YELLOW, 0, 260);

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
