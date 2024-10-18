#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <WS2812Serial.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// put function declarations here:

const int numled = 16;
const int ledpin = 8;
//   Teensy 4.1:  1, 8, 14, 17, 20, 24, 29, 35, 47, 53

byte drawingMemory[numled*3];         //  3 bytes per LED
DMAMEM byte displayMemory[numled*12]; // 12 bytes per LED

WS2812Serial leds(numled, displayMemory, drawingMemory, ledpin, WS2812_GRB);

#define FLIP_UP    0xcd6133
#define FLIP_UP2    0xb33939
#define FLIP_DOWN  0x218c74
#define FLIP_DOWN2  0x227093
#define NEUTRAL  0x888888

const int forwardbuttonpin = 23;
const int backbuttonpin = 33;
int forwardButtonState = 0; // 0: not pressed. 1: pressed. 2: debounced. 3: just released
int backButtonState = 0;

// for reading/scanning file
const int maxFiles = 20;
const int maxFilenameLength = 50;
char filename[maxFilenameLength];
String allFilenames[maxFiles]={};
int filenum=0;
String tieupstring = "";
String treadlingstring = "";
String key = "";
int keynum = 1;
int keymax = 1;
bool keyLoaded = false;
int currentPick[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int lastPick[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

// the screen
#define OLED_WIDTH 128
#define OLED_HEIGHT 32
#define OLED_ADDR   0x3C
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT);

int state = 0;
// 0: file selection
// 1: doing the thing (loop)
// 3: test state

// put function definitions here:

// Pick array setting
void shufflePicks() { // set last pick to current pick and current pick to 0
  for (int x=0; x<16; x++) {
    lastPick[x]=currentPick[x];
    currentPick[x]=0;
  }
}

String getPick() {
  shufflePicks();
  int position = key.indexOf(String(keynum)+"=")+String(keynum).length()+1;
  int endpos = key.indexOf(";",position);
  String s = key.substring(position,endpos);
  s+=",";
  // Go through the thing
  String thisNumber="";
  for (unsigned int i=0; i<s.length(); i++) {
    if (s.substring(i,i+1)==",") {
      int thisLever=thisNumber.toInt();
      // Serial.println(thisNumber);
      currentPick[thisLever-1]=1;
      thisNumber="";
    }
    else {
      thisNumber+=s.substring(i,i+1);
    }
  }
  // Serial.print(currentPick[0]);
  // Serial.print(currentPick[1]);
  // Serial.print(currentPick[2]);
  // Serial.print(currentPick[3]);
  // Serial.print(currentPick[4]);
  // Serial.print(currentPick[5]);
  // Serial.print(currentPick[6]);
  // Serial.print(currentPick[7]);
  // Serial.print(currentPick[8]);
  // Serial.print(currentPick[9]);
  // Serial.print(currentPick[10]);
  // Serial.print(currentPick[11]);
  // Serial.print(currentPick[12]);
  // Serial.print(currentPick[13]);
  // Serial.print(currentPick[14]);
  // Serial.print(currentPick[15]);
  return s;
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
    keynumstring=treadlingstring.substring(lastLineReturn,equalspos).trim();
    // get the tieup number (number between this position and the next line return)
    lastLineReturn=treadlingstring.indexOf(";",lastLineReturn+1);
    String treadle = treadlingstring.substring(equalspos+1,lastLineReturn).trim();
    // get this part from the tieup by using treadle+"="
    int tieuppos = tieupstring.indexOf(String(treadle.toInt())+"=");
    String thisTieup = tieupstring.substring(tieuppos+String(treadle.toInt()).length()+1,tieupstring.indexOf(";",tieuppos)).trim();
    // Serial.println(thisTieup);
    // Serial.println(keynumstring+"="+thisTieup.substring(0,thisTieup.length()-1)+";");
    key+=keynumstring+"="+thisTieup;
    // set last line return and last equals sign
    lastEqualsSign=equalspos;
    keymax=keynumstring.substring(1,keynumstring.length()).toInt();
  }
  if (key.substring(key.length()-1,key.length())!=";") {
    key=key+";";
  }
    keyLoaded=true;
    // Serial.println(tieupstring);
    // Serial.println(treadlingstring);
    // Serial.println(key);
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
          if (String(Buffer_Read_Line).trim()!="") {
            if (writeToTieUp) {
              // write the buffer to tie-up string
              tieupstring+=String(Buffer_Read_Line).trim()+";";
              tieupstring=tieupstring.trim();
            }

            if (writeToTreadling) {
              treadlingstring+=String(Buffer_Read_Line).trim()+";";
              treadlingstring=treadlingstring.trim();
            }
          }
        }
        Buffer_Read_Line="";      //string to 0
      }
    }
    // repeat again at the end because you're at the end of the line
    if (String(Buffer_Read_Line).trim()!="") {
      if (writeToTieUp) {
        // write the buffer to tie-up string
        tieupstring+=String(Buffer_Read_Line).substring(0,String(Buffer_Read_Line).length()-1).trim()+";";
        tieupstring=tieupstring.trim();
      }

      if (writeToTreadling) {
        treadlingstring+=String(Buffer_Read_Line).substring(0,String(Buffer_Read_Line).length()-1).trim()+";";
        treadlingstring=treadlingstring.trim();
      }
    }

  }
  printFile.close();
  // Serial.println("Tieup: ");
  // Serial.println(tieupstring);
  // Serial.println("Treadling: ");
  // Serial.println(treadlingstring);
}  

