#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>

//블루투스로 받은 정보들 처리하기 위한 변수

int bluetoothTx = 5;
int bluetoothRx = 6;
int rgb[3] = {random(255),random(255),random(255)};
int commaPosition;
SoftwareSerial mySerial(bluetoothTx, bluetoothRx);

// 블루투스로 받은 정보 rgb값으로 분할하는 함수

void getColor(String message){
  for(int i=0;i<3;i++){
    commaPosition = message.indexOf(',');
    rgb[i] = message.substring(0,commaPosition).toInt();
    message = message.substring(commaPosition+1,message.length());
  }
}

// 패턴들 멀티태스킹 하기 위해서 만든 클래스

enum  pattern { NONE, RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, SCANNER, FADE };
// Patern directions supported:
enum  direction { FORWARD, REVERSE };
 
// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPatterns : public Adafruit_NeoPixel
{
    public:
 
    // Member Variables:  
    pattern  ActivePattern;  // which pattern is running
    direction Direction;     // direction to run the pattern
    
    unsigned long Interval;   // milliseconds between updates
    unsigned long lastUpdate; // last update of position
    
    uint32_t Color1, Color2;  // What colors are in use
    uint16_t TotalSteps;  // total number of steps in the pattern
    uint16_t Index;  // current step within the pattern
    
    void (*OnComplete)();  // Callback on completion of pattern
    
    // Constructor - calls base-class constructor to initialize strip
    NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)())
   :Adafruit_NeoPixel(pixels, pin, type)
    {
        OnComplete = callback;
    }
    
    // Update the pattern
    void Update()
    {
        if((millis() - lastUpdate) > Interval) // time to update
        {
            lastUpdate = millis();
            switch(ActivePattern)
            {
                case RAINBOW_CYCLE:
                    RainbowCycleUpdate();
                    break;
                case THEATER_CHASE:
                    TheaterChaseUpdate();
                    break;
                case COLOR_WIPE:
                    ColorWipeUpdate();
                    break;
                case SCANNER:
                    ScannerUpdate();
                    break;
                case FADE:
                    FadeUpdate();
                    break;
                default:
                    NoneUpdate();
                    break;
            }
        }
    }
  
    // Increment the Index and reset at the end
    void Increment()
    {
        if (Direction == FORWARD)
        {
           Index++;
           if (Index >= TotalSteps)
            {
                Index = 0;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
        else // Direction == REVERSE
        {
            --Index;
            if (Index <= 0)
            {
                Index = TotalSteps-1;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
    }
    
    // Reverse pattern direction
    void Reverse()
    {
        if (Direction == FORWARD)
        {
            Direction = REVERSE;
            Index = TotalSteps-1;
        }
        else
        {
            Direction = FORWARD;
            Index = 0;
        }
    }
    
    // Initialize for a RainbowCycle
    void RainbowCycle(uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = RAINBOW_CYCLE;
        Interval = interval;
        TotalSteps = 255;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Rainbow Cycle Pattern
    void RainbowCycleUpdate()
    {
        for(int i=0; i< numPixels(); i++)
        {
            setPixelColor(i, Wheel(((i * 256 / numPixels()) + Index) & 255));
        }
        show();
        Increment();
    }
 
    // Initialize for a Theater Chase
    void TheaterChase(uint32_t color1, uint32_t color2, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = THEATER_CHASE;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        Direction = dir;
   }
    
    // Update the Theater Chase Pattern
    void TheaterChaseUpdate()
    {
        for(int i=0; i< numPixels(); i++)
        {
            if ((i + Index) % 3 == 0)
            {
                setPixelColor(i, Color1);
            }
            else
            {
                setPixelColor(i, Color2);
            }
        }
        show();
        Increment();
    }
 
    // Initialize for a ColorWipe
    void ColorWipe(uint32_t color, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = COLOR_WIPE;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Color Wipe Pattern
    void ColorWipeUpdate()
    {
        setPixelColor(Index, Color1); 
        show();
        Increment();
    }

    void None()
    {
        ActivePattern = NONE;
    }
    
    // Update the Color Wipe Pattern
    void NoneUpdate()
    {
        for(int i=0; i< numPixels(); i++)
            setPixelColor(i, Color(0,0,0));
        show();
    }
    
    // Initialize for a SCANNNER
    void Scanner(uint32_t color1, uint8_t interval)
    {
        ActivePattern = SCANNER;
        Interval = interval;
        TotalSteps = (numPixels() - 1) * 2;
        Color1 = color1;
        Index = 0;
    }
 
    // Update the Scanner Pattern
    void ScannerUpdate()
    { 
        for (int i = 0; i < numPixels(); i++)
        {
            if (i == Index)  // Scan Pixel to the right
            {
                 setPixelColor(i, Color1);
            }
            else if (i == TotalSteps - Index) // Scan Pixel to the left
            {
                 setPixelColor(i, Color1);
            }
            else // Fading tail
            {
                 setPixelColor(i, DimColor(getPixelColor(i)));
            }
        }
        show();
        Increment();
    }
    
    // Initialize for a Fade
    void Fade(uint32_t color1, uint32_t color2, uint16_t steps, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = FADE;
        Interval = interval;
        TotalSteps = steps;
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Fade Pattern
    void FadeUpdate()
    {
        // Calculate linear interpolation between Color1 and Color2
        // Optimise order of operations to minimize truncation error
        uint8_t red = ((Red(Color1) * (TotalSteps - Index)) + (Red(Color2) * Index)) / TotalSteps;
        uint8_t green = ((Green(Color1) * (TotalSteps - Index)) + (Green(Color2) * Index)) / TotalSteps;
        uint8_t blue = ((Blue(Color1) * (TotalSteps - Index)) + (Blue(Color2) * Index)) / TotalSteps;
        
        ColorSet(Color(red, green, blue));
        show();
        Increment();
    }
   
    // Calculate 50% dimmed version of a color (used by ScannerUpdate)
    uint32_t DimColor(uint32_t color)
    {
        // Shift R, G and B components one bit to the right
        uint32_t dimColor = Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
        return dimColor;
    }
 
    // Set all pixels to a color (synchronously)
    void ColorSet(uint32_t color)
    {
        for (int i = 0; i < numPixels(); i++)
        {
            setPixelColor(i, color);
        }
        show();
    }
 
    // Returns the Red component of a 32-bit color
    uint8_t Red(uint32_t color)
    {
        return (color >> 16) & 0xFF;
    }
 
    // Returns the Green component of a 32-bit color
    uint8_t Green(uint32_t color)
    {
        return (color >> 8) & 0xFF;
    }
 
    // Returns the Blue component of a 32-bit color
    uint8_t Blue(uint32_t color)
    {
        return color & 0xFF;
    }
    
    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t Wheel(byte WheelPos)
    {
        WheelPos = 255 - WheelPos;
        if(WheelPos < 85)
        {
            return Color(255 - WheelPos * 3, 0, WheelPos * 3);
        }
        else if(WheelPos < 170)
        {
            WheelPos -= 85;
            return Color(0, WheelPos * 3, 255 - WheelPos * 3);
        }
        else
        {
            WheelPos -= 170;
            return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
        }
    }
};
void Ring1Complete();

// 아두이노 핀번호 변수
int neopixel = 3;
int relay1 = 8, relay2 = 9, relay3 = 10;
int button = 7;

NeoPatterns Ring1(16, neopixel, NEO_RGBW + NEO_KHZ800,  &Ring1Complete); // in function
int action = 0; // ring pattern state

// Initialize everything and prepare to start
void setup()
{
   mySerial.begin(9600);
   Serial.begin(9600);
   pinMode(relay1, OUTPUT);
   pinMode(relay2, OUTPUT);
   pinMode(relay3, OUTPUT);
   pinMode(button, INPUT);
   
   randomSeed(analogRead(0));
   
   Ring1.begin();
   Ring1.setBrightness(200);
   Ring1.RainbowCycle(3);
}


// 링 번호 릴레이 통해서 변환시키기 위한 변수 및 패턴 변화 시키기 위한 변수
int nowstate = 0; // change the pattern
int ringnumber = 1; // ring number

// Main loop
void loop()
{
   if (mySerial.available()) { //블루투스에서 넘어온 데이터 입력 및 확인
    getColor(mySerial.readStringUntil('\n'));
    Serial.println(rgb[0]);
    Serial.println(rgb[1]);
    Serial.println(rgb[2]);
  }
    // 칼라 변환
    Ring1.Color1 = Ring1.Color(rgb[0],rgb[1],rgb[2]);
    Ring1.Color2 = Ring1.Color(255-rgb[0],255-rgb[1],255-rgb[2]);

    //릴레이 변환
    if(digitalRead(button) == 0)
     {
        while(digitalRead(button) == 0);
        if(ringnumber >=3)
            ringnumber = 1;
        else
            ringnumber++;
     }
     //ring1
      
     if(ringnumber == 1)
     {
        digitalWrite(relay1,HIGH);
        digitalWrite(relay2,LOW);
        digitalWrite(relay3,LOW);
     }
      
      //ring2
      if(ringnumber == 2)
      {
          digitalWrite(relay1,LOW);
          digitalWrite(relay2,HIGH);
          digitalWrite(relay3, LOW);
      }
      
      //ring3
      if(ringnumber == 3)
      {
          digitalWrite(relay1,LOW);
          digitalWrite(relay2,LOW);
          digitalWrite(relay3,HIGH);
      }

      
    // Update the rings. 패턴 변환
    Ring1.Update();
    if(nowstate) 
    {  
      nowstate = 0;
      action++;
        if(action >= 4)
          action = 0;
        if(action == 0)
        {
           Ring1.RainbowCycle(3);
        }
         else if(action == 1)
           Ring1.TheaterChase(Ring1.Color1, Ring1.Color2, 100);
         else if(action == 2)
            Ring1.Scanner(Ring1.Color2, 55);
         else if(action == 3)
            Ring1.Fade(Ring1.Color1, Ring1.Color2,  30,  100);
    }
}


//반복횟수 카운팅
int num = 0; // number of executions
// 다음 패턴으로 넘기기 위한 함수 및 fade 패턴에서 color1 -> color2 -> color1으로 이어지도록 하기 위한 함수
void Ring1Complete()
{
  num++;
  if(Ring1.ActivePattern == FADE)
  {
    Ring1.Reverse();
  }
  if(num>=5)
  {
      nowstate = 1;
      num = 0;
  }
}
