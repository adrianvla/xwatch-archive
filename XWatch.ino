#include <M5Stack.h>
#include <WiFi.h>
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#ifndef TFT_PINK
  #define TFT_PINK 0xFD79
 #endif
#define TFT_GREY 0x5AEB
//#include "Free_Fonts.h"
//#include <M5Stack.h>
float sx = 0, sy = 1, mx = 1, my = 0, hx = -1, hy = 0;    // Saved H, M, S x & y multipliers
float sdeg=0, mdeg=0, hdeg=0;
//float mx1 = 0,my1 = 0,hx1 = 0, hy1 = 0;
float m =0,h=0;
uint16_t osx=120, osy=120, omx=120, omy=120, ohx=120, ohy=120;  // Saved H, M, S x & y coords
uint16_t x0=0, x1=0, yy0=0, yy1=0;
uint32_t targetTime = 0;                    // for next 1 second timeout
float minu=0;
float hou=0;
float sou=0;
float sou1=0;
static uint8_t conv2d(const char* p); // Forward declaration needed for IDE 1.6.x
uint8_t hh=conv2d(__TIME__), mm=conv2d(__TIME__+3), ss=conv2d(__TIME__+6);  // Get H, M, S from compile time
uint8_t ss1=0;
uint8_t bedmm=0;
uint8_t bedhh=0;
uint8_t bedss=0;
boolean flagbed=0;
uint8_t bedmm1=0;
uint8_t bedhh1=0;
uint8_t bedss1=0;
boolean alarmmp3=0;
boolean alarmmp31=0;
float volume=0.25;
boolean ocnfus=0;
boolean initial = 1;
boolean confus=0;
int ssold;
int brigh;
int menu = 0;
int menus=0;
int alarmm=0;
int alarmh=0;
int alarmevent=0;
boolean flaga=0;
boolean flagb=0;
boolean flagc=0;
boolean flagd=0;
boolean flage=0;
boolean flagf=0;
boolean flagh=0;
boolean flagi=0;
boolean flagger=0;
boolean redraw;
boolean red=0;
boolean flagg;
boolean bright=200;
boolean tbright;
boolean pressed;
boolean alarmev;
boolean appmode=0;
boolean mp3draw=0;
boolean flagtime=0;
int b=0;
int app = 1;
int selecto = 0;
boolean appone;
boolean oncea=0;
#include <string.h>
#include <stdlib.h>
//=================================OTHER APPS============================================
struct Track
{
  String label;
  int timePos;
  Track *left;
  Track *right;
};

Track *trackList;
bool playing = true;
unsigned long currentMillis;

extern unsigned char logo[];
extern unsigned char timer_logo[];
extern unsigned char insertsd_logo[];
extern unsigned char error_logo[];

AudioGeneratorMP3 *mp3;
AudioFileSourceSD *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

char* strToChar(String str) {
  int len = str.length() + 1;
  char* buf = new char[len];
  strcpy(buf, str.c_str());
  return buf;
}

int cntChar(String str, char chr) {
  int n = 0;
  for (int i = 0; i < str.length(); i++)
  {
    if (str[i] == chr) n++;
  }
  return n;
}

int posChar(String str, char chr) {
  int n = cntChar(str, chr);
  int t = 0;
  for (int p = 0; p < str.length(); p++)
  {
    if (str[p] == chr) t++;
    if (t == n) return p; 
  }
  return -1;
}

String parseString(int idSeparator, char separator, String str) { // like a split JS
  String output = "";
  int separatorCout = 0;
  for (int i = 0; i < str.length(); i++)
  {
    if ((char)str[i] == separator)
    {
      separatorCout++;
    }
    else
    {
      if (separatorCout == idSeparator)
      {
        output += (char)str[i];
      }
      else if (separatorCout > idSeparator)
      {
        break;
      }
    }
  }
  return output;
}

bool createTrackList(String dir) {
  int i = 0;
  File root = SD.open(strToChar(dir));
  if (root)
  {
    while (true)
    {
      File entry =  root.openNextFile();
      if (!entry) break;
      if (!entry.isDirectory())
      {
        String ext = parseString(cntChar(entry.name(), '.'), '.', entry.name());
        if (ext == "mp3") 
        {
          i++;
          Track *tmp = new Track;
          tmp->label = entry.name();
          tmp->timePos = 0;
          tmp->right = tmp; 
          if (trackList == NULL)
          {
            tmp->left = tmp;
            trackList = tmp;
          }
          else
          {
            tmp->left = trackList;
            trackList->right = tmp;
            trackList = trackList->right;
          }
        }
      }
      entry.close();
    }
    if (i > 1)
    {
      do
      {
        trackList = trackList->left;
      } while(trackList != trackList->left);
    }
    root.close();
  }
  if (i > 0)
    return true;
  return false;
}
void drawCover() {
  M5.Lcd.drawJpgFile(SD, strToChar(trackList->label + ".jpg"), 30, 15, 75, 75);
}
bool play(char dir) {
  switch (dir)
  {
    case 'r':
      if (trackList == trackList->right) return false;
      trackList->timePos = 0;
      trackList = trackList->right;       
    break;
    case 'l':
    if (trackList == trackList->left) return false;
      trackList->timePos = 0;
      trackList = trackList->left;
    break;
    case 'm': // mute
      delete file;
      delete out;
      delete mp3;
      mp3 = NULL;
      file = NULL;
      out = NULL;
      file = new AudioFileSourceSD("/");
      id3 = new AudioFileSourceID3(file);
      out = new AudioOutputI2S(0, 1);
      out->SetOutputModeMono(true);
      mp3 = new AudioGeneratorMP3();
      mp3->begin(id3, out);
      playing = false;
    return true;
    default:
      if (playing)
      {
        trackList->timePos = id3->getPos();
        play('m');
        return true;
      }
    break;
  }
  drawCover();
  file = new AudioFileSourceSD(strToChar(trackList->label));
  id3 = new AudioFileSourceID3(file);
  id3->seek(trackList->timePos, 1);
  out = new AudioOutputI2S(0, 1);
  out->SetOutputModeMono(true);
  out->SetGain(volume);
  mp3 = new AudioGeneratorMP3();
  playing = true;
  mp3->begin(id3, out);
  
  return true;
}

