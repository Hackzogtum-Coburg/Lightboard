#include <Arduino.h>
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

// Import required libraries
#include <ESP8266WiFi.h>
#include <aREST.h>

// Create aREST instance
aREST rest = aREST();
 WiFiManager wifiManager;



#define PIN            0

int numrows = 16;
int numcols = 16;
int pixelon = 0;
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
  "to /display");
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
  
 
     DynamicJsonBuffer buffer(1024*8);
      JsonObject& newjson = buffer.parseObject(server->arg("plain"));
        if(newjson == JsonObject::invalid())
      {
                  pixels.setPixelColor(1,100,0,0);
                  pixels.show();
                        server->send ( 500, "text/json",server->arg("plain"));

          return;
      }
      JsonArray& jsonPixels = newjson["pixels"];
      for(int i = 0; i < numcols*numrows; i++)
          pixels.setPixelColor(i,0,0,0);

      for (auto& pixel : jsonPixels) {
        int x = pixel["X"];
        int y = pixel["Y"];
        int r = pixel["R"];
        int g = pixel["G"];
        int b = pixel["B"];
      
        setPixel(x,y,r,g,b);

      }
     pixels.show();
      server->send ( 200, "text/json", "{success:true}" );
     //server->send ( 200, "text/json", server->arg("plain"));
}

void setup() {

    pixels.begin();
    pixels.show();
 // system_update_cpu_freq(160);

  // put your setup code here, to run once:
  wifiManager.autoConnect("LIGHTBOARD");
  
  server.reset(new ESP8266WebServer(WiFi.localIP(), 80));

  server->on("/", handleRoot);
  server->on("/reset", handleReset);


  server->on("/display", handleDisplay);

  server->client().setNoDelay(true);
  server->begin();



}

void loop() {
  server->handleClient();


  
}


