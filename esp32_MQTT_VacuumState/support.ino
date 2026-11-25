// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;
  #if DEBUG == 1
  //listNetworks();
  #endif
  // Stop if already connected.
  if (mqtt.connected()) {
    MQTTPings = 0;
    return;
  }

  debugPrint("Connecting to MQTT... ");

  uint8_t retries = 60; //60 * 10 = 600 seconds (10 minutes)
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       debugPrintln(mqtt.connectErrorString(ret));
       debugPrintln("Retrying MQTT connection in 10 seconds...");
       mqtt.disconnect();
       delay(10000);  // wait 10 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         debugPrintln("Died waiting for MQTT");
       }
  }
  debugPrintln("MQTT Connected!");
  if (millis()> 20000) {
    debugPrint(millis()); debugPrintln("       ...  Why did we disconnect? (updating MQTTConnections)");
  if (! MQTTConnections.publish(String(++MQTTretries).c_str())) {
    debugPrintln(F("Failed"));
    } else {
    debugPrintln(F("OK!"));
    }
    
  }//if interval
}//MQTT_connect


#if DEBUG ==1  
void listNetworks() {
    // WiFi.scanNetworks will return the number of networks found.
    int n = WiFi.scanNetworks();
    byte bssid[6];

    Serial.println("Scan done");
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        Serial.print("\n # | SSID ");
          WiFi.BSSID(bssid);    

          Serial.print(bssid[5],HEX);
          Serial.print(":");
          Serial.print(bssid[4],HEX);
          Serial.print(":");
          Serial.print(bssid[3],HEX);
          Serial.print(":");
          Serial.print(bssid[2],HEX);
          Serial.print(":");
          Serial.print(bssid[1],HEX);
          Serial.print(":");
          Serial.print(bssid[0],HEX);

          Serial.println("     | RSSI | CH | Encryption");
        
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.printf("%2d",i + 1);
            Serial.print(" | ");
            Serial.printf("%-26.26s", WiFi.SSID(i).c_str());

              
            Serial.print(" | ");
            Serial.printf("%4ld", WiFi.RSSI(i));
            
            if (WiFi.SSID(i) == WLAN_SSID) MQTTConnections.publish(WiFi.RSSI(i));

            Serial.print(" | ");
            Serial.printf("%2ld", WiFi.channel(i));
            Serial.print(" | ");
            switch (WiFi.encryptionType(i))
            {
            case WIFI_AUTH_OPEN:
                Serial.print("open");
                break;
            case WIFI_AUTH_WEP:
                Serial.print("WEP");
                break;
            case WIFI_AUTH_WPA_PSK:
                Serial.print("WPA");
                break;
            case WIFI_AUTH_WPA2_PSK:
                Serial.print("WPA2");
                break;
            case WIFI_AUTH_WPA_WPA2_PSK:
                Serial.print("WPA+WPA2");
                break;
            case WIFI_AUTH_WPA2_ENTERPRISE:
                Serial.print("WPA2-EAP");
                break;
            case WIFI_AUTH_WPA3_PSK:
                Serial.print("WPA3");
                break;
            case WIFI_AUTH_WPA2_WPA3_PSK:
                Serial.print("WPA2+WPA3");
                break;
            case WIFI_AUTH_WAPI_PSK:
                Serial.print("WAPI");
                break;
            default:
                Serial.print("unknown");
            }
            Serial.println();
            delay(10);
        }
    }
    Serial.println("");
    mqtt.processPackets(500);

    // Delete the scan result to free memory for code below.
    WiFi.scanDelete();


}//listNetworks

#endif
