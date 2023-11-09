// Open Energy Receiver
// See: https://esphome.io/custom/custom_component.html
#include "esphome.h"

#include "esphome_lora_version.h"

#include <SX126x-Arduino.h>
#include <SPI.h>

// Tag for log output.
static const char* TAG = "receiver";

//////////////////////////////////////////////////////////////////////////////
// Define LoRa parameters
#define RF_FREQUENCY 915000000  // Hz
#define TX_OUTPUT_POWER 22		// dBm
#define LORA_BANDWIDTH 0		// [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 7 // [SF7..SF12]
#define LORA_CODINGRATE 1		// [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0   // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 3000
#define TX_TIMEOUT_VALUE 3000

#define BUFFER_SIZE 64 // Define the payload size here
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;

#define PKT_ON  "@+++"
#define PKT_OFF "@---"

//////////////////////////////////////////////////////////////////////////////
#define GPIO5 5
#define GPIO6 6

String lstate = ""; // Last state received
String state  = "";

//////////////////////////////////////////////////////////////////////////////
hw_config hwConfig;

// Heltec Wifi LoRa 32 (V3) - SX126x pin configuration
int PIN_LORA_RESET = 12;  // LORA RESET
int PIN_LORA_DIO_1 = 14;  // LORA DIO_1
int PIN_LORA_BUSY  = 13;  // LORA SPI BUSY
int PIN_LORA_NSS   =  8;  // LORA SPI CS
int PIN_LORA_SCLK  =  9;  // LORA SPI CLK
int PIN_LORA_MISO  = 11;  // LORA SPI MISO
int PIN_LORA_MOSI  = 10;  // LORA SPI MOSI
int RADIO_TXEN = -1;	  // LORA ANTENNA TX ENABLE
int RADIO_RXEN = -1;	  // LORA ANTENNA RX ENABLE

//////////////////////////////////////////////////////////////////////////////
// State controls
unsigned long lastMillis  = 0;
unsigned long stateMillis = millis();
unsigned long delayMillis = 0;
int inDelay = 0;

// Radio messages are processed in two steps.

// On receiving a signal that requires a state change, a delay is set, which
// will trigger more processing to occur in the main loop when this expires.
// To do this:
// - the inDelay flag is set, and a delayMillis time is set.
// Once the event has been processed the 'inDelay' flag is cleared.

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    memcpy(rxpacket, payload, size );
    rxpacket[size]='\0';

    Radio.Sleep( );
    ESP_LOGD(TAG, "Received packet \"%s\" with rssi:%d length:%d",rxpacket,rssi,size);
    if(String(rxpacket) == PKT_OFF){
        state = "OFF";
    }
    else if(String(rxpacket) == PKT_ON){
        state = "ON";
    }

    if(lstate != state){
        // Reset delay timer
        stateMillis = millis();
        inDelay = 1;

        if (lstate == "OFF" && state == "ON"){
            delayMillis = 2000;
            ESP_LOGD(TAG,"Turn ON after 2 sec delay");

        }  else if (lstate == "ON" && state == "OFF"){
            ESP_LOGD(TAG,"Turn OFF after 1 sec delay");
            delayMillis = 1000;
        }

        lstate = state;
    } else {
        ESP_LOGD(TAG,"Radio packet received: %s", state);
    }

    // Set Radio to receive next packet
    Radio.Rx(RX_TIMEOUT_VALUE);

}

unsigned long previousMillis = 0UL;
unsigned long interval = 10000UL;

class MyCustomComponent : public Component {
public:

