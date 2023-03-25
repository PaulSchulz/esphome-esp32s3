// My Custome Component
// See: https://esphome.io/custom/custom_component.html
#include "esphome.h"

#include <SX126x-Arduino.h>
#include <SPI.h>

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
static uint16_t BufferSize = BUFFER_SIZE;
static uint8_t RcvBuffer[BUFFER_SIZE];
static uint8_t TxdBuffer[BUFFER_SIZE];
static bool isMaster = true;
const uint8_t PingMsg[] = "PING";
const uint8_t PongMsg[] = "PONG";

hw_config hwConfig;

// Heltec Wifi LoRa 32 (V3) - SX126x pin configuration
int PIN_LORA_RESET = 12;  // LORA RESET
int PIN_LORA_DIO_1 = 14; // LORA DIO_1
int PIN_LORA_BUSY  = 13;  // LORA SPI BUSY
int PIN_LORA_NSS   =  8;	// LORA SPI CS
int PIN_LORA_SCLK  =  9;  // LORA SPI CLK
int PIN_LORA_MISO  = 11;  // LORA SPI MISO
int PIN_LORA_MOSI  = 10;  // LORA SPI MOSI
int RADIO_TXEN = -1;	 // LORA ANTENNA TX ENABLE
int RADIO_RXEN = -1;	 // LORA ANTENNA RX ENABLE

time_t timeToSend;

time_t cadTime;

uint8_t pingCnt = 0;
uint8_t pongCnt = 0;

////////////////////////////////////////////////////////////////////////
/**@brief Function to be executed on Radio Tx Done event
*/

void OnTxDone(void)
{
    ESP_LOGD("custom", "OnTxDone");
    Radio.Rx(RX_TIMEOUT_VALUE);
}


/**@brief Function to be executed on Radio Rx Done event
*/
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
    ESP_LOGD("custom", "OnRxDone");
    delay(10);
    BufferSize = size;
    memcpy(RcvBuffer, payload, BufferSize);

    ESP_LOGD("custom", "RssiValue=%d dBm, SnrValue=%d", rssi, snr);

    for (int idx = 0; idx < size; idx++)
    {
        // Serial.printf("%02X ", RcvBuffer[idx]);
    }
    // Serial.println("");

    // digitalWrite(LED_BUILTIN, HIGH);

    if (isMaster == true)
    {
        if (BufferSize > 0)
        {
            if (strncmp((const char *)RcvBuffer, (const char *)PongMsg, 4) == 0)
            {
                ESP_LOGD("custom", "Received a PONG in OnRxDone as Master");

                // Wait 500ms before sending the next package
                delay(500);

                // Check if our channel is available for sending
                Radio.Standby();
                Radio.SetCadParams(LORA_CAD_08_SYMBOL, LORA_SPREADING_FACTOR + 13, 10, LORA_CAD_ONLY, 0);
                cadTime = millis();
                Radio.StartCad();
                // Sending next Ping will be started when the channel is free
            }
            else if (strncmp((const char *)RcvBuffer, (const char *)PingMsg, 4) == 0)
            { // A master already exists then become a slave
                ESP_LOGD("custom", "Received a PING in OnRxDone as Master");

                isMaster = false;
                Radio.Rx(RX_TIMEOUT_VALUE);
            }
            else // valid reception but neither a PING or a PONG message
            {	// Set device as master and start again
                isMaster = true;
                Radio.Rx(RX_TIMEOUT_VALUE);
            }
        }
    }
    else
    {
        if (BufferSize > 0)
        {
            if (strncmp((const char *)RcvBuffer, (const char *)PingMsg, 4) == 0)
            {
                ESP_LOGD("custom", "Received a PING in OnRxDone as Slave");

                // Check if our channel is available for sending
                Radio.Standby();
                Radio.SetCadParams(LORA_CAD_08_SYMBOL,
                                   LORA_SPREADING_FACTOR + 13,
                                   10,
                                   LORA_CAD_ONLY,
                                   0);
                cadTime = millis();
                Radio.StartCad();
                // Sending Pong will be started when the channel is free
            }
            else // valid reception but not a PING as expected
            {	// Set device as master and start again
                ESP_LOGD("custom", "Received something in OnRxDone as Slave");

                isMaster = true;
                Radio.Rx(RX_TIMEOUT_VALUE);
            }
        }
    }
}

/**@brief Function to be executed on Radio Tx Timeout event
*/
void OnTxTimeout(void)
{
    // Radio.Sleep();
    ESP_LOGD("custom", "OnTxTimeout");
    // digitalWrite(LED_BUILTIN, LOW);

    Radio.Rx(RX_TIMEOUT_VALUE);
}

