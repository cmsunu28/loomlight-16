#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <WS2812Serial.h>

// put function declarations here:

const int numled = 16;
const int ledpin = 1;
//   Teensy 4.1:  1, 8, 14, 17, 20, 24, 29, 35, 47, 53

const int forwardbuttonpin = 41;
const int backbuttonpin = 40;

// for reading/scanning file
File myFile;
const byte buffer_size = 20;  // need to be big enough
char lineBuffer[buffer_size + 1]; // keep space for a trailing null char
int bufferIndex;
char character;

String tieupstring = "";
String treadlingstring = "";
String key = "";
int keynum = 1;
int keymax = 1;
int currentPick[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

int state = 0;
// 0: processing file
// 1: waiting for button press
// 2: pressed forward button, doing next
// 3: pressed back button, doing previous

byte drawingMemory[numled*4];         //  4 bytes per LED for RGBW
DMAMEM byte displayMemory[numled*16]; // 16 bytes per LED for RGBW

WS2812Serial leds(numled, displayMemory, drawingMemory, ledpin, WS2812_GRBW);

#define RED    0x00FF0000
#define GREEN  0x0000FF00
#define BLUE   0x000000FF
#define YELLOW 0x00FFD000
#define PINK   0x44F00080
#define ORANGE 0x00FF4200
#define WHITE  0xAA000000
#define OFF 0x00000000


// put function definitions here:

void setCurrentPickToZero() {
  currentPick[0]=0;
  currentPick[1]=0;
  currentPick[2]=0;
  currentPick[3]=0;
  currentPick[4]=0;
  currentPick[5]=0;
  currentPick[6]=0;
  currentPick[7]=0;
  currentPick[8]=0;
  currentPick[9]=0;
  currentPick[10]=0;
  currentPick[11]=0;
  currentPick[12]=0;
  currentPick[13]=0;
  currentPick[14]=0;
  currentPick[15]=0;
}

void getPick() {
  setCurrentPickToZero();
  int position = key.indexOf(String(keynum)+"=")+String(keynum).length()+1;
  int endpos = key.indexOf(";",position);
  String s = key.substring(position,endpos);
  Serial.println(s);
  // Go through the thing
  String thisNumber="";
  for (int i=0; i<s.length(); i++) {
    if (s.substring(i,i+1)=",") {
      int thisLever=thisNumber.toInt();
      currentPick[thisLever-1]=1;
      thisNumber="";
    }
    else {
      thisNumber+=s.substring(i,i+1);
    }
  }
}


void createKey() { // replace treadling values using tieup and put into Key
  int lastEqualsSign=0;
  int lastLineReturn=0;
  String keynumstring = "";
// for each line in treadling string, find the last character before the line return
  while (lastEqualsSign!=treadlingstring.lastIndexOf("=")) {
    // get everything before equals sign and move to key
    int equalspos=treadlingstring.indexOf("=",lastEqualsSign+1);
    keynumstring=treadlingstring.substring(lastLineReturn,equalspos);
    // get the tieup number (number between this position and the next line return)
    lastLineReturn=treadlingstring.indexOf(";",lastLineReturn+1);
    String treadle = treadlingstring.substring(equalspos+1,lastLineReturn-1);
    // get this part from the tieup by using treadle+"="
    int tieuppos = tieupstring.indexOf(String(treadle.toInt())+"=");
    String thisTieup = tieupstring.substring(tieuppos+String(treadle.toInt()).length()+1,tieupstring.indexOf(";",tieuppos)-1);
    // Serial.println(keynumstring+"="+thisTieup.substring(0,thisTieup.length()-1)+";");
    key+=keynumstring+"="+thisTieup.substring(0,thisTieup.length()-1);
    // set last line return and last equals sign
    lastEqualsSign=equalspos;
  }
    keymax=keynumstring.toInt();
  // go through tieup until you find this character
  // copy the treadling number, equals, and tieup (post equals sign) into the Key
}

void pullTieupAndTreadling(char *filename){  // get tieup and treadling strings from file 
  File printFile;
  String Buffer_Read_Line = "";
  int LastPosition=0;

  bool writeToTreadling = false;
  bool writeToTieUp = false;

  printFile = SD.open(filename);
  int totalBytes = printFile.size();

  while (printFile.available()){
    for(LastPosition=0; LastPosition<= totalBytes; LastPosition++){
      char character=printFile.read();
      Buffer_Read_Line=Buffer_Read_Line+character;
      if(character==10 ){            //ASCII new line
        // Serial.println(Buffer_Read_Line);

        if (Buffer_Read_Line.indexOf("[TREADLING]")>=0) {
          // Serial.println("PRINTING TREADLING");
          writeToTreadling=true;
          writeToTieUp=false;
        }
        else if (Buffer_Read_Line.indexOf("[TIEUP]")>=0) {
          // Serial.println("PRINTING TIE-UP");
          writeToTreadling=false;
          writeToTieUp=true;
        }
        else if (Buffer_Read_Line.indexOf(";")>=0) {
          // Serial.println("FOUND STOP");
          writeToTreadling=false;
          writeToTieUp=false;
        }
        else {
          if (writeToTieUp) {
            // write the buffer to tie-up string
            tieupstring+=String(Buffer_Read_Line)+";";
          }

          if (writeToTreadling) {
            treadlingstring+=String(Buffer_Read_Line)+";";
          }
        }
        Buffer_Read_Line="";      //string to 0
      }
    }
  }
  printFile.close();
  // Serial.println("Tieup: ");
  // Serial.println(tieupstring);
  // Serial.println("Treadling: ");
  // Serial.println(treadlingstring);
}  


void iterateKeynum(int n) {
  if (keynum==keymax) {
    if (n>0) {
      keynum = n;
    }
    else {
      keynum = keymax+n;
    }
  } else {
    keynum = keynum+n;
  }
}

bool isInArray(int n, int array[], int length) {
  bool found=false;
  for (int i=0; i<length; i++) {
    if (n==array[i]) {
      found=true;
    }
  }
  return found;
}

void setNextArray(int nextPick[]) {
  for(int i=0; i<16; i++) {
    if (isInArray(i,nextPick,16)) {
      leds.setPixel(i,GREEN);
    }
    else {
      leds.setPixel(i,RED);
    }
  }
}

void colorWipe(int color, int wait_us) {
  for (int i=0; i < leds.numPixels(); i++) {
    leds.setPixel(i, color);
    leds.show();
    delayMicroseconds(wait_us);
  }
}

void testAnimation() {
    // change all the LEDs in 1.5 seconds
  int microsec = 1500000 / leds.numPixels();

  colorWipe(RED, microsec);
  colorWipe(GREEN, microsec);
  colorWipe(BLUE, microsec);
  colorWipe(YELLOW, microsec);
  colorWipe(PINK, microsec);
  colorWipe(ORANGE, microsec);
  colorWipe(WHITE, microsec);
}

void setup() {
  pinMode(backbuttonpin, INPUT_PULLDOWN);
  pinMode(forwardbuttonpin, INPUT_PULLDOWN);

  // put your setup code here, to run once:
  leds.begin();
  leds.setBrightness(200); // 0=off, 255=brightest

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.print("Initializing SD card...");

  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  // readFileTest("test.wif");
  pullTieupAndTreadling("test.wif");
  createKey();

  // Serial.println(treadlingstring.substring(0,20));
  // Serial.println(key.substring(0,100));

}

void loop() {
  // put your main code here, to run repeatedly:
  // testAnimation();

  // test by running getnextpick every second
  getPick();
  setNextArray(currentPick);
  delay(1000);
  iterateKeynum(1);
  Serial.println("Next...");

  if (digitalRead(forwardbuttonpin)==HIGH) {
    Serial.println("Go forward");
    iterateKeynum(1);
    getPick();
    setNextArray(currentPick);
    delay(500);
  }

  if (digitalRead(backbuttonpin)==HIGH) {
    Serial.println("Go back");
    iterateKeynum(-1);
    getPick();
    setNextArray(currentPick);
    delay(500);
  }

}