    void setup() override {
        ESP_LOGD(TAG, "ESPHOME_LORA_VERSION: %s", ESPHOME_LORA_VERSION);

        pinMode(GPIO5,OUTPUT);
        pinMode(GPIO6,OUTPUT);

        ESP_LOGD(TAG, "Setup SPI LoRa");

        // Define the HW configuration between MCU and SX126x
        hwConfig.CHIP_TYPE = SX1262_CHIP;		  // Example uses an eByte E22 module with an SX1262
        hwConfig.PIN_LORA_RESET = PIN_LORA_RESET; // LORA RESET
        hwConfig.PIN_LORA_NSS = PIN_LORA_NSS;	  // LORA SPI CS
        hwConfig.PIN_LORA_SCLK = PIN_LORA_SCLK;   // LORA SPI CLK
        hwConfig.PIN_LORA_MISO = PIN_LORA_MISO;   // LORA SPI MISO
        hwConfig.PIN_LORA_DIO_1 = PIN_LORA_DIO_1; // LORA DIO_1
        hwConfig.PIN_LORA_BUSY = PIN_LORA_BUSY;   // LORA SPI BUSY
        hwConfig.PIN_LORA_MOSI = PIN_LORA_MOSI;   // LORA SPI MOSI
        hwConfig.RADIO_TXEN = RADIO_TXEN;		  // LORA ANTENNA TX ENABLE
        hwConfig.RADIO_RXEN = RADIO_RXEN;		  // LORA ANTENNA RX ENABLE
        hwConfig.USE_DIO2_ANT_SWITCH = true;	  // Example uses an CircuitRocks Alora RFM1262 which uses DIO2 pins as antenna control
        hwConfig.USE_DIO3_TCXO = true;			  // Example uses an CircuitRocks Alora RFM1262 which uses DIO3 to control oscillator voltage
        hwConfig.USE_DIO3_ANT_SWITCH = false;	  // Only Insight ISP4520 module uses DIO3 as antenna control

        uint8_t deviceId[8];

        BoardGetUniqueId(deviceId);
        ESP_LOGD("custom", "BoardId: %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X",
                 deviceId[0],
                 deviceId[1],
                 deviceId[2],
                 deviceId[3],
                 deviceId[4],
                 deviceId[5],
                 deviceId[6],
                 deviceId[7]);

        // Initialize the LoRa chip
        ESP_LOGD(TAG, "Starting lora_hardware_init");
        lora_hardware_init(hwConfig);

        // Initialize the Radio callbacks
        RadioEvents.TxDone    = NULL;        // OnTxDone;
        RadioEvents.RxDone    = OnRxDone;
        RadioEvents.TxTimeout = NULL;        // OnTxTimeout;
        RadioEvents.RxTimeout = NULL;        // OnRxTimeout;
        RadioEvents.RxError   = NULL;        // OnRxError;
        RadioEvents.CadDone   = NULL;        // OnCadDone;

        // Initialize the Radio
        Radio.Init(&RadioEvents);

        // Set Radio channel
        Radio.SetChannel(RF_FREQUENCY);

        // Set Radio RX configuration
        Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                          LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                          LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                          0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

        // Start LoRa
        ESP_LOGD(TAG, "Starting Radio.Rx");
        Radio.Rx(RX_TIMEOUT_VALUE);
    }

    void loop() override {
        // This will be called very often after setup time.
        // think of it as the loop() call in Arduino

        unsigned long currentMillis = millis();

        // Keep alive message, 1 sec tick
        if(currentMillis - previousMillis > interval)
            {
                previousMillis = currentMillis;
                ESP_LOGD(TAG, "Tick");
            }

        // Programable state change delay
        if (inDelay && (currentMillis - stateMillis > delayMillis)) {
            if (state == "ON"){
                //Inverter switching has been delayed and should be turned on.
                //turnOnRGB(RGB_PENDING,0);
                ESP_LOGD(TAG,"Status is ON after %.2f sec delay", delayMillis / 1000.0);
                digitalWrite(GPIO5,HIGH);
                digitalWrite(GPIO6,HIGH);
            } else if (state == "OFF"){
                ESP_LOGD(TAG,"Status is OFF after %.2f sec delay", delayMillis / 1000.0);
                digitalWrite(GPIO5,LOW);
                digitalWrite(GPIO6,LOW);
            }

            inDelay = 0;
        }

        return;
    }
};