/**@brief Function to be executed on Radio Rx Timeout event
*/
void OnRxTimeout(void)
{
    ESP_LOGD("custom", "OnRxTimeout");

    // digitalWrite(LED_BUILTIN, LOW);

    if (isMaster == true)
    {
        // Wait 500ms before sending the next package
        delay(500);

        // Check if our channel is available for sending
        Radio.Standby();
        Radio.SetCadParams(LORA_CAD_08_SYMBOL, LORA_SPREADING_FACTOR + 13, 10, LORA_CAD_ONLY, 0);
        cadTime = millis();
        Radio.StartCad();
        // Sending the ping will be started when the channel is free
    }
    else
    {
        // No Ping received within timeout, switch to Master
        isMaster = true;
        // Check if our channel is available for sending
        Radio.Standby();
        Radio.SetCadParams(LORA_CAD_08_SYMBOL, LORA_SPREADING_FACTOR + 13, 10, LORA_CAD_ONLY, 0);
        cadTime = millis();
        Radio.StartCad();
        // Sending the ping will be started when the channel is free
    }
}

/**@brief Function to be executed on Radio Rx Error event
*/
void OnRxError(void)
{
    ESP_LOGD("custom", "OnRxError");

    // digitalWrite(LED_BUILTIN, LOW);

    if (isMaster == true)
    {
        // Wait 500ms before sending the next package
                delay(500);

                // Check if our channel is available for sending
                Radio.Standby();
                Radio.SetCadParams(LORA_CAD_08_SYMBOL, LORA_SPREADING_FACTOR + 13, 10, LORA_CAD_ONLY, 0);
                cadTime = millis();
                Radio.StartCad();
                // Sending the ping will be started when the channel is free
            }
            else
            {
                Radio.Rx(RX_TIMEOUT_VALUE);
            }
        }

    /**@brief Function to be executed on Radio Rx Error event
     */
    void OnCadDone(bool cadResult)
        {
            time_t duration = millis() - cadTime;
            if (cadResult)
            {
                ESP_LOGD("custom", "CAD returned channel busy after %ldms", duration);

                Radio.Rx(RX_TIMEOUT_VALUE);
            }
            else
            {
                ESP_LOGD("custom", "CAD returned channel free after %ldms", duration);

                if (isMaster)
                {
                    ESP_LOGD("custom", "Sending a PING in OnCadDone as Master");

                    // Send the next PING frame
                    TxdBuffer[0] = 'P';
                    TxdBuffer[1] = 'I';
                    TxdBuffer[2] = 'N';
                    TxdBuffer[3] = 'G';
                }
                else
                {
                    ESP_LOGD("custom", "Sending a PONG in OnCadDone as Slave");
                    // Serial.println("Sending a PONG in OnCadDone as Slave");

                    // Send the reply to the PONG string
                    TxdBuffer[0] = 'P';
                    TxdBuffer[1] = 'O';
                    TxdBuffer[2] = 'N';
                    TxdBuffer[3] = 'G';
                }
                // We fill the buffer with numbers for the payload
                for (int i = 4; i < BufferSize; i++)
                {
                    TxdBuffer[i] = i - 4;
                }

                Radio.Send(TxdBuffer, BufferSize);
            }
        }

unsigned long previousMillis = 0UL;
unsigned long interval = 10000UL;

class MyCustomComponent : public Component {
public:

    void setup() override {
        // You can also log messages
        ESP_LOGD("custom", "Setup Custom Timer (10s interval)");
        ESP_LOGD("custom", "Setup SPI LoRa");

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

        ESP_LOGD("custom", "SX126x PingPong test");

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
        ESP_LOGD("custom", "Starting lora_hardware_init");
        lora_hardware_init(hwConfig);

        // Initialize the Radio callbacks
        RadioEvents.TxDone    = OnTxDone;
        RadioEvents.RxDone    = OnRxDone;
        RadioEvents.TxTimeout = OnTxTimeout;
        RadioEvents.RxTimeout = OnRxTimeout;
        RadioEvents.RxError   = OnRxError;
        RadioEvents.CadDone   = OnCadDone;

        // Initialize the Radio
        Radio.Init(&RadioEvents);

        // Set Radio channel
        Radio.SetChannel(RF_FREQUENCY);

        // Set Radio TX configuration
        Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                          LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                          LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                          true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);

        // Set Radio RX configuration
        Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                          LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                          LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                          0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

        // Start LoRa
        ESP_LOGD("custom", "Starting Radio.Rx");
        Radio.Rx(RX_TIMEOUT_VALUE);

        timeToSend = millis();

    }

    void loop() override {
        // This will be called very often after setup time.
        // think of it as the loop() call in Arduino

        unsigned long currentMillis = millis();

        if(currentMillis - previousMillis > interval)
        {
            /* The Arduino executes this code once every second
             *  (interval = 1000 (ms) = 1 second).
             */

            // Don't forget to update the previousMillis value
            previousMillis = currentMillis;
            ESP_LOGD("custom", "Tick");

            bool cadResult = 0;
            OnCadDone(cadResult);
        }
}


};
