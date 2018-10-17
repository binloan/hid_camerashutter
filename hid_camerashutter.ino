/********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************

 This example was modified to work together with the Yi Cameras 
 who are compatible to the YiRemote.

 This is a rough implementation to show whats needed to get going.
 I take no responsibility for any side effects or harm caused by 
 this code.

*******************************************************************/

/*
 * This sketch uses the HID Consumer Key API to send the Volume Down
 * key when PIN_SHUTTER is grounded. This will cause your mobile device
 * to capture a photo when you are in the camera app
 */
#include <bluefruit.h>

BLEDis bledis;
BLEHidAdafruit blehid;

#define PIN_SHUTTER   11
#define PIN_MODE   7
#define MAX_PERIPHERAL_CONNECTIONS 3

uint8_t flags[] = {0x06};
uint8_t appearance[] = {0xC1, 0x03};
uint8_t incompleteServices[] = {0x12,0x18,0x0F,0x18,0x0A,0x18,0xF5,0xFE};
uint8_t serviceData[] = {0x0A,0x18,0x00,0x01};
uint8_t shortName[] = {0x58,0x69,0x61,0x6F,0x59,0x69};
uint8_t completeName[] = {0x58,0x69,0x61,0x6F,0x59,0x69,0x5F,0x52,0x43};
uint8_t remoteAdress[] = {0xA8,0x88,0x00,0x76,0xE6,0x04};

bool isKeyPressed = false;
uint8_t connectionCount = 0;

void setup()
{
  pinMode(PIN_SHUTTER, INPUT_PULLUP);  
  pinMode(PIN_MODE, INPUT_PULLUP);  

  Serial.begin(115200);

  Serial.println("Bluefruit52 HID Camera Shutter Example");
  Serial.println("--------------------------------------\n");

  Serial.println();
  Serial.println("Go to your phone's Bluetooth settings to pair your device");
  Serial.println("then open the camera application");

  Serial.println();
  Serial.printf("Set pin %d to GND to capture a photo\n", PIN_SHUTTER);
  Serial.printf("Set pin %d to GND to change camera mode\n", PIN_MODE);
  Serial.println();

  // select the amount of maximum (peripheral, central) connections possible with the NRF
  // BE SURE to have a bootloader newer than 5.0.0 selected! Otherwise you will be limited to a single connection!
  Bluefruit.begin(MAX_PERIPHERAL_CONNECTIONS, 0);
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(0);  
  Bluefruit.setName("XiaoYi_RC");
  Bluefruit.setConnectCallback(prph_connect_callback);
  Bluefruit.setDisconnectCallback(prph_disconnect_callback);

  // Configure and start DIS (Device Information Service)
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather 52");
  bledis.begin();

  /* Start BLE HID
   * Note: Apple requires BLE devices to have a min connection interval >= 20m
   * (The smaller the connection interval the faster we can send data).
   * However, for HID and MIDI device Apple will accept a min connection
   * interval as low as 11.25 ms. Therefore BLEHidAdafruit::begin() will try to
   * set the min and max connection interval to 11.25 ms and 15 ms respectively
   * for the best performance.
   */
  blehid.begin();
  // Set up and start advertising
  startAdv();
}

void startAdv(void)
{  
   //Setting YI Remote Adv Data
  Bluefruit.Advertising.addData(BLE_GAP_AD_TYPE_FLAGS, flags, 1);
  Bluefruit.Advertising.addData(BLE_GAP_AD_TYPE_APPEARANCE, appearance, 2);
  Bluefruit.Advertising.addData(BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE, incompleteServices, 8);
  Bluefruit.Advertising.addData(BLE_GAP_AD_TYPE_SERVICE_DATA, serviceData, 4);
  Bluefruit.Advertising.addData(BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, shortName, 6);
  Bluefruit.ScanResponse.addData(BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, completeName, 9);
  Bluefruit.Gap.setAddr(remoteAdress, 0);
  /* Start Advertising
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.setIntervalMS(20, 50);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

void prph_connect_callback(uint16_t conn_handle)
{  
  connectionCount++;
  char peer_name[32] = { 0 };
  Bluefruit.Gap.getPeerName(conn_handle, peer_name, sizeof(peer_name));

  if(!Bluefruit.Gap.paired(conn_handle)){
    requestPairing(peer_name);
  }

  Serial.print("Connected to ");
  Serial.println(peer_name);

  if(connectionCount < MAX_PERIPHERAL_CONNECTIONS){
    Bluefruit.Advertising.start(0);
    return;
  }
  Bluefruit.Advertising.stop();
}

void requestPairing(char* peer_name){
  if(!Bluefruit.requestPairing()){
    Serial.print("Pairing with ");
    Serial.print(peer_name);
    Serial.println(" failed. Retrying...");

    requestPairing(peer_name);
    
    return;
  }
  Serial.print("Pairing with ");
  Serial.print(peer_name);
  Serial.println(" successful!");
}

void prph_disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;
  connectionCount--;
  char peer_name[32] = { 0 };
  Bluefruit.Gap.getPeerName(conn_handle, peer_name, sizeof(peer_name));

  Serial.println();
  Serial.print("Disconnected from ");
  Serial.println(peer_name);
}

void loop()
{
  // Make sure you are connected and bonded/paired
  if ( Bluefruit.connected() && Bluefruit.connPaired() )
  {
    // Check if pin GND'ed
    if ( digitalRead(PIN_SHUTTER) == 0)
    {
      if(!isKeyPressed){
        // Send the 'volume down' key press
        uint16_t val = 0x0040; // the SHUTTER button
        blehid.inputReport(3, &val, sizeof(val));
        isKeyPressed = true;
        // need delay to ignore jitter
        delay(100);
      }
    }else if (digitalRead(PIN_MODE) == 0){
      if(!isKeyPressed){
        // Send the 'volume up' key press
        uint16_t val = 0x0080; // the MODE button
        blehid.inputReport(3, &val, sizeof(val));
        isKeyPressed = true;
        // need delay to ignore jitter
        delay(100);
      }
    }else {
      if(isKeyPressed){
        uint16_t val = 0;
        blehid.inputReport(3, &val, sizeof(val));
        isKeyPressed = false;
        delay(100);
      }
    }
  }
}
