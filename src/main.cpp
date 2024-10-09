#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <WS2812Serial.h>

// put function declarations here:
File myFile;
const int numled = 16;
const int ledpin = 1;
//   Teensy 4.1:  1, 8, 14, 17, 20, 24, 29, 35, 47, 53

// for reading/scanning file
const byte buffer_size = 20;  // need to be big enough
char lineBuffer[buffer_size + 1]; // keep space for a trailing null char
int bufferIndex;
char character;

String tieupstring = "";
String treadlingstring = "";
String key = "";
int index = 1;
int currentPick[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

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

void setup() {
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

  readFileTest();

}

void loop() {
  // put your main code here, to run repeatedly:
  testAnimation();
}

void getPick() {
  setCurrentPickToZero();
  int position = key.indexOf(String(index)+"=");
  int endpos = key.indexOf(10,position);
  String s = key.substring(position,endpos);
  int i = 0;
  while (i<s.length()) {
      if (s[i]!=',') {
        int nextComma = s.indexOf(',',i);
        int thisLever = s.substring(i-1,nextComma).toInt();
        currentPick[thisLever-1]=1;
        i=nextComma+1;
      }
  }
}

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

// put function definitions here:
void createKey() { // replace treadling values using tieup and put into Key
  int lastEqualsSign=0;
  int lastLineReturn=0;
// for each line in treadling string, find the last character before the line return
  while (lastEqualsSign!=treadlingstring.lastIndexOf("=")) {
    // get everything before equals sign and move to key
    int equalspos=treadlingstring.indexOf("=",lastEqualsSign+1);
    key+=treadlingstring.substring(lastLineReturn,equalspos);
    // get the tieup number (number between this position and the next line return)
    lastLineReturn=treadlingstring.indexOf(10,lastLineReturn+1);
    String treadle = treadlingstring.substring(equalspos,lastLineReturn);
    // get this part from the tieup by using treadle+"="
    int tieuppos = tieupstring.indexOf(treadle+"=");
    key+=tieupstring.substring(tieuppos,tieupstring.indexOf(10,tieuppos));
    key+=10;
    // set last line return and last equals sign
    lastEqualsSign=equalspos;
  }
  // go through tieup until you find this character
  // copy the treadling number, equals, and tieup (post equals sign) into the Key

}

void pullTieupAndTreadling(){  // get tieup and treadling strings from file 
  File printFile;
  String Buffer_Read_Line = "";
  int LastPosition=0;

  bool writeToTreadling = false;
  bool writeToTieUp = false;

  printFile = SD.open("test.wif");
  int totalBytes = printFile.size();

  while (printFile.available()){
    for(LastPosition=0; LastPosition<= totalBytes; LastPosition++){
      char caracter=printFile.read();
      Buffer_Read_Line=Buffer_Read_Line+caracter;
      if(caracter==10 ){            //ASCII new line
        Serial.println(Buffer_Read_Line);

        if (writeToTieUp) {
          // write the buffer to tie-up string
          tieupstring+=Buffer_Read_Line;
        }

        if (writeToTreadling) {
          treadlingstring+=Buffer_Read_Line;
        }

        if (Buffer_Read_Line.indexOf("[TREADLING]")>0) {
          writeToTreadling=true;
          writeToTieUp=false;
        }
        else if (Buffer_Read_Line.indexOf("[TIEUP]")>0) {
          writeToTreadling=false;
          writeToTieUp=true;
        }
        else if (Buffer_Read_Line.indexOf(";")>0) {
          writeToTreadling=false;
          writeToTieUp=false;
        }
        Buffer_Read_Line="";      //string to 0
      }
    }
  }
  printFile.close();
  Serial.println("Tieup: ");
  Serial.println(tieupstring);
  Serial.println("Treadling: ");
  Serial.println(treadlingstring);
}  

void readFileTest() {

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("test.wif", FILE_READ);

  // re-open the file for reading:
  myFile = SD.open("test.wif");
  if (myFile) {
    Serial.println("test.wif:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.wif");
  }
}

bool isInArray(int n, int array[]) {
  bool found=false;
  for (int i=0; i<sizeof(array); i++) {
    if (n==array[i]) {
      found=true;
    }
  }
  return found;
}

void setNextArray(int nextPick[]) {
  for(int i=0; i<16; i++) {
    if (isInArray(i,nextPick)) {
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