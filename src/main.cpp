#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

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

// ---------------------- Clean dashboard graphics ----------------------
// Layout: two calm information panels. No decorative objects behind text.

static void drawThermometer(int cx, int cy, uint16_t color) {
  // Tube
  gfx.drawRoundRect(cx - 5, cy - 17, 10, 30, 5, color);
  gfx.drawLine(cx, cy - 10, cx, cy + 13, color);
  // Bulb
  gfx.drawCircle(cx, cy + 17, 8, color);
  gfx.fillCircle(cx, cy + 17, 4, color);
}

static void drawHumidityDrop(int cx, int cy, uint16_t color) {
  // Symmetric drop with a pointed top
  gfx.drawLine(cx, cy - 11, cx - 8, cy + 2, color);
  gfx.drawLine(cx, cy - 11, cx + 8, cy + 2, color);
  gfx.drawCircle(cx, cy + 4, 8, color);
  gfx.drawLine(cx - 6, cy + 8, cx + 6, cy + 8, color);
}

static void drawSunCloud(int x, int y) {
  // Sun behind a compact cloud
  gfx.drawCircle(x + 11, y + 10, 7, YELLOW);
  gfx.drawLine(x + 11, y - 1, x + 11, y - 5, YELLOW);
  gfx.drawLine(x + 2, y + 3, x - 1, y, YELLOW);
  gfx.drawLine(x + 20, y + 3, x + 23, y, YELLOW);
  gfx.drawLine(x, y + 11, x - 4, y + 11, YELLOW);
  gfx.drawLine(x + 22, y + 11, x + 26, y + 11, YELLOW);

  // Cloud outline
  gfx.drawCircle(x + 12, y + 20, 7, WHITE);
  gfx.drawCircle(x + 23, y + 17, 9, WHITE);
  gfx.drawCircle(x + 33, y + 21, 7, WHITE);
  gfx.drawFastHLine(x + 7, y + 27, 33, WHITE);
}

static void drawIndoorHouseIcon(int x, int y) {
  // Simple house with a lit window
  gfx.drawLine(x, y + 15, x + 17, y, YELLOW);
  gfx.drawLine(x + 17, y, x + 34, y + 15, YELLOW);
  gfx.drawRect(x + 7, y + 15, 20, 17, YELLOW);
  gfx.fillRect(x + 14, y + 21, 7, 11, YELLOW);
}

static void drawHouseDivider() {
  // A clear house-shaped separator in the middle.
  gfx.drawLine(178, 40, 214, 10, WHITE);
  gfx.drawLine(214, 10, 250, 40, WHITE);
  gfx.drawLine(214, 10, 214, 132, WHITE);

  // Small chimney
  gfx.drawLine(231, 24, 231, 13, WHITE);
  gfx.drawLine(231, 13, 242, 13, WHITE);
  gfx.drawLine(242, 13, 242, 33, WHITE);

  // Ground line
  gfx.drawFastHLine(10, 132, 408, GREY);
}

static void drawTemperature(int centerX, const char *value, uint16_t iconColor) {
  drawThermometer(centerX - 73, 61, iconColor);

  gfx.setFont(&FreeSansBold12pt7b);
  gfx.setTextColor(WHITE);
  int16_t x1, y1;
  uint16_t tw, th;
  gfx.getTextBounds(value, 0, 0, &x1, &y1, &tw, &th);
  int textX = centerX - (int)tw / 2 + 8;
  gfx.setCursor(textX, 70);
  gfx.print(value);

  // Degree mark and C, placed after the numeric value.
  int degreeX = textX + tw + 3;
  gfx.drawCircle(degreeX + 3, 48, 3, WHITE);
  gfx.setFont();
  gfx.setTextSize(1);
  gfx.setCursor(degreeX + 9, 47);
  gfx.setTextColor(WHITE);
  gfx.print("C");
}

static void drawHumidity(int centerX, const char *value, uint16_t color) {
  drawHumidityDrop(centerX - 49, 108, color);

  gfx.setFont(&FreeSansBold12pt7b);
  gfx.setTextColor(WHITE);
  int16_t x1, y1;
  uint16_t tw, th;
  gfx.getTextBounds(value, 0, 0, &x1, &y1, &tw, &th);
  gfx.setCursor(centerX - (int)tw / 2 + 10, 119);
  gfx.print(value);
  gfx.setFont();
}

static void drawDashboard() {
  gfx.fillScreen(BLACK);

  // Subtle panel headers
  gfx.setFont(&FreeSansBold9pt7b);
  gfx.setTextColor(BLUE);
  gfx.setCursor(17, 17);
  gfx.print("OUTSIDE");

  gfx.setTextColor(YELLOW);
  gfx.setCursor(324, 17);
  gfx.print("INSIDE");
  gfx.setFont();

  // Small, separate weather / home pictograms
  drawSunCloud(22, 25);
  drawIndoorHouseIcon(365, 25);

  // Temperature and humidity, centered in their own halves
  drawTemperature(105, "+5.2", BLUE);
  drawTemperature(323, "+22.4", ORANGE);

  // Thin colored separators
  gfx.drawFastHLine(16, 88, 180, BLUE);
  gfx.drawFastHLine(232, 88, 180, YELLOW);

  drawHumidity(105, "78%", BLUE);
  drawHumidity(323, "48%", YELLOW);

  // Draw divider last so it remains visually crisp.
  drawHouseDivider();

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
