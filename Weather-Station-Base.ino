#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <ArduinoJson.h>

const char* proxy_server = "167.205.22.103";
const unsigned int proxy_port = 8080;
const char* server = "api.aviana.fadhlika.com";
const unsigned int port = 80;

const String nodeid[] = {"baafm7rmke00rou2k8ig" , "baaeum3mke00e9u4pdtg"};
const String fields[] = {"date", "dir", "voltage", "speed", "rainfall"};

const char* ssid     = "ELKA-AP01";
const char* password = "12345678!";

char * data;
char * cmd;
bool isdata = false;
bool iscmd = false;

int pkt;
int rssi;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("LoRa Sender");

  LoRa.setPins(5, 17, 22);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // register the receive callback
  LoRa.onReceive(onReceive);

  // put the radio into receive mode
  LoRa.receive();
}

void loop() {
  if(Serial.available() > 0) {
    int i = 0;
    cmd = new char[10];
    
    while(Serial.available() > 0){
      cmd[i++] = Serial.read() - '0';
    }

    pkt = i;
  }

  if(iscmd) {
    LoRa.beginPacket();
    for(int j=0; j < pkt; j++) {
      LoRa.write(cmd[j]);
      Serial.print(cmd[j], HEX);
      Serial.print(" ");
    }
    LoRa.endPacket();
    Serial.println();
    LoRa.receive();

    delete[] cmd;
    iscmd = false;
    pkt = 0;
  }

  if(isdata) {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    int i = 0;
    char *part;
    part = strtok(data, ";");
    part = strtok(NULL, ";");
    while(part != NULL){
      if(fields[i] == "rainfall") {
        root[fields[i]] = atof(part) /10000.0f;
      } else if (fields[i] == "date") {
        root[fields[i]] = String(part) + "+07:00";         
      } else if(fields[i] == "dir"){
        root[fields[i]] = part;
      } else {
        root[fields[i]] = atof(part) / 1000.0f;
      }
      part = strtok(NULL, ";");
      i++;
    }
    root["rssi"] = rssi;

    String json;
    root.printTo(json);
    root.prettyPrintTo(Serial);
    Serial.println();
    
    isdata = false;
    delete[] data;

    WiFiClient client;
    if (client.connect(proxy_server, proxy_port)) {
        
        String req = "POST http://" + String(server) + "/data?id=baaeum3mke00e9u4pdtg HTTP/1.1\r\n";
        req += "Proxy-Authorization: Basic ZmFkaGxpa2E6NzgzNzEzMTE=\r\n";
        req += "Content-Length: " + String(json.length()) +"\r\n";
        req += "\r\n";
        req += json;

        client.println(req);
        client.println();
    }

    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 1000) {
            Serial.println("»> Client Timeout !");
            client.stop();
            return;
        }
    }
    // Read all the lines of the reply from server and print them to Serial
    while(client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
    }
    client.stop();
  }
}


void onReceive(int packetSize) {
  if(packetSize < 5){
    // received a packet
    Serial.print("Received packet '");

    for (int i = 0; i < packetSize; i++) {
      Serial.print((char)LoRa.read());
    }
    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
    iscmd = true;
  } else {
    data = new char[packetSize + 1];
    int i;
    for (i = 0; i < packetSize; i++) {
      data[i] = (char)LoRa.read();
    }
    data[i] = '\0';
    rssi = LoRa.packetRssi();
        
    isdata = true;
  }
}
