#include <Arduino.h>
#include <Streaming.h>

/********************
Arduino generic menu system
U8G2 menu example
U8G2: https://github.com/olikraus/u8g2

Oct. 2016 Stephen Denne https://github.com/datacute
Based on example from Rui Azevedo - ruihfazevedo(@rrob@)gmail.com
Original from: https://github.com/christophepersoz
creative commons license 3.0: Attribution-ShareAlike CC BY-SA
This software is furnished "as is", without technical support, and with no
warranty, express or implied, as to its usefulness for any purpose.

Thread Safe: No
Extensible: Yes

menu on U8G2 device
output: Wemos D1 mini OLED Shield (SSD1306 64x48 I2C) + Serial
input: Serial
mcu: nano328p

*/

#ifdef ESP8266
  #define typeof(x) __typeof__(x)
  #include <SPI.h>
  #include <Wire.h>
#endif

#include <menu.h>
#include <menuIO/chainStream.h>
#include <menuIO/serialOut.h>
#include <menuIO/u8g2Out.h>

using namespace Menu;

// #define LEDPIN LED_BUILTIN
#define LEDPIN A3

#define U8G2OUT PCD8544
// #define U8G2OUT SSD1306

#ifndef U8G2OUT
  #error U8G2OUT not defined! edit the scketch to choose one.
#elif (U8G2OUT==PCD8544)
  #include <SPI.h>
  #define USE_HWSPI
  #define U8_DC 9
  #define U8_CS 8
  #define U8_RST 7
  #define fontName u8g2_font_5x7_tf
  #define fontX 5
  #define fontY 9
  #define offsetX 0
  #define offsetY 0
  #define U8_Width 84
  #define U8_Height 48
  U8G2_PCD8544_84X48_1_4W_HW_SPI u8g2(U8G2_R0, U8_CS, U8_DC , U8_RST);
#elif (U8G2OUT==SSD1306)
  #include <Wire.h>
  #define fontName u8g2_font_5x7_tf
  #define fontX 5
  #define fontY 8
  #define offsetX 0
  #define offsetY 0
  #define offsetX 32
  #define offsetY 16
  #define U8_Width 64
  #define U8_Height 48
  U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, SCL, SDA);
#endif

// define menu colors --------------------------------------------------------
//each color is in the format:
//  {{disabled normal,disabled selected},{enabled normal,enabled selected, enabled editing}}
// this is a monochromatic color table
const colorDef<uint8_t> colors[] MEMMODE={
  {{0,0},{0,1,1}},//bgColor
  {{1,1},{1,0,0}},//fgColor
  {{1,1},{1,0,0}},//valColor
  {{1,1},{1,0,0}},//unitColor
  {{0,1},{0,0,1}},//cursorColor
  {{0,0},{1,1,1}},//titleColor
};

result showEvent(eventMask e,navNode& nav,prompt& item) {
  Serial.print("event: ");
  Serial.println(e);
  return proceed;
}

int test=55;

result action1(eventMask e) {
  Serial.print(e);
  Serial.println(" action1 executed, proceed menu");
  Serial.flush();
  return proceed;
}

result action2(eventMask e, navNode& nav, prompt &item, Stream &in, menuOut &out) {
  Serial.print(e);
  Serial.print(" action2 executed, quiting menu");
  return quit;
}

int ledCtrl=HIGH;

result ledOn() {
  ledCtrl=HIGH;
  return proceed;
}
result ledOff() {
  ledCtrl=LOW;
  return proceed;
}

TOGGLE(ledCtrl,setLed,"Led: ",doNothing,noEvent,noStyle//,doExit,enterEvent,noStyle
  ,VALUE("On",HIGH,doNothing,noEvent)
  ,VALUE("Off",LOW,doNothing,noEvent)
);

int selTest=0;
SELECT(selTest,selMenu,"Select",doNothing,noEvent,noStyle
  ,VALUE("Zero",0,doNothing,noEvent)
  ,VALUE("One",1,doNothing,noEvent)
  ,VALUE("Two",2,doNothing,noEvent)
);

