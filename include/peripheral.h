#ifndef SRC_PERIPHERAL_H_
#define SRC_PERIPHERAL_H_


// -- Defines -- //
// Main Task Events.
#define MAIN_TASK_INIT_EVENT 0x01
#define MAIN_TASK_TIMEOUT_EVENT 0x02
#define MAIN_TASK_WRITERSP_EVENT 0x04

// ADV parameters.
#define DEFAULT_ADVERTISING_INTERVAL    80 // in multiples of 625us.

// TASK intervals.
#define MAIN_TASK_ADV_TIMEOUT 48000 // also in multiples of 625us. 1600 is 1 second, 4800 is 30 secs.

// connection parameters.
// Minimum connection interval (units of 1.25ms, 2=2.5ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL    2
// Maximum connection interval (units of 1.25ms, 12=15ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL    12
// Slave latency to use if automatic parameter update request is enabled
#define DEFAULT_DESIRED_SLAVE_LATENCY        0
// Supervision timeout value (units of 10ms, 1000=10s) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_CONN_TIMEOUT         1000
// Whether to enable automatic parameter update request when a connection is formed
#define DEFAULT_ENABLE_UPDATE_REQUEST        TRUE
// Connection Pause Peripheral time value (in seconds)
#define DEFAULT_CONN_PAUSE_PERIPHERAL        6


// -- Function Declarations -- //
void OTA_Init();
uint16_t Main_Task_ProcessEvent(uint8_t task_id, uint16_t events);


#endif /* SRC_PERIPHERAL_H_ */
