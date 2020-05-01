#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "boards.h"
#include "app_timer.h"
#include "app_button.h"
#include "ble_lbs.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "bsp.h"
#include "nrf_delay.h"
#include "app_uart.h"
#include "nrf_drv_saadc.h"
#include "oled.h"
#include "OLEDFONT.h"

#include "nrf_drv_twi.h"
#include "mpu6050.h"

#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif

#define ADVERTISING_LED                 BSP_BOARD_LED_0                         /**< Is on when device is advertising. */
#define CONNECTED_LED                   BSP_BOARD_LED_1                         /**< Is on when device has connected. */
#define LEDBUTTON_LED                   BSP_BOARD_LED_2                         /**< LED to be toggled with the help of the LED Button Service. */
#define LEDBUTTON_BUTTON                BSP_BUTTON_0                            /**< Button that will trigger the notification event with the LED Button Service */

#define DEVICE_NAME                     "IK-52832DK TEST"                         /**< Name of device. Will be included in the advertising data. */

#define APP_BLE_OBSERVER_PRIO           3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG            1                                       /**< A tag identifying the SoftDevice BLE configuration. */

#define APP_ADV_INTERVAL                128                                      /**< The advertising interval (in units of 0.625 ms; this value corresponds to 40 ms). */
#define APP_ADV_DURATION                BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED   /**< The advertising time-out (in units of seconds). When set to 0, we will never time out. */


#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (1 second). */
#define SLAVE_LATENCY                   0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory time-out (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(20000)                  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (15 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000)                   /**< Time between each call to sd_ble_gap_conn_param_update after the first call (5 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50)                     /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */

#define DEAD_BEEF                       0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define UART_TX_BUF_SIZE                256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256                                         /**< UART RX buffer size. */

#define  BEEP_PIN   12
#define  TOUCH_PIN  3


BLE_LBS_DEF(m_lbs);                                                             /**< LED Button Service instance. */
NRF_BLE_GATT_DEF(m_gatt);                                                       /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                         /**< Context for the Queued Write module.*/


APP_TIMER_DEF(beep_timer_id);
APP_TIMER_DEF(saadc_timer_id);
APP_TIMER_DEF(mpu6050_timer_id);
#define BEEP_INTERVAL             APP_TIMER_TICKS(500) 
#define SAADC_INTERVAL            APP_TIMER_TICKS(1000)
#define MPU6050_INTERVAL          APP_TIMER_TICKS(800)
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;                        /**< Handle of the current connection. */
static uint8_t m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;                   /**< Advertising handle used to identify an advertising set. */
static uint8_t m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];                    /**< Buffer for storing an encoded advertising set. */
static uint8_t m_enc_scan_response_data[BLE_GAP_ADV_SET_DATA_SIZE_MAX];         /**< Buffer for storing an encoded scan data. */

