#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <LoRa_E220.h>

// ============================================================
// THIS NODE
// ============================================================

#define MY_NAME "KAVIN"
#define MY_NODE_ID 4

// ============================================================
// E220 PINS
// ============================================================

#define LORA_RX 16
#define LORA_TX 17

#define PIN_M0 25
#define PIN_M1 26
#define PIN_AUX 27

HardwareSerial E220Serial(2);

LoRa_E220 e220ttl(
  &E220Serial,
  PIN_AUX,
  PIN_M0,
  PIN_M1
);

// ============================================================
// LORA
// ============================================================

#define LORA_CHANNEL 18

// ============================================================
// NODE ADDRESSES
// ============================================================

#define THANU_ADDH 0
#define THANU_ADDL 1

#define JESS_ADDH  0
#define JESS_ADDL  2

#define ASMI_ADDH  0
#define ASMI_ADDL  3

#define NODE4_ADDH 0
#define NODE4_ADDL 4

// ============================================================
// BLE UUIDs
// ============================================================

#define SERVICE_UUID \
"6E400001-B5A3-F393-E0A9-E50E24DCCA9E"

#define RX_CHARACTERISTIC \
"6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

#define TX_CHARACTERISTIC \
"6E400003-B5A3-F393-E0A9-E50E"

// ============================================================
// BLE GLOBALS
// ============================================================

BLECharacteristic *txCharacteristic = nullptr;

bool deviceConnected = false;

// ============================================================
// LORA RECEIVE BUFFER
// ============================================================

String loraBuffer = "";


// ============================================================
// SEND DATA TO PHONE
// ============================================================

void sendToPhone(String data)
{
  if (!deviceConnected)
    return;

  data.trim();

  if (data.length() == 0)
    return;

  txCharacteristic->setValue(
    data.c_str()
  );

  txCharacteristic->notify();
}


// ============================================================
// GET DESTINATION ADDRESS
// ============================================================

bool getDestinationAddress(
  String name,
  uint8_t &addH,
  uint8_t &addL
)
{
  name.trim();
  name.toUpperCase();

  if (name == "THANU")
  {
    addH = THANU_ADDH;
    addL = THANU_ADDL;

    return true;
  }

  if (name == "JESS")
  {
    addH = JESS_ADDH;
    addL = JESS_ADDL;

    return true;
  }

  if (name == "ASMI")
  {
    addH = ASMI_ADDH;
    addL = ASMI_ADDL;

    return true;
  }

  if (name == "KAVIN")
  {
    addH = NODE4_ADDH;
    addL = NODE4_ADDL;

    return true;
  }

  return false;
}


// ============================================================
// SEND MESSAGE TO SPECIFIC NODE
// ============================================================

void sendMessageToPerson(
  String destination,
  String message
)
{
  destination.trim();
  message.trim();

  uint8_t addH;
  uint8_t addL;

  // ----------------------------------------------------------
  // FIND DESTINATION ADDRESS
  // ----------------------------------------------------------

  if (
    !getDestinationAddress(
      destination,
      addH,
      addL
    )
  )
  {
    Serial.print("Unknown destination: ");
    Serial.println(destination);

    return;
  }

  // ----------------------------------------------------------
  // DON'T SEND TO YOURSELF
  // ----------------------------------------------------------

  if (
    destination.equalsIgnoreCase(
      MY_NAME
    )
  )
  {
    Serial.println(
      "Cannot send message to yourself."
    );

    return;
  }

  // ----------------------------------------------------------
  // SEND FIXED MESSAGE
  // ----------------------------------------------------------

  ResponseStatus rs =
    e220ttl.sendFixedMessage(
      addH,
      addL,
      LORA_CHANNEL,
      message
    );

  if (
    rs.code == E220_SUCCESS
  )
  {
    Serial.println(
      "Message sent successfully."
    );
  }
  else
  {
    Serial.print(
      "LoRa send error: "
    );

    Serial.println(
      rs.getResponseDescription()
    );
  }
}


// ============================================================
// BLE SERVER CALLBACKS
// ============================================================

class ServerCallbacks :
  public BLEServerCallbacks
{
  void onConnect(
    BLEServer *server
  )
  {
    deviceConnected = true;

    Serial.println();
    Serial.println(
      "================================"
    );

    Serial.println(
      "PHONE CONNECTED"
    );

    Serial.print(
      "NODE: "
    );

    Serial.println(
      MY_NAME
    );

    Serial.println(
      "================================"
    );
  }

  void onDisconnect(
    BLEServer *server
  )
  {
    deviceConnected = false;

    Serial.println();
    Serial.println(
      "PHONE DISCONNECTED"
    );

    delay(300);

    server->startAdvertising();

    Serial.println(
      "BLE ADVERTISING RESTARTED"
    );
  }
};


// ============================================================
// PHONE -> ESP32
// ============================================================
//
// Phone sends:
//
// THANU|Hello
// JESS|Hello
// ASMI|Hello
//
// ============================================================