unsigned long genSpectrum_previousMillis = 0;
void genSpectrum() {
  currentMillis = millis();
  if (currentMillis - genSpectrum_previousMillis > 100)
  {
    genSpectrum_previousMillis = currentMillis;
    drawSpectrum(random(0,101),random(0,101),random(0,101),random(0,101));
  }
}



unsigned long drawTimeline_previousMillis = 0;
void drawTimeline() {
  currentMillis = millis();
  if (currentMillis - drawTimeline_previousMillis > 250)
  {
    int x = 30;
    int y = 110;
    int width = 260;
    int heightLine = 2;
    int heightMark = 20;
    int widthMark = 2;
    int yClear = y - (heightMark / 2);
    int wClear = width + (widthMark / 2);
    
    drawTimeline_previousMillis = currentMillis;
    M5.Lcd.fillRect(x, yClear, wClear, heightMark, 0xffff);
    M5.Lcd.fillRect(x, y, width, heightLine, 0x7bef);
    int size_ = id3->getSize();
    int pos_ = id3->getPos();
    int xPos = x + ((pos_ * (width - (widthMark / 2))) / size_);
    M5.Lcd.fillRect(xPos, yClear, widthMark, heightMark, 0xe8e4);
  }
}

void drawSpectrum(int a, int b, int c, int d) { // %
  int x = 195;
  int y = 30;
  int padding = 10;
  int height = 30;
  int width = 15;

  int aH = ((a * height) / 100);
  int aY = y + (height - aH);
  M5.Lcd.fillRect(x, y, width, height, 0xffff);
  M5.Lcd.fillRect(x, aY, width, aH, 0x7bef); //0xe8e4
  
  int bH = ((b * height) / 100);
  int bY = y + (height - bH);
  int bX = x + width + padding;
  M5.Lcd.fillRect(bX, y, width, height, 0xffff);
  M5.Lcd.fillRect(bX, bY, width, bH, 0x7bef); //0xff80

  int cH = ((c * height) / 100);
  int cY = y + (height - cH);
  int cX = bX + width + padding;
  M5.Lcd.fillRect(cX, y, width, height, 0xffff);
  M5.Lcd.fillRect(cX, cY, width, cH, 0x7bef);//0x2589

  int dH = ((d * height) / 100);
  int dY = y + (height - dH);
  int dX = cX + width + padding;;
  M5.Lcd.fillRect(dX, y, width, height, 0xffff);
  M5.Lcd.fillRect(dX, dY, width, dH, 0x7bef);//0x051d
}

void drawTrackList() {
  M5.Lcd.fillRect(0, 130, 320, 75, 0xffff);
  if (trackList->left != trackList)
  {
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(0x7bef);
    M5.Lcd.setCursor(10, 130);
    String label = ((trackList->left)->label).substring(1, posChar(((trackList->left)->label), '.'));
    if (((trackList->left)->label).length() > 24) label = label.substring(0, 24) + ".";
    M5.Lcd.print(label);
  }
 
  M5.Lcd.setTextSize(3); 
  M5.Lcd.setTextColor(0x0000);
  M5.Lcd.setCursor(10, 155);
  String label = (trackList->label).substring(1, posChar((trackList->label), '.'));
  if ((trackList->label).length() > 16) label = label.substring(0, 16) + ".";
  M5.Lcd.print(label);

  if (trackList->right != trackList)
  {
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(0x7bef);
    M5.Lcd.setCursor(10, 185);
    String label = ((trackList->right)->label).substring(1, posChar(((trackList->right)->label), '.'));
    if (((trackList->right)->label).length() > 24) label = label.substring(0, 24) + ".";
    M5.Lcd.print(label);
  }
}

void drawGUI() {
  M5.Lcd.drawBitmap(0, 0, 320, 150, (uint16_t *)logo);
  M5.Lcd.setTextColor(0x7bef);
  drawTrackList();
  while (true)
  {
    if (m5.BtnB.wasPressed())
    {
      M5.Lcd.fillRect(0, 0, 320, 240, 0x0000);
      M5.Lcd.fillRoundRect(0, 0, 320, 240, 7, 0xffff); 
      drawTrackList();
      break; 
    }
    m5.update();
  }
}


//=======================================================================================



unsigned char months, days, years;

