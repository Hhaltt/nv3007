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

// ---------------------- Climate dashboard ----------------------
// 428x142 logical landscape. No OUTDOOR/INDOOR captions.
// Compact independent rows, multi-colour pictograms and a single
// thick house-shaped divider between the two measurement zones.

static const uint8_t SEGMENTS[10] = {
  0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F
};

static void hSeg(int x,int y,int w,uint16_t c) {
  gfx.fillRoundRect(x,y,w,3,1,c);
}
static void vSeg(int x,int y,int h,uint16_t c) {
  gfx.fillRoundRect(x,y,3,h,1,c);
}

// 15x29 seven-segment digits. All segments are inside the digit cell.
static void drawDigit(int x,int y,uint8_t n,uint16_t c) {
  const uint8_t s=SEGMENTS[n];
  if(s&0x01) hSeg(x+3,y,9,c);
  if(s&0x02) vSeg(x+12,y+3,10,c);
  if(s&0x04) vSeg(x+12,y+16,10,c);
  if(s&0x08) hSeg(x+3,y+27,9,c);
  if(s&0x10) vSeg(x,y+16,10,c);
  if(s&0x20) vSeg(x,y+3,10,c);
  if(s&0x40) hSeg(x+3,y+13,9,c);
}

static int drawNumber(int x,int y,const char *s,uint16_t c) {
  for(;*s;++s) {
    if(*s>='0'&&*s<='9') {
      drawDigit(x,y,(uint8_t)(*s-'0'),c);
      x+=17;
    } else if(*s=='-') {
      gfx.fillRoundRect(x+1,y+13,9,3,1,c);
      x+=12;
    } else if(*s=='+') {
      gfx.fillRoundRect(x+1,y+13,9,3,1,c);
      gfx.fillRoundRect(x+4,y+10,3,9,1,c);
      x+=12;
    } else if(*s=='.') {
      gfx.fillCircle(x+1,y+29,1,c);
      x+=5;
    }
  }
  return x;
}

static void drawUnit(int x,int baseline,const char *s,uint16_t c) {
  gfx.setFont(&FreeSansBold9pt7b);
  gfx.setTextColor(c);
  gfx.setCursor(x,baseline);
  gfx.print(s);
  gfx.setFont();
}
static void drawDegreeC(int x,int y,uint16_t c) {
  gfx.drawCircle(x+2,y+4,2,c);
  drawUnit(x+7,y+11,"C",c);
}

// Filled weather cloud: cool grey body, white highlight, cyan rain.
static void drawCloudRain(int cx,int cy) {
  const uint16_t cloud=0xD69A, highlight=0xFFFF, rain=0x07FF;
  gfx.fillCircle(cx-7,cy,7,cloud);
  gfx.fillCircle(cx+2,cy-5,10,cloud);
  gfx.fillCircle(cx+12,cy,7,cloud);
  gfx.fillRoundRect(cx-13,cy-1,31,11,5,cloud);
  gfx.fillCircle(cx-1,cy-7,5,highlight);
  gfx.drawLine(cx-8,cy+13,cx-11,cy+19,rain);
  gfx.drawLine(cx+1,cy+13,cx-2,cy+19,rain);
  gfx.drawLine(cx+10,cy+13,cx+7,cy+19,rain);
}

// Teardrop with a bright highlight, not a hollow outline.
static void drawDropIcon(int cx,int cy,uint16_t body) {
  gfx.fillTriangle(cx,cy-12,cx-8,cy+1,cx+8,cy+1,body);
  gfx.fillCircle(cx,cy+2,8,body);
  gfx.fillCircle(cx,cy+2,5,body);
  gfx.drawLine(cx-3,cy+2,cx-1,cy+6,WHITE);
}

// A proper small dial with a dark face, coloured bezel and needle.
static void drawPressureIcon(int cx,int cy) {
  const uint16_t bezel=0xFD20, face=0x4208, tick=0xFFFF;
  gfx.fillCircle(cx,cy,13,bezel);
  gfx.fillCircle(cx,cy,10,face);
  gfx.drawFastVLine(cx,cy-8,3,tick);
  gfx.drawFastHLine(cx-8,cy,3,tick);
  gfx.drawFastHLine(cx+5,cy,3,tick);
  gfx.drawLine(cx,cy,cx+5,cy-6,0x07FF);
  gfx.fillCircle(cx,cy,2,tick);
}

// Indoor thermometer: yellow casing, red liquid, white bulb.
static void drawThermometer(int cx,int cy) {
  gfx.fillRoundRect(cx-5,cy-14,10,23,5,0xFFE0);
  gfx.fillRoundRect(cx-2,cy-10,4,19,2,0xF800);
  gfx.fillCircle(cx,cy+11,7,0xFFE0);
  gfx.fillCircle(cx,cy+11,4,0xF800);
  gfx.fillCircle(cx,cy+11,2,0xFFFF);
}

// One large, bold house-shaped divider. This IS the house graphic;
// there is no second house icon inside it and no full-screen border.
static void thickLine(int x1,int y1,int x2,int y2,uint16_t c,int t=3) {
  for(int i=-(t/2);i<=t/2;i++) {
    gfx.drawLine(x1+i,y1,x2+i,y2,c);
    gfx.drawLine(x1,y1+i,x2,y2+i,c);
  }
}
static void drawHouseDivider() {
  const uint16_t roof=0x07FF;
  // Mirrored roof profile, located only in the central gutter.
  thickLine(181,43,214,9,roof,3);
  thickLine(214,9,247,43,roof,3);
  // Wall and lower sill make the central house silhouette readable.
  thickLine(214,43,214,132,roof,3);
  thickLine(214,132,420,132,roof,3);
}

static void drawDashboard() {
  gfx.fillScreen(BLACK);

  // The house separator occupies the middle gutter only.
  drawHouseDivider();

  // LEFT / OUTDOOR: fixed rows with safe spacing.
  drawCloudRain(27,40);
  int x=drawNumber(51,27,"-5.2",WHITE);
  drawDegreeC(x+3,27,WHITE);
  gfx.drawFastHLine(9,64,166,0x4208);

  drawDropIcon(27,82,0x07FF);
  x=drawNumber(51,69,"64",WHITE);
  drawUnit(x+5,91,"% RH",0x07FF);
  gfx.drawFastHLine(9,101,166,0x4208);

  drawPressureIcon(27,119);
  x=drawNumber(51,105,"1013",WHITE);
  drawUnit(x+4,129,"hPa",0xFD20);

  // RIGHT / INDOOR: same typography, warm yellow accents.
  drawThermometer(250,48);
  x=drawNumber(269,35,"22.4",WHITE);
  drawDegreeC(x+2,35,WHITE);
  gfx.drawFastHLine(248,78,168,0x4208);

  drawDropIcon(250,105,0xFFE0);
  x=drawNumber(269,92,"45",WHITE);
  drawUnit(x+5,114,"% RH",0xFFE0);

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
