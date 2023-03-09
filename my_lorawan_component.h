// My Custome Component
// See: https://esphome.io/custom/custom_component.html
#include "esphome.h"

//#include <SX126x-Arduino.h>
#include <LoRaWan-Arduino.h>
#include <SPI.h>

#include "lorawan_secrets.h"

// LoRaWan setup definitions
#define SCHED_MAX_EVENT_DATA_SIZE APP_TIMER_SCHED_EVENT_DATA_SIZE // Maximum size of scheduler events
#define SCHED_QUEUE_SIZE 60	// Maximum number of events in the scheduler queue

/**< Maximum number of events in the scheduler queue  */
#define LORAWAN_APP_DATA_BUFF_SIZE 256 // Size of the data to be transmitted
#define LORAWAN_APP_TX_DUTYCYCLE 5000 // Defines the application data transmission duty cycle. 30s, value in [ms]
#define APP_TX_DUTYCYCLE_RND 1000 // Defines a random delay for application data transmission duty cycle. 1s, value in [ms]
#define JOINREQ_NBTRIALS 3

bool doOTAA = true;
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

// Define Helium or TTN OTAA keys. All msb (big endian).
// These are stored is 'secret.h'
uint8_t nodeDeviceEUI[8] = NODEDEVICEEUI;
uint8_t nodeAppEUI[8]    = NODEAPPEUI;
uint8_t nodeAppKey[16]   = NODEAPPKEY;

// Foward declaration
/** LoRaWAN callback when join network finished */
static void lorawan_has_joined_handler(void);
/** LoRaWAN callback when join network failed */
static void lorawan_join_fail_handler(void);
/** LoRaWAN callback when data arrived */
static void lorawan_rx_handler(lmh_app_data_t *app_data);
/** LoRaWAN callback after class change request finished */
static void lorawan_confirm_class_handler(DeviceClass_t Class);
/** LoRaWAN callback after class change request finished */
static void lorawan_unconfirm_tx_finished(void);
/** LoRaWAN callback after class change request finished */
static void lorawan_confirm_tx_finished(bool result);
/** LoRaWAN Function to send a package */
static void send_lora_frame(void);
static uint32_t timers_init(void);

// APP_TIMER_DEF(lora_tx_timer_id);	 // LoRa tranfer timer instance.
TimerEvent_t appTimer;	 // LoRa tranfer timer instance.
static uint8_t m_lora_app_data_buffer[LORAWAN_APP_DATA_BUFF_SIZE]; // Lora user application data buffer.
static lmh_app_data_t m_lora_app_data = {m_lora_app_data_buffer, 0, 0, 0, 0};	 // Lora user application data structure.

/**@brief Structure containing LoRaWan parameters, needed for lmh_init()
 */
static lmh_param_t lora_param_init = {LORAWAN_ADR_OFF, DR_3, LORAWAN_PUBLIC_NETWORK,
										JOINREQ_NBTRIALS, LORAWAN_DEFAULT_TX_POWER, LORAWAN_DUTYCYCLE_OFF};

static lmh_callback_t lora_callbacks = {BoardGetBatteryLevel, BoardGetUniqueId, BoardGetRandomSeed,
										lorawan_rx_handler, lorawan_has_joined_handler,
										lorawan_confirm_class_handler, lorawan_join_fail_handler,
										lorawan_unconfirm_tx_finished, lorawan_confirm_tx_finished};

class MyCustomComponent : public Component {
public:

    void setup() override {
        ESP_LOGD("lorawan", "Setup SPI LoRa");

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


        ESP_LOGD("lorawan", "Configure LoRaWAN");
        uint32_t err_code = lora_hardware_init(hwConfig);
        if (err_code != 0)
        {
            ESP_LOGD("lorawan", "lora_hardware_init failed - %d", err_code);
        }

        // Initialize Scheduler and timer (Must be after lora_hardware_init)
        err_code = timers_init();
        if (err_code != 0)
        {
            ESP_LOGD("lorawan", "timers_init failed - %d", err_code);
        }

        // Setup the EUIs and Keys
        lmh_setDevEui(nodeDeviceEUI);
        lmh_setAppEui(nodeAppEUI);
        lmh_setAppKey(nodeAppKey);

        // Initialize LoRaWan
        // CLASS C works for esp32 and e22, US915 region works in america, other local frequencies can be found
        // here https://docs.helium.com/lorawan-on-helium/frequency-plans/

        err_code = lmh_init(&lora_callbacks, lora_param_init, doOTAA, CLASS_C, LORAMAC_REGION_AU915);
        if (err_code != 0)
        {
            ESP_LOGD("lora", "lmh_init failed - %d", err_code);
        }

        // For Helium and US915, you need as well to select subband 2 after you called lmh_init(),
        // For US816 you need to use subband 1. Other subbands configurations can be found in
        // https://github.com/beegee-tokyo/SX126x-Arduino/blob/1c28c6e769cca2b7d699a773e737123fc74c47c7/src/mac/LoRaMacHelper.cpp

        // lmh_setSubBandChannels(2);

        // Start Join procedure
        lmh_join();
    }

    void loop() override {
        // This will be called very often after setup time.
        // think of it as the loop() call in Arduino
    }

};


static void lorawan_join_fail_handler(void)
{
	ESP_LOGD("lorawan", "OTAA joined failed");
    ESP_LOGD("lorawan", "Check LPWAN credentials and if a gateway is in range");
	// Restart Join procedure
    ESP_LOGD("lorawan","Restart network join request");
}

/**@brief LoRa function for handling HasJoined event.
 */