class RxCallbacks :
  public BLECharacteristicCallbacks
{
  void onWrite(
    BLECharacteristic *characteristic
  )
  {
    String data =
      characteristic->getValue();

    data.trim();

    if (data.length() == 0)
      return;

    // --------------------------------------------------------
    // FIND |
    // --------------------------------------------------------

    int separator =
      data.indexOf('|');

    if (separator == -1)
    {
      Serial.println(
        "Invalid message format."
      );

      Serial.println(
        "Use: NAME|MESSAGE"
      );

      return;
    }

    // --------------------------------------------------------
    // DESTINATION
    // --------------------------------------------------------

    String destination =
      data.substring(
        0,
        separator
      );

    // --------------------------------------------------------
    // MESSAGE
    // --------------------------------------------------------

    String message =
      data.substring(
        separator + 1
      );

    destination.trim();
    message.trim();

    if (
      destination.length() == 0 ||
      message.length() == 0
    )
    {
      return;
    }

    // --------------------------------------------------------
    // SEND TO SELECTED NODE
    // --------------------------------------------------------

    sendMessageToPerson(
      destination,
      message
    );
  }
};


// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);

  // ==========================================================
  // E220 PINS
  // ==========================================================

  pinMode(
    PIN_M0,
    OUTPUT
  );

  pinMode(
    PIN_M1,
    OUTPUT
  );

  pinMode(
    PIN_AUX,
    INPUT
  );

  // ==========================================================
  // NORMAL MODE
  // ==========================================================

  digitalWrite(
    PIN_M0,
    LOW
  );

  digitalWrite(
    PIN_M1,
    LOW
  );

  delay(100);

  // ==========================================================
  // E220 UART
  // ==========================================================

  E220Serial.begin(
    9600,
    SERIAL_8N1,
    LORA_RX,
    LORA_TX
  );

  e220ttl.begin();

  // ==========================================================
  // BLE
  // ==========================================================

  BLEDevice::init(
    "NODE4_CHAT"
  );

  BLEServer *server =
    BLEDevice::createServer();

  server->setCallbacks(
    new ServerCallbacks()
  );

  // ==========================================================
  // BLE SERVICE
  // ==========================================================

  BLEService *service =
    server->createService(
      SERVICE_UUID
    );

  // ==========================================================
  // ESP32 -> PHONE
  // ==========================================================

  txCharacteristic =
    service->createCharacteristic(
      TX_CHARACTERISTIC,
      BLECharacteristic::PROPERTY_NOTIFY
    );

  txCharacteristic->addDescriptor(
    new BLE2902()
  );

  // ==========================================================
  // PHONE -> ESP32
  // ==========================================================

  BLECharacteristic *rxCharacteristic =
    service->createCharacteristic(
      RX_CHARACTERISTIC,
      BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_WRITE_NR
    );

  rxCharacteristic->setCallbacks(
    new RxCallbacks()
  );

  // ==========================================================
  // START BLE
  // ==========================================================

  service->start();

  BLEAdvertising *advertising =
    BLEDevice::getAdvertising();

  advertising->addServiceUUID(
    SERVICE_UUID
  );

  advertising->setScanResponse(
    true
  );

  BLEDevice::startAdvertising();

  // ==========================================================
  // READY
  // ==========================================================

  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "        KAVIN NODE 4 READY"
  );

  Serial.println(
    "================================"
  );

  Serial.print(
    "NAME    : "
  );

  Serial.println(
    MY_NAME
  );

  Serial.println(
    "BLE     : KAVIN_CHAT"
  );

  Serial.println(
    "ADDRESS : 0x0004"
  );

  Serial.println(
    "CHANNEL : 18"
  );

  Serial.println(
    "STATUS  : READY"
  );

  Serial.println(
    "================================"
  );
}


// ============================================================
// HANDLE LORA RECEIVE
// ============================================================

void handleLoRa()
{
  while (
    E220Serial.available()
  )
  {
    byte data =
      E220Serial.read();

    // --------------------------------------------------------
    // START
    // --------------------------------------------------------

    if (data == 0x02)
    {
      loraBuffer = "";
    }

    // --------------------------------------------------------
    // END
    // --------------------------------------------------------

    else if (data == 0x03)
    {
      if (
        loraBuffer.length() > 0
      )
      {
        String received =
          loraBuffer;

        loraBuffer = "";

        // ----------------------------------------------
        // SEND RECEIVED MESSAGE TO PHONE
        // ----------------------------------------------

        sendToPhone(
          received
        );
      }
    }

    // --------------------------------------------------------
    // DATA
    // --------------------------------------------------------

    else
    {
      loraBuffer +=
        (char)data;

      if (
        loraBuffer.length() > 512
      )
      {
        loraBuffer = "";
      }
    }
  }
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  handleLoRa();

  delay(5);
}