// filename listing
void printDirectory(File dir) {
  int n=0;
  while (true)
  {
    File entry =  dir.openNextFile();
    if (! entry)
    {
      break;
    }
    // check if the entry.name() ends in .wif
    String thisName = entry.name();
    if (thisName.substring(thisName.length()-3,thisName.length())=="wif" && thisName.substring(0,1)!=".") {
      if (n<maxFiles) {
        allFilenames[n]=thisName;
        n++;
      } else {
        Serial.println("more than maximum # wif files found; not listing all of them.");
      }
    }
    entry.close();
  }
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
  } else if (keynum==1) {
    if (n==-1) {
      keynum = keymax;
    } else if (n<-1) {
      keynum = keymax+1+n;
    } else {
      keynum = keynum+n;
    }
  }
  else {
    keynum = keynum+n;
  }
}

// Display
void displayNewText(String s) {
  Serial.println(s);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE,SSD1306_BLACK);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(s);
  display.display();
}

// LED setting

void setPickInLeds() {
  leds.clear();
  for(int i=0; i<16; i++) {
    if (currentPick[i]==1) {
      if (keynum%2==0) {
        leds.setPixel(i,FLIP_DOWN);
      } else {
        leds.setPixel(i,FLIP_DOWN2);
      }
    }
    else if (currentPick[i]==0 && lastPick[i]==1) {
      if (keynum%2==0) {
        leds.setPixel(i,FLIP_UP);
      } else {
        leds.setPixel(i,FLIP_UP2);
      }    }
    else {
      leds.setPixel(i,NEUTRAL);
    }
  }
  leds.show();
}

void testScreenWrite() {
    // display.invertDisplay(true);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE,SSD1306_BLACK);
    display.setTextSize(2);
    display.setCursor(10, 5);
    display.print("Loomlight v1.0");
    display.display();
}

void colorWipe(int color, int wait) {
  for (int i=0; i < leds.numPixels(); i++) {
    leds.setPixel(i, color);
    leds.show();
    delayMicroseconds(wait);
  }
}

void setup() {
  
  pinMode(backbuttonpin, INPUT_PULLDOWN);
  pinMode(forwardbuttonpin, INPUT_PULLDOWN);

  pinMode(ledpin, OUTPUT);

  // put your setup code here, to run once:
  leds.begin();
  leds.show();
  leds.setBrightness(50);
  leds.clear();
  int microsec = 750000 / leds.numPixels();

  colorWipe(FLIP_DOWN, microsec);
  colorWipe(FLIP_UP, microsec);
  colorWipe(NEUTRAL, microsec);
  colorWipe(FLIP_DOWN2, microsec);
  colorWipe(FLIP_UP2, microsec);
  colorWipe(NEUTRAL, microsec);

  // screen
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  // display.clearDisplay();
  testScreenWrite();

  Serial.begin(9600);
  Serial.print("Initializing SD card...");

  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  // read all the files out from the SD card and store them
  Serial.println("printing sd files");
  File root = SD.open("/");
  printDirectory(root);
  Serial.println(allFilenames[0]);

  // automatically set filename to first filename found
  memset(filename, 0, maxFilenameLength);
  strcpy(filename,allFilenames[filenum].c_str());
  Serial.println("Setting filename to ");
  Serial.print(filename);
  String displayText = String(filename);
  if (allFilenames[1].length()!=0) {
    displayText+=" >";
  }
  displayNewText(displayText);

  // Serial.println(treadlingstring.substring(0,20));
  // Serial.println(key.substring(0,100));

}

