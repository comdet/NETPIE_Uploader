#include <ESP8266WiFi.h>
#include <MicroGear.h>
#include <WiFiUdp.h>

const char* ssid     = "xxxxxxxxx";
const char* password = "xxxxxxxxx";

#define APPID   "xxxxxx"
#define KEY     "xxxxxx"
#define SECRET  "xxxxxx"
#define ALIAS   "esp8266"

WiFiClient client;
MicroGear microgear(client);
bool isUploading = false;
uint32_t uploaded = 0;
uint32_t filesize = 0;
void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
    String sTopic(topic);
    //Serial.printf("Incoming Topic : %s\n",sTopic.c_str());
    if(sTopic == String("/") + APPID+"/ping"){
      Serial.println("Received Ping ... Pong back");
      microgear.publish("/pong",ALIAS);
    }
    else if(sTopic == String("/") + APPID + "/" + ALIAS + "_upload"){
      if(!isUploading){ // first msg is total length of file
        msg[msglen] = '\0';
        String fs((char *)msg);
        filesize = fs.toInt();
        Serial.printf("Found Firmware Update : (%d byte)\n", filesize);
        WiFiUDP::stopAll();
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if(!Update.begin(maxSketchSpace)){//start with max available size
          Update.printError(Serial);
        }else{
          isUploading = true;
          Serial.println("Start Uploading!");
        }
      }else{ // incoming firmware
        for(int i=0;i<msglen;i++)
        {
          Serial.print("0x");
          Serial.print(msg[i],HEX);
          Serial.print(" ");
        }
        if(isUploading){
          if(Update.write(msg, msglen) != msglen){
            Update.printError(Serial);
          }else{
            uploaded += msglen;
          }
          Serial.printf("%d : %d msglen = %d\n",filesize,uploaded,msglen);
          if(uploaded >= filesize){ // finish uploaded
            if(uploaded == filesize){
              if(Update.end(true)){ //true to set the size to the current progress
                Serial.printf("Update Success: %u\nRebooting...\n", uploaded);
              } else {
                Update.printError(Serial);
              }
              Serial.setDebugOutput(false);
              ESP.restart();
            }
          }
        }
      }
    }
}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.println("Connected to NETPIE...");
    microgear.setName(ALIAS);
    microgear.subscribe("/ping");
    char topic[64];
    strcpy (topic,"/");
    strcat (topic,ALIAS);
    strcat (topic,"_upload");    
    microgear.subscribe(topic);    
}


void setup() {
    microgear.on(MESSAGE,onMsghandler);
    microgear.on(CONNECTED,onConnected);

    Serial.begin(115200);
    Serial.println("Starting...");

    if (WiFi.begin(ssid, password)) {
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
    }

    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    microgear.init(KEY,SECRET,ALIAS);
    microgear.connect(APPID);
}
unsigned long timer = 0;
void loop() {
    if (microgear.connected()) {
        //Serial.println("connected");
        microgear.loop();
    }else {
        Serial.println("connection lost, reconnect...");
        if (timer >= 5000) {
            microgear.connect(APPID);
            timer = 0;
        }
        else timer += 100;
    }
    delay(10);
}
