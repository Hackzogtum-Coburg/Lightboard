#include <Arduino.h>
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include "stringstream.h"

// Import required libraries
#include <ESP8266WiFi.h>
#include <aREST.h>
#include <U8x8lib.h>

// Create aREST instance
aREST rest = aREST();
 WiFiManager wifiManager;


//U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ D2, /* data=*/ D1, /* reset=*/ U8X8_PIN_NONE); 
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ D2, /* data=*/ D1, /* reset=*/ U8X8_PIN_NONE); 
#define PIN            0

int numrows = 16;
int numcols = 16;
int pixelon = 0;
int maxBright = 100;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(numrows*numcols, PIN, NEO_GRB + NEO_KHZ800);
std::unique_ptr<ESP8266WebServer> server;

void handleReset() {
    server->send(200, "text/plain", "Resetting WLAN configuration, please connect to LIGHTBOARD access point to configure network again");
    wifiManager.resetSettings();
    ESP.restart();

}

void handleRoot() {
  server->send(200, "text/plain", "Hello from Lightboard! \n"
  "Usage is easy, just post some json like\n\n"
  "{\n"
  "  ""pixels"": [\n"
  "    {\n"
  "      ""X"": ""1"",\n"
  "      ""Y"": ""1"",\n"
  "      ""R"": ""10"",\n"
  "      ""G"": ""10"",\n"
  "      ""B"": ""10""\n"
  "    },\n"
  "    {\n"
  "      ""X"": ""2"",\n"
  "      ""Y"": ""2"",\n"
  "      ""R"": ""10"",\n"
  "      ""G"": ""10"",\n"
  "      ""B"": ""10""\n"
  "    },\n"
  "    {\n"
  "      ""X"": ""3"",\n"
  "      ""Y"": ""3"",\n"
  "      ""R"": ""10"",\n"
  "      ""G"": ""10"",\n"
  "      ""B"": ""10""\n"
  "    }\n"
  "  ]\n"
  "}\n\n"
  "to /display"
  "\n\n"
  "Alternatively to the X,Y,R,G,B keys you can send compound pixel information.\n"
  "This is neccessary when sending more than ~ 180 pixels in one json.\n"
  "\n"
  "Example:\n" 
  "{\n"
  "  ""pixels"": [\n"
  "    {\n"
  "      ""C"": ""655360""\n"
  "    },\n"
  "    {\n"
  "      ""C"": ""4281479730""\n"
  "    }\n"
  "  ]\n"
  "}\n"
  "\n"
  "the C key contains a 32 bit integer, built of X (4 Bits), Y (4Bits) and R,G,B (each 8 Bits) \n"
  "the C=4281479730 fronm above example is in binary:\n"
  "1111 1111 00110010 00110010 00110010\n"
  "X=16 Y=16    R=50    G=50     b=50\n"


  );
}

void setPixel(int x, int y, int r, int g, int b)
{
  // calculate pixelNo
  int pixelNum = y * numcols;
  if ( (y % 2) == 0)
  {
    // even row, we simply add X
    pixelNum += x;
  }
  else
  {
    // odd row, we have to count from right
    pixelNum += ((numcols-1)-x);
  }

  pixels.setPixelColor(pixelNum,r,g,b);

}

void handleDisplay()
{
   
   
 //   String * str = new String(server->arg("plain").c_str());
   // StringStream strstream(*str);
     DynamicJsonDocument buffer(1024*8);

    auto error = deserializeJson(buffer,server->arg("plain").c_str());

      //  JsonObject& newjson = buffer.parseObject(server->arg("plain").c_str());

      if (error)
      // if(newjson == JsonObject::invalid())
      {
                  pixels.setPixelColor(0,100,0,0);
                  pixels.show();
                        server->send ( 500, "text/json",server->arg("plain").c_str());

          return;
      }
       JsonObject newjson =buffer.as<JsonObject>();
      if(newjson["keep"] != "true")
      {
      
      for(int i = 0; i < numcols*numrows; i++)
          pixels.setPixelColor(i,0,0,0);
      }
      else if(newjson.containsKey("fade"))
      {
        
          int fadefactor= newjson["fade"];
          
          for(int i = 0; i < numcols*numrows; i++)
          {
            uint32_t c = pixels.getPixelColor(i);
            uint8_t r = (uint8_t)(c >> 16);
            uint8_t g = (uint8_t)(c >> 8);
            uint8_t b = (uint8_t)(c);

            if(r <= fadefactor)
              r = 0;
            else
              r-=fadefactor;
              if(g <= fadefactor)
              g = 0;
            else
              g-=fadefactor;
              if( b<= fadefactor)
              b = 0;
            else
              b-=fadefactor;
            pixels.setPixelColor(i, pixels.Color(r,g,b));
          }
      }
      JsonArray jsonPixels = newjson["pixels"];
      for (auto pixel : jsonPixels) {
        JsonObject obj= pixel;
        bool isc= obj.containsKey("C");
        if(isc )
        {
        uint32_t c = pixel["C"];
           uint8_t xy= (uint8_t)(c >> 24),
            r = (uint8_t)(c >> 16),
            g = (uint8_t)(c >>  8),
            b = (uint8_t)c;
  
        

        uint8_t x =  (xy  & 0x0F);
        uint8_t y =  (xy  & 0xF0)  >> 4;
      setPixel(x,y,r,g,b);

        }
        else{
           int x = pixel["X"];
        int y = pixel["Y"];
        int r = pixel["R"];
        int g = pixel["G"];
        int b = pixel["B"];
      
        setPixel(x,y,r,g,b);

        }
        
      }
      
/*
     while(true)
     {
        JsonObject& newjson = buffer.parseObject(strstream,);
        if(newjson == JsonObject::invalid())
          break;
        int x = newjson["X"];
        int y = newjson["Y"];
        int r = newjson["R"];
        int g = newjson["G"];
        int b = newjson["B"];
      
        setPixel(x,y,r,g,b);


    
     }

     */
     pixels.show();
      server->send ( 200, "text/json", "{success:true}" );
     //server->send ( 200, "text/json", server->arg("plain"));
}

void setup() {

u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);

      u8x8.drawString(0, 0, "   Welcome to");
      u8x8.drawString(0, 1, "   LIGHTBOARD");

    pixels.begin();
    pixels.show();
  system_update_cpu_freq(160);

        u8x8.drawString(0, 2, "Connect to Wifi");        
        u8x8.drawString(0, 3, "'Lightboard'");
        u8x8.drawString(0, 4, "And use browser");        
       // u8x8.drawString(0, 5, "192.168.4.1");
        u8x8.drawString(0, 5, "to configure");

  // put your setup code here, to run once:
  wifiManager.autoConnect("LIGHTBOARD");
  WiFi.hostname("Lightboard");


  server.reset(new ESP8266WebServer(WiFi.localIP(), 80));
u8x8.drawString(0,7,WiFi.localIP().toString().c_str());
  server->on("/", handleRoot);
  server->on("/reset", handleReset);


  server->on("/display", handleDisplay);

  server->client().setNoDelay(true);
  server->begin();
    pixels.setBrightness(maxBright);



}

void loop() {
  server->handleClient();


  
}


