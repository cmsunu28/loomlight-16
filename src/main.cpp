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
bool keyLoaded = false;
int currentPick[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

int state = 0;
// 0: file selection
// 1: file processing
// 2: doing the thing (loop)

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

// Pick array setting
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

// Key setup

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
    keymax=keynumstring.substring(1,keynumstring.length()).toInt();
  }
    keyLoaded=true;
}

void saveKeyInfo(char *filename) {
  char newFilename[50];
  strcpy(newFilename,filename);
  strcat(newFilename,"data");
  // delete if it exists; you're resaving
  SD.remove(newFilename);
  Serial.print("opening ");
  Serial.print(newFilename);
  Serial.println(" to save...");
  File myFile = SD.open(newFilename, FILE_WRITE);
  if (myFile) {
    Serial.print("Writing to sd card...");
    Serial.println(String(keynum)+"|"+String(keymax));
    myFile.println(String(keynum)+"|"+String(keymax));
    // myFile.println(String(keynum)+"|"+String(keymax)+"|"+key);
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening file");
  }
}

bool loadKeyInfo(char *filename) {
  char newFilename[50];
  strcpy(newFilename,filename);
  strcat(newFilename,"data");
  Serial.print("attempting to open ");
  Serial.print(String(newFilename));
  Serial.println(" to load...");
  File myFile = SD.open(newFilename);
  if (myFile) {
    Serial.println("File exists, reading data...");
    // process save file by splitting by "|" and then first is keynum, second is keymax, third is key
      String Buffer_Read_Line = "";
      int LastPosition=0;
      int iterator=0;
      int totalBytes = myFile.size();
      while (myFile.available()){
        for(LastPosition=0; LastPosition<= totalBytes; LastPosition++){
          char character=myFile.read();
          Buffer_Read_Line=Buffer_Read_Line+character;
          if(String(character)=="|") {
            if (iterator==0) {
              Serial.println(Buffer_Read_Line.substring(0,Buffer_Read_Line.length()-1).toInt());
              keynum=Buffer_Read_Line.substring(0,Buffer_Read_Line.length()-1).toInt();
            }
            else if (iterator==1) {
              Serial.println(Buffer_Read_Line.substring(0,Buffer_Read_Line.length()-1).toInt());
              keymax=Buffer_Read_Line.substring(0,Buffer_Read_Line.length()-1).toInt();
            }
            // else if (iterator==2) {
            //   key=Buffer_Read_Line;
            // }
            Buffer_Read_Line="";
            iterator++;
          }
        }
      }
    return true;
  } else {
    Serial.println("error opening file; data file may not exist");
    return false;
  }
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

// Key iteration
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

// LED setting
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

// void colorWipe(int color, int wait_us) {
//   for (int i=0; i < leds.numPixels(); i++) {
//     leds.setPixel(i, color);
//     leds.show();
//     delayMicroseconds(wait_us);
//   }
// }

// void testAnimation() {
//     // change all the LEDs in 1.5 seconds
//   int microsec = 1500000 / leds.numPixels();

//   colorWipe(RED, microsec);
//   colorWipe(GREEN, microsec);
//   colorWipe(BLUE, microsec);
//   colorWipe(YELLOW, microsec);
//   colorWipe(PINK, microsec);
//   colorWipe(ORANGE, microsec);
//   colorWipe(WHITE, microsec);
// }

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

  pullTieupAndTreadling("test.wif");
  createKey();
  loadKeyInfo("test.wif");
  // saveKeyInfo("test.wif");


  // Serial.println(treadlingstring.substring(0,20));
  // Serial.println(key.substring(0,100));

}

void loop() {
  // put your main code here, to run repeatedly:
  // testAnimation();

  // test by running getnextpick every second
  Serial.println(String(keynum));
  getPick();
  setNextArray(currentPick);
  delay(1000);
  iterateKeynum(1);
  saveKeyInfo("test.wif");

  if (digitalRead(forwardbuttonpin)==HIGH) {
    Serial.println("Go forward");
    iterateKeynum(1);
    getPick();
    setNextArray(currentPick);
    saveKeyInfo("test.wif");
    delay(500);
  }

  if (digitalRead(backbuttonpin)==HIGH) {
    Serial.println("Go back");
    iterateKeynum(-1);
    getPick();
    setNextArray(currentPick);
    saveKeyInfo("test.wif");
    delay(500);
  }

}