const char *monthss[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void getdate(void)
{
  char temp [] = __DATE__;
  unsigned char i;

  years = atoi(temp + 9);
  *(temp + 6) = 0;
  days = atoi(temp + 4);
  *(temp + 3) = 0;
  for (i = 0; i < 12; i++)
  {
    if (!strcmp(temp, monthss[i]))
    {
      months = i + 1;
      return;
    }
  }
}
void setup(void) {
  M5.begin();
  WiFi.mode(WIFI_OFF);
  //WiFi.mode(WIFI_OFF);
   //M5.Lcd.setRotation(0);
  getdate();
  //M5.Lcd.fillScreen(TFT_BLACK);
  //M5.Lcd.fillScreen(TFT_RED);
  //M5.Lcd.fillScreen(TFT_GREEN);
  //M5.Lcd.fillScreen(TFT_BLUE);
  //M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.drawJpgFile(SD, "/p2.jpg");
  //M5.Lcd.fillScreen(TFT_GREY);
  
  M5.Lcd.setTextColor(TFT_WHITE, TFT_GREY);  // Adding a background colour erases previous text automatically
  
  // Draw clock face
  //M5.Lcd.fillCircle(120, 120, 118, TFT_BLACK);
  //M5.Lcd.fillCircle(120, 120, 110, TFT_GREEN);
  //M5.Lcd.fillCircle(120, 120, 102, TFT_ORANGE);
 /* M5.Lcd.fillCircle(120, 120, 94, TFT_GREY);
  M5.Lcd.fillCircle(120, 120, 86, TFT_BLUE);
  M5.Lcd.fillCircle(120, 120, 78, TFT_RED);
  M5.Lcd.fillCircle(120, 120, 70, TFT_YELLOW);
  M5.Lcd.fillCircle(120, 120, 62, TFT_BLACK);
  M5.Lcd.fillCircle(120, 120, 54, TFT_GREEN);
  M5.Lcd.fillCircle(120, 120, 46, TFT_ORANGE);
  M5.Lcd.fillCircle(120, 120, 38, TFT_BLUE);
  M5.Lcd.fillCircle(120, 120, 30, TFT_YELLOW);
  */
  // Draw 12 lines
  for(int i = 0; i<360; i+= 30) {
    sx = cos((i-90)*0.0174532925);
    sy = sin((i-90)*0.0174532925);
    x0 = sx*114+120;
    yy0 = sy*114+120;
    x1 = sx*100+120;
    yy1 = sy*100+120;

    M5.Lcd.drawLine(x0, yy0, x1, yy1, TFT_RED);
  }

  // Draw 60 dots
  for(int i = 0; i<360; i+= 6) {
    sx = cos((i-90)*0.0174532925);
    sy = sin((i-90)*0.0174532925);
    x0 = sx*102+120;
    yy0 = sy*102+120;
    // Draw minute markers
    M5.Lcd.drawPixel(x0, yy0, TFT_ORANGE);
    
    // Draw main quadrant dots
    if(i==0 || i==180) M5.Lcd.fillCircle(x0, yy0, 2, TFT_RED);
    if(i==90 || i==270) M5.Lcd.fillCircle(x0, yy0, 2, TFT_RED);
  }

  

  // Draw text at position 120,260 using fonts 4
  // Only font numbers 2,4,6,7 are valid. Font 6 only contains characters [space] 0 1 2 3 4 5 6 7 8 9 : . - a p m
  // Font 7 is a 7 segment font and only contains characters [space] 0 1 2 3 4 5 6 7 8 9 : .
  //M5.Lcd.drawCentreString("M5Stack",120,260,4);

  targetTime = millis() + 1000; 
  //OTHER APPS=============
  /*WiFi.mode(WIFI_OFF);
  M5.Lcd.fillRoundRect(0, 0, 320, 240, 7, 0xffff);
  M5.Lcd.setTextColor(0x7bef);
  M5.Lcd.setTextSize(2);
  M5.Lcd.drawBitmap(30, 75, 59, 59, (uint16_t *)timer_logo);
  M5.Lcd.setCursor(110, 90);
  M5.Lcd.print("STARTING...");
  M5.Lcd.setCursor(110, 110);
  M5.Lcd.print("WAIT A MOMENT");
  if (!SD.begin())
  {
    M5.Lcd.fillRoundRect(0, 0, 320, 240, 7, 0xffff);
    M5.Lcd.drawBitmap(50, 70, 62, 115, (uint16_t *)insertsd_logo);
    M5.Lcd.setCursor(130, 70);
    M5.Lcd.print("INSERT");
    M5.Lcd.setCursor(130, 90);
    M5.Lcd.print("THE TF-CARD");
    M5.Lcd.setCursor(130, 110);
    M5.Lcd.print("AND TAP");
    M5.Lcd.setCursor(130, 130);
    M5.Lcd.setTextColor(0xe8e4);
    M5.Lcd.print("POWER");
    M5.Lcd.setTextColor(0x7bef);
    M5.Lcd.print(" BUTTON"); 
    while(true);
  }
  if (!createTrackList("/"))
  {
    M5.Lcd.fillRoundRect(0, 0, 320, 240, 7, 0xffff);
    M5.Lcd.drawBitmap(30, 75, 59, 59, (uint16_t *)error_logo);
    M5.Lcd.setCursor(110, 70);
    M5.Lcd.print("ADD MP3 FILES");
    M5.Lcd.setCursor(110, 90);
    M5.Lcd.print("TO THE TF-CARD");
    M5.Lcd.setCursor(110, 110);
    M5.Lcd.print("AND TAP");
    M5.Lcd.setCursor(110, 130);
    M5.Lcd.setTextColor(0xe8e4);
    M5.Lcd.print("POWER");
    M5.Lcd.setTextColor(0x7bef);
    M5.Lcd.print(" BUTTON");
    while(true);
  }
  drawGUI();
  play('m');
  */
  //=======================

  
}

void loop() {
  if(bedss==60) {
    bedmm++;
    bedss=0;
  }
  if(bedmm==60) {
      bedhh++;
      bedmm=0;
  }
  if(bedhh==24) bedhh=0;
  if(app!=2) M5.Lcd.setTextSize(0);
  if(ocnfus==1) {
    if(M5.BtnB.wasPressed()) {
      play('m');
      selecto=0;
      app=0;
      M5.Lcd.drawJpgFile(SD, "/b1.jpg");
    }
  }
    if(confus==0) {
  if(M5.BtnB.wasReleasefor(250)) {
    selecto=0;
    app=0;
    M5.Lcd.drawJpgFile(SD, "/b1.jpg");
  }
  }
  else
  {
    if(M5.BtnB.wasReleasefor(2000)) {
    selecto=0;
    app=0;
    M5.Lcd.drawJpgFile(SD, "/b1.jpg");
  }
  }
  if(app==4) app=0;   //number of apps +1 if the if(app==here)
      if (targetTime < millis()) {
    targetTime += 1000;
    ss++;    // Advance second
    bedss++;
    if (ss==60) {
      ss=0;
      mm++;            // Advance minute
      if(mm>59) {
        mm=0;
        hh++;          // Advance hour
        if (hh>23) {
          hh=0;
          //days++;
        }
        }
      }
    }
    if(app!=1) {
      if(bright==1) {
        brigh=200;
      }
      if(bright==0) {    
      if(brigh>0) {
        brigh=brigh-0.000001;
    
        }
      }
      M5.Lcd.setBrightness(brigh);
      if(M5.BtnC.wasReleasefor(4000) || tbright==1)
      {
        if (bright==0)
        {
          bright=1;
        }
        else
        {
          bright=0;
        }
          tbright=0;
        }
        if(M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
          bright=1;
          pressed=1;
        } 
    }
    
    if(app==1 && appone==0) {
      if(menu==0) redraw=1;
      M5.Lcd.drawJpgFile(SD, "/p2.jpg");
      appone=1;
    }
    if(app!=0) flagi=0;

    
    if(app==0) {
      if(flagi==0){
M5.Lcd.drawJpgFile(SD, "/b1.jpg");
flagi=1;
      }
appone=0;
if(app!=0) flagi=0;
    /*  if(select==0) {
        M5.Lcd.fillRoundRect(10, 10, 118, 118, 8, TFT_WHITE);
      }else{
        M5.Lcd.fillRoundRect(10, 10, 118, 118, 8, TFT_PINK);
      }else if(select==1) {
      d   M5.Lcd.fillRoundRect(122, 10, 118, 118, 8, TFT_WHITE);
      }else{
        M5.Lcd.fillRoundRect(122, 10, 118, 118, 8, TFT_PINK);
      }
      */
      switch(selecto)
      {
        case 0:
        M5.Lcd.fillRoundRect(10, 10, 118, 118, 8, TFT_WHITE);
        M5.Lcd.fillRoundRect(132, 10, 118, 118, 8, TFT_PINK);
        M5.Lcd.fillRoundRect(132, 100, 118, 118, 8, TFT_PINK);
        break;
        case 1:
        M5.Lcd.fillRoundRect(132, 100, 118, 118, 8, TFT_PINK);
        M5.Lcd.fillRoundRect(132, 10, 118, 118, 8, TFT_WHITE);
        M5.Lcd.fillRoundRect(10, 10, 118, 118, 8, TFT_PINK);
        break;
        case 2:
        M5.Lcd.fillRoundRect(132, 100, 118, 118, 8, TFT_WHITE);
        M5.Lcd.fillRoundRect(132, 10, 118, 118, 8, TFT_PINK);
        M5.Lcd.fillRoundRect(10, 10, 118, 118, 8, TFT_PINK);
        break;
        case 100:
        //Do Nothing
        break;
        
      }
   /*   M5.Lcd.fillRoundRect(10, 210, 45, 20, 8, TFT_BLUE);
      M5.Lcd.fillRoundRect(145, 210, 45, 20, 8, TFT_BLUE);
      M5.Lcd.fillRoundRect(305, 210, 45, 20, 8, TFT_BLUE);
      */
      //M5.Lcd.fillRect(10, 10, 118, 118, TFT_PINK);
      //M5.Lcd.
      if(M5.BtnA.wasReleased() && selecto>0) selecto--;
      if(M5.BtnC.wasReleased() && selecto<6) selecto++;
      if(M5.BtnB.wasReleased()) {
        app=selecto+1;
      }
    }
    
    //#ifdef BLABLA
    /*
    if(app==2) {
      if(appmode==0) {
        M5.Lcd.drawJpgFile(SD, "/b3.jpg");
        selecto=100;
        appmode=1;
        }
        if(M5.BtnB.wasReleasefor(1000)) {
          b++;
        }
        if(b>3) b=0;
        if(b==0) {
          if(M5.BtnA.wasReleased()) {
          mm++;    
          }
          if(M5.BtnC.wasReleased()) {
          hh++;    
          }
        }
       if(b==1) {
          if(M5.BtnA.wasReleased()) {
          days++;    
          }
          if(M5.BtnB.wasReleased()) {
          months++;    
          }
          if(M5.BtnC.wasReleased()) {
          years++;    
          }
        }
       if(b==2) {
          if(M5.BtnA.wasReleased()) {
          mm--;    
          }
          if(M5.BtnC.wasReleased()) {
          hh--;    
          }
        }
        if(b==3) {
          if(M5.BtnA.wasReleased()) {
          days--;    
          }
          if(M5.BtnB.wasReleased()) {
          months--;    
          }
          if(M5.BtnC.wasReleased()) {
          years--;    
          }
        }
      //  #endif
       M5.Lcd.setTextColor(TFT_BLUE, TFT_BLACK);
       M5.Lcd.drawNumber(mm,10,160,7);
       //M5.Lcd.drawNumber(mm,110,160,7);
       M5.Lcd.drawNumber(hh,200,160,7);
       M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
       M5.Lcd.drawNumber(days,10,100,7);
       M5.Lcd.drawNumber(months,10,100,7);
       M5.Lcd.drawNumber(years,200,100,7); 
       
    }
    */
    if(mm==61) mm=0;
    if(hh==25) hh=0;
    if(days==32) days=0;
    if(months==13) months=0;
    //if(mm==61) years=0;
    if(app==2) {
      ocnfus=1;
      //hands(120,121,10,20,TFT_WHITE);
  }
  if(app!=2) ocnfus=0;
      //M5.Lcd.drawJpgFile(SD, "/moris.jpg");
    
  if(M5.BtnA.wasReleasefor(1000)) b++;
  if(menu==0) b=0;
  if(b==4) {
    b=0;
  }
  //if(M5.BtnC.wasReleasefor(1000)) a=1;
/*        if(ss==0) {
          if(flagger==0) {
            M5.Lcd.drawJpgFile(SD, "/b3.jpg");
            flagger=1;
          }
        }
        else
        {
          flagger=0;
        }
    */
    if(app!=3) bedss=0;
    if(app!=3) flagbed=0;
    if(app==3) {
      if(flagbed==0) {
      M5.Lcd.drawJpgFile(SD, "/bedtime.jpg");
      
      flagbed=1;
      }
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(TFT_GREY, TFT_BLACK);
     // M5.Lcd.drawCentreString("Press A to start bedtime,B to exit,C to stop bedtime",200,200,2);       //Vignette
            //if(M5.BtnA.wasReleased()) {
        
        M5.Lcd.setTextSize(0);
        M5.Lcd.drawNumber(bedhh,10,50,7);
        M5.Lcd.drawNumber(bedmm,150,50,7);
        M5.Lcd.drawNumber(bedss,75,75,7);
      //}
    }
    if(app==2) {
      if(oncea==0) {
        M5.Lcd.fillRoundRect(0, 0, 320, 240, 7, 0xffff);
  M5.Lcd.setTextColor(0x7bef);
  M5.Lcd.setTextSize(2);
  M5.Lcd.drawBitmap(30, 75, 59, 59, (uint16_t *)timer_logo);
  M5.Lcd.setCursor(110, 90);
  M5.Lcd.print("STARTING...");
  M5.Lcd.setCursor(110, 110);
  M5.Lcd.print("WAIT A MOMENT");
  if (!SD.begin())
  {
    M5.Lcd.fillRoundRect(0, 0, 320, 240, 7, 0xffff);
    M5.Lcd.drawBitmap(50, 70, 62, 115, (uint16_t *)insertsd_logo);
    M5.Lcd.setCursor(130, 70);
    M5.Lcd.print("INSERT");
    M5.Lcd.setCursor(130, 90);
    M5.Lcd.print("THE TF-CARD");
    M5.Lcd.setCursor(130, 110);
    M5.Lcd.print("AND TAP");
    M5.Lcd.setCursor(130, 130);
    M5.Lcd.setTextColor(0xe8e4);
    M5.Lcd.print("POWER");
    M5.Lcd.setTextColor(0x7bef);
    M5.Lcd.print(" BUTTON"); 
    while(true);
  }
  if (!createTrackList("/"))
  {
    M5.Lcd.fillRoundRect(0, 0, 320, 240, 7, 0xffff);
    M5.Lcd.drawBitmap(30, 75, 59, 59, (uint16_t *)error_logo);
    M5.Lcd.setCursor(110, 70);
    M5.Lcd.print("ADD MP3 FILES");
    M5.Lcd.setCursor(110, 90);
    M5.Lcd.print("TO THE TF-CARD");
    M5.Lcd.setCursor(110, 110);
    M5.Lcd.print("AND TAP");
    M5.Lcd.setCursor(110, 130);
    M5.Lcd.setTextColor(0xe8e4);
    M5.Lcd.print("POWER");
    M5.Lcd.setTextColor(0x7bef);
    M5.Lcd.print(" BUTTON");
    while(true);
  }
  drawGUI();
  play('m');
        //M5.Lcd.drawJpgFile(SD, "/moris.jpg");
        oncea=1;
      }
      
     // M5.Lcd.fillCircleHelper(120,121,5,0xffff,);
     if (m5.BtnA.wasPressed())
  {
    play('l');
    drawTrackList();
  }

  if (m5.BtnB.wasPressed())
  {
    play('t');
  }

  if (m5.BtnC.wasPressed())
  {
    play('r');
    drawTrackList();
  }

  if (playing)
  {
    if (mp3->isRunning())
    {
      if (!mp3->loop())
      {
        mp3->stop();
        playing = false;
        play('r');
        drawTrackList();
      }
    }
    else
    {
      //delay(1000);
    }
    genSpectrum();
    drawTimeline();
  }
  m5.update();
}

  if(app!=2) oncea=0;
    
    if(app!=2) appmode=0;
    if(app==1) {
  if(M5.BtnC.wasReleasefor(2000)) {
    
    if(menu==1){
      redraw=1;
    menu=0;  
      
    }else{
    menu=1;
    M5.Lcd.drawJpgFile(SD, "/p2.jpg");
    M5.Lcd.setTextColor(TFT_GREY, TFT_BLACK);
    M5.Lcd.drawCentreString("Designed and programmed by Adrian",200,220,2);       //Vignette
    }
  }
  if(bright==1) {
  brigh=100;
  }
  if(bright==0) {    
  if(brigh>0) {
    brigh=brigh-0.000001;
    
    }
  }
  M5.Lcd.setBrightness(brigh);
  if(M5.BtnC.wasReleasefor(4000) || tbright==1)
  {
    if (bright==0)
    {
      bright=1;
    }
    else
    {
      bright=0;
    }
    tbright=0;
  }
    if(M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
      bright=1;
      pressed=1;
    }
    
  if(pressed==0) {
      //if(flagh==0) {
        ssold=ss;
        //flagh=1;
      //}
  
  if((ss-ssold)==5) {
      //if(flagh==0){
      tbright=1;
      //flagh=1;
      
      
    }
    //if((ssold-ss)==1) flagh=0;
  }  
if(M5.BtnA.wasReleasefor(2000))
  {
    if (alarmev==0)
    {
      alarmev=1;
    }
    else
    {
      alarmev=0;
    }
    //tbright=0;
  }
if(redraw==1) {
  M5.Lcd.drawJpgFile(SD, "/p2.jpg");
  //M5.Lcd.fillScreen(TFT_GREY);
  
  M5.Lcd.setTextColor(TFT_WHITE, TFT_GREY);  // Adding a background colour erases previous text automatically
  
  // Draw clock face
  //M5.Lcd.fillCircle(120, 120, 118, TFT_BLACK);
  //M5.Lcd.fillCircle(120, 120, 110, TFT_GREEN);
  //M5.Lcd.fillCircle(120, 120, 102, TFT_ORANGE);
 /* M5.Lcd.fillCircle(120, 120, 94, TFT_GREY);
  M5.Lcd.fillCircle(120, 120, 86, TFT_BLUE);
  M5.Lcd.fillCircle(120, 120, 78, TFT_RED);
  M5.Lcd.fillCircle(120, 120, 70, TFT_YELLOW);
  M5.Lcd.fillCircle(120, 120, 62, TFT_BLACK);
  M5.Lcd.fillCircle(120, 120, 54, TFT_GREEN);
  M5.Lcd.fillCircle(120, 120, 46, TFT_ORANGE);
  M5.Lcd.fillCircle(120, 120, 38, TFT_BLUE);
  M5.Lcd.fillCircle(120, 120, 30, TFT_YELLOW);
*/
  // Draw 12 lines
  for(int i = 0; i<360; i+= 30) {
    sx = cos((i-90)*0.0174532925);
    sy = sin((i-90)*0.0174532925);
    x0 = sx*114+120;
    yy0 = sy*114+120;
    x1 = sx*100+120;
    yy1 = sy*100+120;

    M5.Lcd.drawLine(x0, yy0, x1, yy1, TFT_RED);
  }

  // Draw 60 dots
  for(int i = 0; i<360; i+= 6) {
    sx = cos((i-90)*0.0174532925);
    sy = sin((i-90)*0.0174532925);
    x0 = sx*102+120;
    yy0 = sy*102+120;
    // Draw minute markers
    M5.Lcd.drawPixel(x0, yy0, TFT_ORANGE);
    
    // Draw main quadrant dots
    if(i==0 || i==180) M5.Lcd.fillCircle(x0, yy0, 2, TFT_RED);
    if(i==90 || i==270) M5.Lcd.fillCircle(x0, yy0, 2, TFT_RED);
  }

  

  // Draw text at position 120,260 using fonts 4
  // Only font numbers 2,4,6,7 are valid. Font 6 only contains characters [space] 0 1 2 3 4 5 6 7 8 9 : . - a p m
  // Font 7 is a 7 segment font and only contains characters [space] 0 1 2 3 4 5 6 7 8 9 : .
  //M5.Lcd.drawCentreString("M5Stack",120,260,4);

  targetTime = millis() + 1000;
  redraw=0;
  //delay(1000); 
}

/*  if(menu==1) {
  
  M5.fillScreen(TFT_BLACK);
  M5.Lcd.drawCentreString("",120,180,7)
  


  
if(M5.BtnB.wasPressed()) { 
  setup();
  menu = 0; }
  }else{
*/
  /*if(alarmevent==1) {
  M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
  }else{
    M5.Lcd.setTextColor(TFT_GREY, TFT_BLACK);
  }
  */
  if(alarmev==1) {
    if(alarmevent==1) {
    if((ss % 2)==0) {
      M5.Lcd.setTextColor(TFT_RED, TFT_ORANGE);
    }
    else
    {
      M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
    }
    }
    if(alarmevent==0) {
    if((ss % 2)==0) {
      M5.Lcd.setTextColor(TFT_GREY, TFT_ORANGE);
    }
    else
    {
      M5.Lcd.setTextColor(TFT_GREY, TFT_BLACK);
    }
    }
    }
    if(alarmev==0) {
      if(alarmevent==1) {
       M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);   
      }
      else
      {
       M5.Lcd.setTextColor(TFT_GREY, TFT_BLACK);
      }
    }
  M5.Lcd.drawCentreString("ALARM",260,30,4);
  M5.Lcd.drawNumber(alarmm,240,60,4);
  M5.Lcd.drawCentreString(":",270,60,4);
  M5.Lcd.drawNumber(alarmh,285,60,4);
 if(alarmm==0&&flaga==0) {
  //M5.Lcd.fillRect(240,60,245,53,TFT_BLACK)
  red=1;
  flaga=1;
 }
 if(alarmh==0&&flagb==0) {
  //M5.Lcd.fillRect(285,60,190,53,TFT_BLACK);
  red=1;
  flagb=1;
  }
  if(red==1) {
M5.Lcd.drawJpgFile(SD, "/p2.jpg");
    red=0;
  }
 if(alarmm!=0) flaga=0;
 if(alarmh!=0) flagb=0;
 //if(alarmm==0) alarmm=00;
  //if(alarmh==0) alarmh=00;
  if(alarmev==1) {
  if(M5.BtnB.wasReleased()) alarmh++;
  if(M5.BtnA.wasReleased()) alarmm++;
  }
  if(menu==1) M5.Lcd.setTextColor(TFT_GREY, TFT_BLACK);
  if(menu==1) {
    if(b==2) {
    M5.Lcd.setTextColor(TFT_BLUE, TFT_YELLOW);
    }
    if(b==1) {
      M5.Lcd.setTextColor(TFT_BLUE, TFT_BLACK);
    }
    if(b==4) {
      M5.Lcd.setTextColor(TFT_BLUE, TFT_RED);
    }
    if(b==0) {
      M5.Lcd.setTextColor(TFT_BLUE, TFT_BLACK);
    }
    if(b==3) {
      M5.Lcd.setTextColor(TFT_BLUE, TFT_BLACK);
    }
       M5.Lcd.drawNumber(days,10,160,7);
       M5.Lcd.drawNumber(months,110,160,7);
       M5.Lcd.drawNumber(years,200,160,7);
       M5.Lcd.setTextColor(TFT_GREY, TFT_BLACK);    
    
  }
  confus=0;
  if(mm>61) {
    M5.Lcd.drawJpgFile(SD, "/p2.jpg");
    red=1;
    M5.Lcd.drawJpgFile(SD, "/p2.jpg");
    mm=0;
    
  }
  if(hh>25) {
    M5.Lcd.drawJpgFile(SD, "/p2.jpg");
    red=1;
    M5.Lcd.drawJpgFile(SD, "/p2.jpg");
    hh=0;
    
  }
  if(days>31) {
    M5.Lcd.drawJpgFile(SD, "/p2.jpg");
days=0;
M5.Lcd.drawJpgFile(SD, "/p2.jpg");
    red=1;
  }
  if(months>12) {
    M5.Lcd.drawJpgFile(SD, "/p2.jpg");
months=0;
M5.Lcd.drawJpgFile(SD, "/p2.jpg");
    red=1;
  }
  if(years>99) {
    M5.Lcd.drawJpgFile(SD, "/p2.jpg");
years=0;
M5.Lcd.drawJpgFile(SD, "/p2.jpg");
    red=1;
  }
  if(menu==1) {
    confus=1;
    if(M5.BtnB.wasPressed() && M5.BtnA.wasPressed()) {
      b++;
    }
    if(b>4) b=0;
    if(b==1) {
      if(M5.BtnA.wasReleased()) hh++;
      if(M5.BtnC.wasReleased()) mm++;
    }
    if(b==2) {
      if(M5.BtnA.wasReleased()) days++;
      if(M5.BtnB.wasReleased()) months++;
      if(M5.BtnC.wasReleased()) years++;
    }
    if(b==3) {
      if(M5.BtnA.wasReleased()) mm=mm-1;
      if(M5.BtnB.wasReleased()) hh=mm-1;
    }
    if(b==4) {
      if(M5.BtnA.wasReleased()) days=days-1;
      if(M5.BtnB.wasReleased()) months=months-1;
      if(M5.BtnC.wasReleased()) years=years-1;
    }
  }
  if(M5.BtnC.wasPressed())
  {
    if (alarmevent==0)
    {
      alarmevent=1;
    }
    else
    {
      alarmevent=0;
    }
  }
  
 if(menu==1) {
  if(b==0) {
  M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  }
  if(b==1) {
    M5.Lcd.setTextColor(TFT_GREEN, TFT_YELLOW);
  }
  if(b==2) {
    M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  }
  if(b==3) {
    M5.Lcd.setTextColor(TFT_GREEN, TFT_RED);
  }
  if(b==4) {
  M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);  
  }
  M5.Lcd.drawNumber(hh,10,50,7);
  M5.Lcd.drawNumber(mm,150,50,7);
  M5.Lcd.drawCentreString(":",120,50,7);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.drawNumber(ss,120,100,6);
  
  
  
 }
 if(menu==1) {
 if(ss==0&&flagd==0) {
  M5.Lcd.drawJpgFile(SD, "/p2.jpg");
  //if(menu==0) redraw=1;
  flagd=1;
 }
 }
 if(red==1) {
  M5.Lcd.drawJpgFile(SD, "/p2.jpg");
  red=0;
 }
 if(ss==1) flagd=0;
 //if() alarmmp3=1;
  if(alarmm==mm&&alarmh==hh&&alarmevent==1) {
    alarmmp3=1;
    }
    if(alarmmp3==1) {
      if(flagc==0) {
    file = new AudioFileSourceSD("/alarm.mp3");
  id3 = new AudioFileSourceID3(file);
  out = new AudioOutputI2S(0, 1); // Output to builtInDAC
  out->SetOutputModeMono(true);
  mp3 = new AudioGeneratorMP3();
  mp3->begin(id3, out);
    }
  if (playing)
  {
    if (mp3->isRunning())
    {
      if (!mp3->loop())
      {
        mp3->stop();
        playing = false;
        //play('r');
       // drawTrackList();
     alarmmp3=0;
      }
    }
    }
   // M5.Speaker.beep();
   if(M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
      play('m');
    }
    flagc=1;
    }
    
  
  
  
  if(alarmm-1==mm) flagc=0;
  
  
  if(alarmm==60) alarmm=0;
  if(alarmh==24) alarmh=0;
  if (targetTime < millis()) {
    targetTime += 1000;
    ss++;              // Advance second
    if (ss==60) {
      ss=0;
      mm++;            // Advance minute
      if(mm>59) {
        mm=0;
        hh++;          // Advance hour
        if (hh>23) {
          hh=0;
          //days++;
        }
        }
      }
    }

    // Pre-compute hand degrees, x & y coords for a fast screen update
    sdeg = ss*6;                  // 0-59 -> 0-354
    mdeg = mm*6+sdeg*0.01666667;  // 0-59 -> 0-360 - includes seconds
    hdeg = hh*30+mdeg*0.0833333;  // 0-11 -> 0-360 - includes minutes and seconds
    hx = cos((hdeg-90)*0.0174532925);    
    hy = sin((hdeg-90)*0.0174532925);
    mx = cos((mdeg-90)*0.0174532925);    
    my = sin((mdeg-90)*0.0174532925);
    sx = cos((sdeg-90)*0.0174532925);    
    sy = sin((sdeg-90)*0.0174532925);
if(menu==0) {
    if (ss==0 || initial) {
      initial = 0;
      // Erase hour and minute hand positions every minute
      //M5.Lcd.drawLine(ohx, ohy, 120, 121, TFT_BLACK);
      ohx = hx*62+121;    
      ohy = hy*62+121;
      //M5.Lcd.drawLine(omx, omy, 120, 121, TFT_BLACK);
      //drawhands();
      
      omx = mx*84+120;    
      omy = my*84+121;
      //
      if(flagg==0) {
      redraw=1;
      flagg=1;
      }
    }
    
if((ss % 2) == 1) flagg=0;
      // Redraw new hand positions, hour and minute hands not erased here to avoid flicker
      //M5.Lcd.drawLine(osx, osy, 120, 121, TFT_BLACK);
      osx = sx*90+121;    
      osy = sy*90+121;
      //M5.Lcd.drawLine(osx, osy, 120, 121, TFT_YELLOW);
      /*
      M5.Lcd.drawLine(ohx, ohy, 120, 121, TFT_GREEN);
      M5.Lcd.drawLine(ohx-1, ohy-1, 120, 121, TFT_GREEN);
      M5.Lcd.drawLine(ohx-2, ohy-2, 120, 121, TFT_GREEN);
      M5.Lcd.drawLine(ohx-3, ohy-3, 120, 121, TFT_GREEN);
      M5.Lcd.drawLine(ohx-4, ohy-4, 120, 121, TFT_GREEN);
      M5.Lcd.drawLine(ohx+1, ohy+1, 120, 121, TFT_GREEN);
      M5.Lcd.drawLine(ohx+2, ohy+2, 120, 121, TFT_GREEN);
      M5.Lcd.drawLine(ohx+3, ohy+3, 120, 121, TFT_GREEN);
      
      */
      
      minu=mm*6;
      hou=hh*6*5;
      sou=ss*6;
      //sou1=(ss-1)*6;
      //M5.Lcd.drawLine((cos((hou-90)*0.0174532925)*50)+120,(sin((hou-90)*0.0174532925)*50)+121, 120, 121, TFT_WHITE);
      if(ss==1&&flagtime==0) {
      for(int radius=0;radius<71;radius+=2) {
     
     M5.Lcd.fillCircle((cos((minu-90)*0.0174532925)*radius)+120,(sin((minu-90)*0.0174532925)*radius)+121,5,TFT_WHITE);
 //radius++;
 }
      for(int radius=0;radius<71;radius+=2) {
     
     M5.Lcd.fillCircle((cos((minu-90)*0.0174532925)*radius)+120,(sin((minu-90)*0.0174532925)*radius)+121,4,TFT_BLACK);
 //radius++;
 }
 for(int radius=0;radius<51;radius+=2) {
        M5.Lcd.fillCircle((cos((hou-90)*0.0174532925)*radius)+120,(sin((hou-90)*0.0174532925)*radius)+121,5,TFT_WHITE);
      }
      for(int radius=0;radius<51;radius+=2) {
        M5.Lcd.fillCircle((cos((hou-90)*0.0174532925)*radius)+120,(sin((hou-90)*0.0174532925)*radius)+121,4,TFT_BLACK);
      }
      flagtime=1;
      }
      if(ss!=1) flagtime=0;
      //M5.Lcd.drawLine((cos((sou1-90)*0.0174532925)*70)+120,(sin((sou1-90)*0.0174532925)*70)+121, 120, 121, TFT_RED);
      //M5.Lcd.fillCircle((cos((sou1-90)*0.0174532925)*65)+120,(sin((sou1-90)*0.0174532925)*65)+121,4,TFT_RED);
      
      //M5.Lcd.drawLine((cos((sou-90)*0.0174532925)*70)+120,(sin((sou-90)*0.0174532925)*70)+121, 120, 121, TFT_RED);
      M5.Lcd.fillCircle((cos((sou-90)*0.0174532925)*65)+120,(sin((sou-90)*0.0174532925)*65)+121,4,TFT_RED);
      
      
      /*
      M5.Lcd.drawLine(omx, omy, 120, 121, TFT_BLUE);
      M5.Lcd.drawLine(omx-1, omy-1, 120, 121, TFT_BLUE);
      M5.Lcd.drawLine(omx-2, omy-2, 120, 121, TFT_BLUE);
      M5.Lcd.drawLine(omx-3, omy-3, 120, 121, TFT_BLUE);
      M5.Lcd.drawLine(omx-4, omy-4, 120, 121, TFT_BLUE);
      M5.Lcd.drawLine(omx+1, omy+1, 120, 121, TFT_BLUE);
      M5.Lcd.drawLine(omx+2, omy+2, 120, 121, TFT_BLUE);
      M5.Lcd.drawLine(omx+3, omy+3, 120, 121, TFT_BLUE);
      M5.Lcd.drawLine(omx+4, omy+4, 120, 121, TFT_BLUE);
      */
      

      if((ss % 2 ) == 0&&flage==0) {
//M5.Lcd.fillCircle(120, 121, 6, TFT_BLACK);
//M5.Lcd.fillCircle(120, 121, 4, TFT_BLACK);
//M5.Lcd.fillCircle(120, 121, 2, TFT_BLUE);
//
//M5.Lcd.fillCircle(120, 121, 3, TFT_BLACK);
//M5.Lcd.fillCircle(120, 121, 6, TFT_WHITE);
//M5.Lcd.fillCircle(120, 121, 4, TFT_YELLOW);
//M5.Lcd.fillCircle(120, 121, 2, TFT_BLUE);
//M5.Lcd.fillCircle(120, 121, 1, TFT_BLACK);   
      flage=1;
      flagf=0; 
      } 
      if((ss % 2) == 1&&flagf==0){
//      M5.Lcd.fillCircle(120, 121, 6, TFT_BLUE);
//M5.Lcd.fillCircle(120, 121, 4, TFT_BLACK);
//
//M5.Lcd.fillCircle(120, 121, 6, TFT_WHITE);
//M5.Lcd.fillCircle(120, 121, 4, TFT_YELLOW);
//M5.Lcd.fillCircle(120, 121, 2, TFT_BLUE);
//M5.Lcd.fillCircle(120, 121, 1, TFT_GREEN);
    flage=0;
    flagf=1;
}
      
  }
    }
/*  if(M5.BtnB.wasPressed())
  {
    menu = 1;
  }
  */
  M5.update();
  //if(flagd==1) M5.update();
  

  //break;
    //case 2:
    //break;
    //case 3:
    //break;
  }
//}

static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}