static bool SAADC_START_FLAG =false;
static bool MPU6050_START_FLAG =false;
/**@brief Struct that contains pointers to the encoded advertising data. */
static ble_gap_adv_data_t m_adv_data =
{
    .adv_data =
    {
        .p_data = m_enc_advdata,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =
    {
        .p_data = m_enc_scan_response_data,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX

    }
};

/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void)
{
    ret_code_t    err_code;
    ble_advdata_t advdata;
    ble_advdata_t srdata;

    ble_uuid_t adv_uuids[] = {{LBS_UUID_SERVICE, m_lbs.uuid_type}};

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance = true;
    advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;


    memset(&srdata, 0, sizeof(srdata));
    srdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
    srdata.uuids_complete.p_uuids  = adv_uuids;

    err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
    APP_ERROR_CHECK(err_code);

    err_code = ble_advdata_encode(&srdata, m_adv_data.scan_rsp_data.p_data, &m_adv_data.scan_rsp_data.len);
    APP_ERROR_CHECK(err_code);

    ble_gap_adv_params_t adv_params;

    // Set advertising parameters.
    memset(&adv_params, 0, sizeof(adv_params));

    adv_params.primary_phy     = BLE_GAP_PHY_1MBPS;
    adv_params.duration        = APP_ADV_DURATION;
    adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
    adv_params.p_peer_addr     = NULL;
    adv_params.filter_policy   = BLE_GAP_ADV_FP_ANY;
    adv_params.interval        = APP_ADV_INTERVAL;

    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &adv_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for handling write events to the LED characteristic.
 *
 * @param[in] p_lbs     Instance of LED Button Service to which the write applies.
 * @param[in] led_state Written/desired state of the LED.
 */
static void led_write_handler(uint16_t conn_handle, ble_lbs_t * p_lbs, uint8_t led_state)
{
    if (led_state)
    {
        bsp_board_led_on(LEDBUTTON_LED);
        NRF_LOG_INFO("Received LED ON!");
    }
    else
    {
        bsp_board_led_off(LEDBUTTON_LED);
        NRF_LOG_INFO("Received LED OFF!");
    }
}
/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    ret_code_t         err_code;
    ble_lbs_init_t     init     = {0};
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Initialize LBS.
    init.led_write_handler = led_write_handler;

    err_code = ble_lbs_init(&m_lbs, &init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module that
 *          are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply
 *       setting the disconnect_on_fail config parameter, but instead we use the event
 *       handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    ret_code_t           err_code;

    err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
    APP_ERROR_CHECK(err_code);

    bsp_board_led_on(ADVERTISING_LED);
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected");
            bsp_board_led_on(CONNECTED_LED);
            bsp_board_led_off(ADVERTISING_LED);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            err_code = app_button_enable();
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected");
            bsp_board_led_off(CONNECTED_LED);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            //err_code = app_button_disable();
            APP_ERROR_CHECK(err_code);
            advertising_start();
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle,
                                                   BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP,
                                                   NULL,
                                                   NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}



//串口事件回调函数
void uart_event_handle(app_uart_evt_t * p_event)
{
	  uint8_t cr;
    ret_code_t err_code;
    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
					  UNUSED_VARIABLE(app_uart_get(&cr));
            printf("%c",cr);
				    switch(cr)
						{
							case '1': //串口接收到字符1，测试电位器电压
                if(SAADC_START_FLAG == false)
								{
									err_code = app_timer_start(saadc_timer_id, SAADC_INTERVAL, NULL);
                  APP_ERROR_CHECK(err_code);
							    SAADC_START_FLAG = true;
								}
							  
							  if(MPU6050_START_FLAG == true)
								{
									err_code = app_timer_stop(mpu6050_timer_id);
                  APP_ERROR_CHECK(err_code);
									MPU6050_START_FLAG = false;
								}							  
							  break;
							
							case '2': //串口接收到字符2，OLED显示一副图片
                if(MPU6050_START_FLAG == true)
								{
									err_code = app_timer_stop(mpu6050_timer_id);
                  APP_ERROR_CHECK(err_code);
									MPU6050_START_FLAG = false;
								}	
							  twi_uninit();
							  nrf_delay_ms(50);
							  OLED_Init();   //显示图形
							  OLED_Fill(0x00);     //清屏
                OLED_DrawBMP(0,0,128,8,BMP1);//显示图形
							  break;
							
							case '3': //串口接收到字符3，测试MPU6050
                if(SAADC_START_FLAG == true)
								{
									err_code = app_timer_stop(saadc_timer_id);
                  APP_ERROR_CHECK(err_code);
									SAADC_START_FLAG = false;
								}
							  if(MPU6050_START_FLAG == false)
								{
									twi_master_init();
									if(mpu6050_init(0x68) == true)//初始化MPU6050;
								  {
									   err_code = app_timer_start(mpu6050_timer_id, MPU6050_INTERVAL, NULL);
                     APP_ERROR_CHECK(err_code);
										 MPU6050_START_FLAG = true;
								  }
								  else  //初始化不成功输出提示信息，同时反初始化I2C
								  {
									   printf("\r\nMPU6050 is not exist!");
										 twi_uninit();
								  }
								}
							  break;
							
							default:
                break;
						}
            break;

        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;

        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;

        default:
            break;
    }
}
//串口初始化
static void uart_init(void)
{
    uint32_t                     err_code;
    app_uart_comm_params_t const comm_params =
    {
        .rx_pin_no    = RX_PIN_NUMBER,
        .tx_pin_no    = TX_PIN_NUMBER,
        .rts_pin_no   = RTS_PIN_NUMBER,
        .cts_pin_no   = CTS_PIN_NUMBER,
        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,
        .use_parity   = false,
#if defined (UART_PRESENT)
        .baud_rate    = NRF_UART_BAUDRATE_115200
#else
        .baud_rate    = NRF_UARTE_BAUDRATE_115200
#endif
    };

    APP_UART_FIFO_INIT(&comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       uart_event_handle,
                       APP_IRQ_PRIORITY_LOWEST,
                       err_code);
    APP_ERROR_CHECK(err_code);
}

static void bsp_configuration(void)
{

    uint32_t err_code = bsp_init(BSP_INIT_LEDS, NULL);
    APP_ERROR_CHECK(err_code);

}

static void beep_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
	  nrf_gpio_pin_clear(BEEP_PIN);
}
static void saadc_timeout_handler(void * p_context)
{
    nrf_saadc_value_t  saadc_val;
	
	  UNUSED_PARAMETER(p_context);
	
	  //启动一次ADC采样（阻塞模式）。
		 nrfx_saadc_sample_convert(0,&saadc_val);
		 //串口输出ADC采样值。
		 printf("Sample value is:  %d\r\n", saadc_val);
		 
		 //串口输出采样值计算得到的电压值。电压值 = 采样值 * 3.6 /1024
		 printf("Voltage = %.3fV\r\n", saadc_val * 3.6 /1024);
}

static void mpu6050_timeout_handler(void * p_context)
{
    int16_t AccValue[3],GyroValue[3];
  
	  UNUSED_PARAMETER(p_context);

	  MPU6050_ReadAcc( &AccValue[0], &AccValue[1] , &AccValue[2] );
	  MPU6050_ReadGyro(&GyroValue[0] , &GyroValue[1] , &GyroValue[2] );
	  //串口输出MPU6050基础数据
	  printf("ACC:  %d	%d	%d	",AccValue[0],AccValue[1],AccValue[2]);
	  printf("GYRO: %d	%d	%d	\r\n",GyroValue[0],GyroValue[1],GyroValue[2]);
}