int chooseTest=-1;
CHOOSE(chooseTest,chooseMenu,"Choose",doNothing,noEvent,noStyle
  ,VALUE("First",1,doNothing,noEvent)
  ,VALUE("Second",2,doNothing,noEvent)
  ,VALUE("Third",3,doNothing,noEvent)
  ,VALUE("Last",-1,doNothing,noEvent)
);

//customizing a prompt look!
//by extending the prompt class
class altPrompt:public prompt {
public:
  altPrompt(constMEM promptShadow& p):prompt(p) {}
  idx_t printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len,idx_t panelNr) override {
    return out.printRaw("special prompt!",len);;
  }
};

MENU(subMenu,"Sub-Menu",showEvent,anyEvent,noStyle
  ,OP("Sub1",showEvent,anyEvent)
  ,OP("Sub2",showEvent,anyEvent)
  ,OP("Sub3",showEvent,anyEvent)
  ,altOP(altPrompt,"",showEvent,anyEvent)
  ,EXIT("<Back")
);

result alert(menuOut& o,idleEvent e) {
  if (e==idling) {
    o.println("alert test");
    o.println("press [select]");
    o.println("to continue...");
  }
  return proceed;
}

result doAlert(eventMask e, navNode& nav, prompt &item, Stream &in, menuOut &out) {
  nav.root->idleOn(alert);
  return proceed;
}

char* const hexDigit PROGMEM="0123456789ABCDEF";
char* const hexNr[] PROGMEM={"0","x",hexDigit,hexDigit};
char buf1[]="0x11";

MENU(mainMenu,"Main menu",doNothing,noEvent,wrapStyle
  ,OP("Op1",action1,anyEvent)
  ,OP("Op2",action2,enterEvent)
  ,FIELD(test,"Test","%",0,100,10,1,doNothing,noEvent,wrapStyle)
  ,SUBMENU(subMenu)
  ,SUBMENU(setLed)
  ,OP("LED On",ledOn,enterEvent)
  ,OP("LED Off",ledOff,enterEvent)
  ,SUBMENU(selMenu)
  ,SUBMENU(chooseMenu)
  ,OP("Alert test",doAlert,enterEvent)
  ,EDIT("Hex",buf1,hexNr,doNothing,noEvent,noStyle)
  ,EXIT("<Back")
);

Stream* inputsList[]={&Serial};
chainStream<1> in(inputsList);//1 is the number of inputs

#define MAX_DEPTH 2

MENU_OUTPUTS(out,MAX_DEPTH
  ,U8G2_OUT(u8g2,colors,fontX,fontY,offsetX,offsetY,{0,0,U8_Width/fontX,U8_Height/fontY})
  ,SERIAL_OUT(Serial)
);

NAVROOT(nav,mainMenu,MAX_DEPTH,Serial,out);

//when menu is suspended
result idle(menuOut& o,idleEvent e) {
  o.clear();
  switch(e) {
    case idleStart:o.println("suspending menu!");break;
    case idling:o.println("suspended...");break;
    case idleEnd:o.println("resuming menu.");break;
  }
  return proceed;
}

void setup() {
  pinMode(LEDPIN,OUTPUT);
  Serial.begin(115200);
  while(!Serial);
  Serial.println("menu 4.x test");Serial.flush();
  #ifdef USE_HWSPI
    SPI.begin();
  #endif
  #ifdef USE_HWI2C
    u8g2_SetI2CAddress(u8g2.getU8g2(), 0x3d*2);
  #endif
  u8g2.begin();
  u8g2.setFont(fontName);

  //disable second option
  mainMenu[1].enabled=disabledStatus;
  nav.idleTask=idle;//point a function to be used when menu is suspended
}

void loop() {
  nav.doInput();
  digitalWrite(LEDPIN, ledCtrl);
  if (nav.changed(0)) {//only draw if menu changed for gfx device
    //change checking leaves more time for other tasks
    u8g2.firstPage();
    do nav.doOutput(); while(u8g2.nextPage());
  }
  delay(100);//simulate other tasks delay
}