void loop() {
  // put your main code here, to run repeatedly:

  // button presses
  if (digitalRead(backbuttonpin)==HIGH && backButtonState==0) {
    backButtonState=1;
  }
  if (digitalRead(backbuttonpin)==LOW && backButtonState==1) {
    delay(200);
    if(digitalRead(backbuttonpin)==LOW) {
      backButtonState=2;
    }
  }
  if (digitalRead(forwardbuttonpin)==HIGH && forwardButtonState==0) {
    forwardButtonState=1;
  }
  if (digitalRead(forwardbuttonpin)==LOW && forwardButtonState==1) {
    delay(200);
    if(digitalRead(forwardbuttonpin)==LOW) {
      forwardButtonState=2;
    }
  }
  

  if (state==0) {
    // file selection
    // show filenames with < and > on either side
    // when you press > button go forward to next filename

    if (forwardButtonState==2 && backButtonState==0) {
      String displayString = "";
      if (filenum<maxFiles-1 && allFilenames[filenum+1].length()!=0) {
        filenum++;
        // get filename
        memset(filename, 0, sizeof(filename));
        strcpy(filename,allFilenames[filenum].c_str());
        displayString = String(filename)+" >";
        if (filenum!=0) {
          displayString = "< "+displayString;
        }
        displayNewText(displayString);
      }
      forwardButtonState=0;
      Serial.println("Pressed forward");
      delay(200);
    }

    else if (backButtonState==2 && forwardButtonState==0) {
      String displayString = "";
      if (filenum>0 && allFilenames[filenum-1].length()!=0) {
        filenum--;
        // get filename
        memset(filename, 0, sizeof(filename));
        strcpy(filename,allFilenames[filenum].c_str());
        displayString = "< "+String(filename);
        if (filenum<19) {
          displayString = displayString+" >";
        }
        displayNewText(displayString);
      }
      Serial.println("Pressed back");
      backButtonState=0;
      delay(200);
    }
    else if ((backButtonState==2 || backButtonState==1) && (forwardButtonState==2 || forwardButtonState==1) && !(forwardButtonState==1 && backButtonState==1) ) { // back button and forward button both have a state of 1 or 2
        // invert briefly and process this
        display.invertDisplay(true);
        pullTieupAndTreadling(filename);
        createKey();
        loadKeyInfo(filename);
        // saveKeyInfo("test.wif");
        display.invertDisplay(false);
        String s = getPick();
        displayNewText(String(keynum)+": "+s);
        setPickInLeds();
        Serial.println("Pressed forward and back");
        backButtonState=0;
        forwardButtonState=0;
        state=1;
        delay(200);
    }


  }


  else if (state==1) {
    // Serial.print(String(backButtonState)+" ");
    // Serial.println(String(forwardButtonState)+" ");
    if (forwardButtonState==2 && backButtonState==0) {
      Serial.println("Go forward");
      iterateKeynum(1);
      String s = getPick();
      displayNewText(String(keynum)+": "+s);
      setPickInLeds();
      saveKeyInfo(filename);
      forwardButtonState=0;
      delay(500);
    }

    else if (backButtonState==2 && forwardButtonState==0) {
      Serial.println("Go back");
      iterateKeynum(-1);
      String s = getPick();
      displayNewText(String(keynum)+": "+s);
      setPickInLeds();
      saveKeyInfo(filename);
      backButtonState=0;
      delay(500);
    }

    else if (backButtonState==2 && forwardButtonState==2) {
      backButtonState=0;
      forwardButtonState=0;
    }
  }

}