static void timers_init(void)
{
    //初始化APP定时器
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
	
	  err_code = app_timer_create(&beep_timer_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                beep_timeout_handler);
    APP_ERROR_CHECK(err_code);
	
	  err_code = app_timer_create(&saadc_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                saadc_timeout_handler);
    APP_ERROR_CHECK(err_code);
	
	  err_code = app_timer_create(&mpu6050_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                mpu6050_timeout_handler);
    APP_ERROR_CHECK(err_code);
}
//SAADC事件回调函数，因为是堵塞模式，所以不需要事件，这里定义了一个空的事件回调函数
void saadc_callback(nrf_drv_saadc_evt_t const * p_event){}
//初始化SAADC，配置使用的SAADC通道的参数
void saadc_init(void)
{
    ret_code_t err_code;
	  //定义ADC通道配置结构体，并使用单端采样配置宏初始化，
	  //NRF_SAADC_INPUT_AIN2是使用的模拟输入通道
    nrf_saadc_channel_config_t channel_config = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN2);
    //初始化SAADC，注册事件回调函数。
	  err_code = nrf_drv_saadc_init(NULL, saadc_callback);
    APP_ERROR_CHECK(err_code);
    //初始化SAADC通道0
    err_code = nrfx_saadc_channel_init(0, &channel_config);
    APP_ERROR_CHECK(err_code);

}
void button_event_handler(uint8_t pin_no, uint8_t button_action)
{
    ret_code_t err_code;
	  switch (pin_no)
    {
        case BUTTON_1:
           if(button_action == APP_BUTTON_PUSH)
					 {
						 nrf_gpio_pin_toggle(LED_1);
					 }
           break;
        case BUTTON_2:
					 if(button_action == APP_BUTTON_PUSH)
					 {
						 nrf_gpio_pin_toggle(LED_2);
					 }
           break;
        case BUTTON_3:
           if(button_action == APP_BUTTON_PUSH)
					 {
						 nrf_gpio_pin_toggle(LED_3);
					 }
           break;
				case BUTTON_4:
           if(button_action == APP_BUTTON_PUSH)
					 {
						 nrf_gpio_pin_toggle(LED_4);
					 }
           break;
				case TOUCH_PIN:
				   nrf_gpio_pin_set(BEEP_PIN);
				   err_code = app_timer_start(beep_timer_id, BEEP_INTERVAL, NULL);
           APP_ERROR_CHECK(err_code);
           break;

        default:
            return; 
    }
}
static void buttons_init(void)
{
    ret_code_t err_code;

    //The array must be static because a pointer to it will be saved in the button handler module.
    static app_button_cfg_t buttons[] =
    {
        {BUTTON_1, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, button_event_handler},
				{BUTTON_2, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, button_event_handler},
				{BUTTON_3, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, button_event_handler},
				{BUTTON_4, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, button_event_handler},
				{TOUCH_PIN, APP_BUTTON_ACTIVE_HIGH, BUTTON_PULL, button_event_handler}
    };

    err_code = app_button_init(buttons, ARRAY_SIZE(buttons),
                               BUTTON_DETECTION_DELAY);
    APP_ERROR_CHECK(err_code);
		
		
		err_code = app_button_enable();
            APP_ERROR_CHECK(err_code);
}
/**@brief Function for application main entry.
 */
int main(void)
{
	  uint32_t err_code;
	  //定义地址结构体变量my_addr
	  static ble_gap_addr_t  my_addr;
	
	  nrf_gpio_cfg_output(BEEP_PIN);
	  nrf_gpio_pin_clear(BEEP_PIN);
	
	  timers_init();
	  bsp_configuration();
	  buttons_init();
	  uart_init();
	  
    //初始化SAADC
	  saadc_init(); 

    ble_stack_init();
    gap_params_init();
    gatt_init();
    services_init();
    advertising_init();
    conn_params_init();
    printf("QITAS: nRF52832 TEST START!\r\n");
		
		/*---------------------------设置设备地址:随机静态地址-----------------------*/
		//初始化地址和地址类型
		my_addr.addr[0] = 0x11;
		my_addr.addr[1] = 0x22;
		my_addr.addr[2] = 0x33;
		my_addr.addr[3] = 0x44;
		my_addr.addr[4] = 0x55;
		my_addr.addr[5] = 0xcc;//注意地址最高两个位必须为1，其它所有的位不能同时为0，也不能同时为1
		my_addr.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;//地址类型设置为随机静态地址
		//写入地址
		err_code = sd_ble_gap_addr_set(&my_addr);//
		if(err_code != NRF_SUCCESS)
		{
			//打印设置地址失败
			NRF_LOG_INFO("Set Address Failed!");
		}
		/*---------------------------------END---------------------------------------*/
		
    advertising_start();
    
    // Enter main loop.
    for (;;)
    {
			
    }
}