static void lorawan_has_joined_handler(void)
{
	ESP_LOGD("lorawan", "Network Joined");

	// lmh_class_request(CLASS_A);
    lmh_class_request(CLASS_C);

    TimerSetValue(&appTimer, LORAWAN_APP_TX_DUTYCYCLE);
	TimerStart(&appTimer);
	// app_timer_start(lora_tx_timer_id, APP_TIMER_TICKS(LORAWAN_APP_TX_DUTYCYCLE), NULL);
	ESP_LOGD("lorawan", "Sending frame");
    send_lora_frame();
}

/**@brief Function for handling LoRaWan received data from Gateway
 *
 * @param[in] app_data  Pointer to rx data
 */
static void lorawan_rx_handler(lmh_app_data_t *app_data)
{
	ESP_LOGD("lorawan", "LoRa Packet received on port %d, size:%d, rssi:%d, snr:%d",
             app_data->port, app_data->buffsize, app_data->rssi, app_data->snr);

    // TODO: Fix debug output for rx packet
	for (int i = 0; i < app_data->buffsize; i++)
	{
		//ESP_LOGD("lorawan", "%0X ", app_data->buffer[i]);
    }
	// Serial.println("");

	switch (app_data->port)
	{
	case 3:
		// Port 3 switches the class
		if (app_data->buffsize == 1)
		{
			switch (app_data->buffer[0])
			{
			case 0:
				lmh_class_request(CLASS_A);
				break;

			case 1:
				lmh_class_request(CLASS_B);
				break;

			case 2:
				lmh_class_request(CLASS_C);
				break;

			default:
				break;
			}
		}
		break;

	case LORAWAN_APP_PORT:
		// YOUR_JOB: Take action on received data
		break;

	default:
		break;
	}
}

/**@brief Function to confirm LORaWan class switch.
 *
 * @param[in] Class  New device class
 */
static void lorawan_confirm_class_handler(DeviceClass_t Class)
{
    ESP_LOGD("lorawan", "switch to class %c done", "ABC"[Class]);

    // Informs the server that switch has occurred ASAP
	m_lora_app_data.buffsize = 0;
	m_lora_app_data.port = LORAWAN_APP_PORT;
	lmh_send(&m_lora_app_data, LMH_UNCONFIRMED_MSG);
}

/**
 * @brief Called after unconfirmed packet was sent
 *
 */
static void lorawan_unconfirm_tx_finished(void)
{
    ESP_LOGD("lorawan", "Uncomfirmed TX finished");
}

/**
 * @brief Called after confirmed packet was sent
 * @param result Result of sending true = ACK received false = No ACK
 */
static void lorawan_confirm_tx_finished(bool result)
{
    ESP_LOGD("lorawan", "Comfirmed TX finished with result %s", result ? "ACK" : "NAK");
}

/**@brief Function for sending a LoRa package.
 */
static void send_lora_frame(void)
{

	// Building the message to send
	int32_t chipTemp = 0;
	uint32_t i = 0;
	Ticker ledTicker;

	if (lmh_join_status_get() != LMH_SET)
	{
		// Not joined, try again later
		ESP_LOGD("lorawan", "Did not join network, skip sending frame");
		return;
	}

	// Building the message to send

	char t100 = (char)(chipTemp / 100);
	char t10 = (char)((chipTemp - (t100 * 100)) / 10);
	char t1 = (char)((chipTemp - (t100 * 100) - (t10 * 10)) / 1);

	// Buffer contruction
	m_lora_app_data.port = LORAWAN_APP_PORT;

	m_lora_app_data.buffer[i++] = '{';
	m_lora_app_data.buffer[i++] = '"';
	m_lora_app_data.buffer[i++] = 'i';
	m_lora_app_data.buffer[i++] = '"';
	m_lora_app_data.buffer[i++] = ':';
	m_lora_app_data.buffer[i++] = ',';
	m_lora_app_data.buffer[i++] = '"';
	m_lora_app_data.buffer[i++] = 'n';
	m_lora_app_data.buffer[i++] = '"';
	m_lora_app_data.buffer[i++] = ':';

	m_lora_app_data.buffer[i++] = t100 + 0x30;
	m_lora_app_data.buffer[i++] = t10 + 0x30;
	m_lora_app_data.buffer[i++] = t1 + 0x30;
	m_lora_app_data.buffer[i++] = '}';
	m_lora_app_data.buffsize = i;

    // TODO: Fix debug data output
	//Serial.print("Data: ");
	//Serial.println((char *)m_lora_app_data.buffer);
	//Serial.print("Size: ");
	//Serial.println(m_lora_app_data.buffsize);
	//Serial.print("Port: ");
	//Serial.println(m_lora_app_data.port);

	chipTemp += 1;
	if (chipTemp >= 999)
		chipTemp = 0;

	lmh_error_status error = lmh_send(&m_lora_app_data, LMH_UNCONFIRMED_MSG);
	if (error == LMH_SUCCESS)
	{
	}

    ESP_LOGD("lorawan", "lmh_send result %d", error);
    // digitalWrite(LED_BUILTIN, LED_ON);
	// ledTicker.once(1, ledOff);
}

/**@brief Function for handling a LoRa tx timer timeout event.
 */
static void tx_lora_periodic_handler(void)
{
	TimerSetValue(&appTimer, LORAWAN_APP_TX_DUTYCYCLE);
	TimerStart(&appTimer);
	ESP_LOGD("lorawan", "Sending frame");
    send_lora_frame();
}

static uint32_t timers_init(void)
{
	appTimer.timerNum = 3;
	TimerInit(&appTimer, tx_lora_periodic_handler);
	return 0;
}
