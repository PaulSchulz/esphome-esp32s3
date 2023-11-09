// Open Energy Sender
// See: https://esphome.io/custom/custom_component.html
#include "esphome.h"

#include "esphome_lora_version.h"

#include <SX126x-Arduino.h>
#include <SPI.h>

// Tag for log output.
static const char* TAG = "sender";

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

static RadioEvents_t RadioEvents;
char txpacket[BUFFER_SIZE];
int16_t rssi,rxSize;

#define PKT_ON  "@+++"
#define PKT_OFF "@---"

//////////////////////////////////////////////////////////////////////////////
#define GPIO5 5 // Input - High On, Low Off
#define GPIO6 6 // Output - Monitor State

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

time_t timeToSend;

time_t cadTime;

////////////////////////////////////////////////////////////////////////
/**@brief Function to be executed on Radio Tx Done event
*/

void OnTxDone(void)
{
    ESP_LOGD(TAG, "OnTxDone");
    Radio.Rx(RX_TIMEOUT_VALUE);
}


/**@brief Function to be executed on Radio Tx Timeout event
*/
void OnTxTimeout(void)
{
    // Radio.Sleep();
    ESP_LOGD(TAG, "OnTxTimeout");
    // digitalWrite(LED_BUILTIN, LOW);

    Radio.Rx(RX_TIMEOUT_VALUE);
}

unsigned long previousMillis = 0UL;
unsigned long interval = 2000UL; // 2sec

class MyCustomComponent : public Component {
public:

    int c_state = 0;
    int p_state = 0;

    void setup() override {
        ESP_LOGD(TAG, "ESPHOME_LORA_VERSION: %s", ESPHOME_LORA_VERSION);

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

        ESP_LOGD(TAG, "SX126x - Send on Signal");

        uint8_t deviceId[8];

        BoardGetUniqueId(deviceId);
        ESP_LOGD(TAG, "BoardId: %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X",
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
        RadioEvents.TxDone    = OnTxDone;
        RadioEvents.RxDone    = NULL; // OnRxDone;
        RadioEvents.TxTimeout = OnTxTimeout;
        RadioEvents.RxTimeout = NULL; // OnRxTimeout;
        RadioEvents.RxError   = NULL; // OnRxError;
        RadioEvents.CadDone   = NULL; // OnCadDone;

        // Initialize the Radio
        Radio.Init(&RadioEvents);

        // Set Radio channel
        Radio.SetChannel(RF_FREQUENCY);

        // Set Radio TX configuration
        ESP_LOGD(TAG, "Starting Radio.Tx");
        Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                          LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                          LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                          true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);

        timeToSend = millis();

        //////////////////////////////////////////////////////////////////////

        //  Serial.begin(115200);
        pinMode(GPIO5, INPUT);
        pinMode(GPIO6, OUTPUT);
        rssi=0;
    }

    void loop() override {
        // This will be called very often after setup time.
        // think of it as the loop() call in Arduino

        unsigned long currentMillis = millis();

        if(currentMillis - previousMillis > interval)
        {
            /* The Arduino executes this code once every 2 seconds
             *  (interval = 2000 (ms) = 2 seconds).
             */

            // Don't forget to update the previousMillis value
            previousMillis = currentMillis;
            ESP_LOGD(TAG, "Tick");

            // bool cadResult = 0;
            // OnCadDone(cadResult);

            //////////////////////////////////////////////////////////////////////
            // return;

            int val = digitalRead(GPIO5);
            //Serial.print("Pin Value: ");
            //Serial.println(val);
            if(val == 0){
                p_state = c_state;
                c_state = 1;
                digitalWrite(GPIO6,HIGH);
            } else {
                p_state = c_state;
                c_state = 0;
                digitalWrite(GPIO6,LOW);
            }

            // If state has changes, repeat transmission 15 times.
            if(p_state != c_state && c_state == 1 ){
                for(int i = 0; i<15; i++){
                    ESP_LOGD(TAG,"Pin Value: %i", val);
                    ESP_LOGD(TAG,"Sending packet: OFF");
                    sprintf(txpacket,"%s", PKT_OFF);
                    Radio.Send( (uint8_t *)txpacket, strlen(txpacket) ); //send the package out
                }
            }
            else if(p_state != c_state && c_state == 0){
                for(int i = 0; i<15; i++){
                    ESP_LOGD(TAG, "Pin Value: %i", val);
                    ESP_LOGD(TAG, "Sending packet: ON");
                    sprintf(txpacket,"%s",PKT_ON);
                    Radio.Send( (uint8_t *)txpacket, strlen(txpacket) ); //send the package out
                }
            } else {
                if(c_state == 1){
                    ESP_LOGD(TAG, "Pin Value: %d", val);
                    ESP_LOGD(TAG, "Sending packet: OFF");
                    sprintf(txpacket,"%s", PKT_OFF);
                    Radio.Send( (uint8_t *)txpacket, strlen(txpacket) ); //send the package out

                }
                else if(c_state == 0){
                    ESP_LOGD(TAG, "Pin Value: %d", val);
                    ESP_LOGD(TAG, "Sending packet: ON");
                    sprintf(txpacket,"%s", PKT_ON);
                    Radio.Send( (uint8_t *)txpacket, strlen(txpacket) ); //send the package out
                }
            }
        }

    }

};
