/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <SoftwareSerial.h>
#include <NeoPixelBrightnessBus.h>
#include <NeoPixelAnimator.h>

int freq = 2000;
int channel = 0;
int resolution = 8;
const int NUM_SAMPLES = 4;
int samp[NUM_SAMPLES] = {-100,-100,-100,-100};
int refreshSamp[NUM_SAMPLES] = {-100,-100,-100,-100};
TaskHandle_t Task1;
const uint16_t PixelCount = 12; // make sure to set this to the number of pixels in your strip
const uint8_t PixelPin = 33;  // make sure to set this to the correct pin, ignored for Esp8266
NeoPixelBrightnessBus<NeoRgbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

int curBrightness = 3;
int animState = 0;
int8_t fadeDir = 0;
int red = 0;
int green = 255;
int blue = 0; 
int anim_bar = 0;
int widthVar = 3;
int scanTime = 1; //In seconds
BLEScan* pBLEScan;
const int RSSI_THRESHOLD = -55;
const int RSSI_MAX = -70;
const int NUM_DEVICES = 25;
String lastDevice = "deviceNAME";
String myDevices[NUM_DEVICES];
int numDevices = 0;
RgbColor warn_color(0,255,12);
RgbColor samp_color(255,255,0);

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      
    }
};

void setup() {

  Serial.begin(115200);
  Serial.println("Scanning...");
  
  strip.Begin();
  strip.Show();
  strip.SetBrightness(curBrightness);
  RgbColor ring_color(green,red,blue);
  for(int i = 0; i < PixelCount; i++)
  { 
    
      strip.SetPixelColor(i, ring_color);
  }
    
    strip.Show();
    delay(1000);
      
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(1);
  pBLEScan->setWindow(1);  // less or equal setInterval value


}

void loop() {
  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(scanTime);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  int lowestRssi = -90;
  String deviceName = "test";
  String deviceAddr = "test";
  pBLEScan->stop();
  String closestAddr = "test";
  
  bool deviceDetected = false;
  
  BLEAdvertisedDevice closestDevice;
  int closestRssi = -200;
  for (int i = 0; i < foundDevices.getCount(); i++){
    BLEAdvertisedDevice device = foundDevices.getDevice(i);
    int rssi = device.getRSSI();
    deviceName = device.getName().c_str();
    deviceAddr = device.toString().c_str();
    
    deviceAddr = deviceAddr.substring(deviceAddr.indexOf("Address: ")+9,deviceAddr.indexOf("Address: ")+26);
    //check if the device was recorded already

    bool inArray = false;
    for(int c = 0; c < numDevices;c++)
    {
        if(myDevices[c] == deviceAddr)
          inArray = true;
    }
    
    if(rssi > closestRssi && !inArray)
    {
      deviceDetected = true;
      closestDevice = device;
      closestRssi = rssi;
      closestAddr = deviceAddr;
      Serial.println(closestAddr);   
    }
 
   
    
  }
  
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory



    
  if(deviceDetected)
  {
    //check if lastDevice == currentDevice

    //sample the device several times


    Serial.println("sampling period beginning for device " + closestAddr);
    strip.SetBrightness(curBrightness);
    int s = 0;
    for(int i = 0; i < PixelCount; i++)
    {
      strip.SetPixelColor(i, (0,0,0));
    }

    int watchdog_timer = millis();
    while(s < NUM_SAMPLES && millis() - watchdog_timer < 10000) //max this loop can run for is 10 seconds
    {
      for(int i = 0; i < (float(s)/float(NUM_SAMPLES-1))*PixelCount; i++)
      {
         strip.SetPixelColor(i, samp_color);       
      }//print output warning for users
      strip.Show();
      
      BLEScanResults foundDevices = pBLEScan->start(scanTime);
      pBLEScan->stop();
      for (int i = 0; i < foundDevices.getCount(); i++){
        BLEAdvertisedDevice device = foundDevices.getDevice(i);
        int rssi = device.getRSSI();
        deviceName = device.getName().c_str();
        deviceAddr = device.toString().c_str();
    
        deviceAddr = deviceAddr.substring(deviceAddr.indexOf("Address: ")+9,deviceAddr.indexOf("Address: ")+26);
        if(deviceAddr == closestAddr)
        {
          samp[s] = rssi;
          s++; //only iterate if sucessful sample taken
        }

        
        
      }
      

      
    }
    Serial.println("sampling period end");


    
    int samp_avg = 0;
    
    for(int i = 0; i<NUM_SAMPLES;i++)
    {
      samp_avg+=samp[i]; 
    }//get avg of samples
    
    samp_avg = samp_avg/NUM_SAMPLES;
    
    //check if closest device detected 
    
    if(samp_avg > RSSI_THRESHOLD)
    { //device within 6 feet of us! play the warning sirens
      
        Serial.println("Device Addr: " + closestAddr + " not in array" + " rssi: " + samp_avg + " " + numDevices);
        
        myDevices[numDevices] = closestAddr;
        numDevices++;
        //play a 5 second warning
        int timeMill = millis();
        strip.SetBrightness(255);
        while(millis()-timeMill < 5000)
        {
          for(int i = 0; i < PixelCount; i++)
          {
            for(int width = 0; width < widthVar; width++)
            {
              strip.SetPixelColor(i+width, warn_color);
              strip.Show();
            }
          
            // strip.SetPixelColor(i+1, (255,0,0));
            strip.SetPixelColor(i, (0));

            if(i+widthVar >= PixelCount)
            {
              strip.SetPixelColor((i+widthVar)-PixelCount, warn_color);
            }//wrap around
            strip.Show();
            delay(50);
            
          }
        }

        for(int i = 0; i < PixelCount; i++)
        {
            strip.SetPixelColor(i, (0,0,0));
        }
         strip.SetBrightness(curBrightness);
          strip.Show(); //clear the strip to end animation
    }
    else
    { //the device is in the "danger zone" but not a threat
      strip.SetBrightness(curBrightness);
      Serial.println("Closest Device: " + closestAddr + " rssi: " + samp_avg);
      for(int i = 0; i < PixelCount; i++)
      { 
          strip.SetPixelColor(i, (0,0,0));
      } //clear the strip

      int lenLed = map(samp_avg,RSSI_MAX,RSSI_THRESHOLD,0,PixelCount);

      for(int i = 0; i < lenLed; i++)
      {
         strip.SetPixelColor(i, warn_color);       
      }//print output warning for users
      strip.Show();
      delay(5000);
    }

    
  }
  else{
      //clear the strip 
  for(int i = 0; i < PixelCount; i++)
  {
      strip.SetPixelColor(i, (0,0,0));
  }

  }
  
  strip.Show();

  
  /*if(lowestRssi >= RSSI_THRESHOLD)
  { 
    ledcWrite(channel, 125);
    Serial.println("active" + -10*lowestRssi);
    ledcWriteTone(channel,-10*lowestRssi);
  }
  else
  {
    ledcWriteTone(channel,0);
  }

  
  Serial.println(deviceName + " " + lowestRssi + " " + deviceAddr);
  */

}
