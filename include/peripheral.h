#ifndef SRC_PERIPHERAL_H_
#define SRC_PERIPHERAL_H_


#include "hash/sha256.h"

// -- Defines -- //
// Chip info.
// defaults to ch571.
#ifndef HARDWARE_VERSION
#define HARDWARE_VERSION             ID_CH571
#endif

#define VARIANT_CH571                0x00000000 // ch571 all has only one variant.
#define VARIANT_CH573Q               0x00000000 // ch573Q has only 256K ROM.
#define VARIANT_CH573F               0x00000001 // ch573F and others have 512K ROM.

#ifndef HARDWARE_VARIANT
#define HARDWARE_VARIANT             VARIANT_CH571
#endif

#define CH571_ROM_SIZE               0x00030000 // 192K
#define CH573_ROM_SIZE               0x00070000 // 448K
#define CH57x_RAM_SIZE               0x00003800 // 14K because we have to subtract BLE_LIB's 4K.

// bootloader info.
#ifndef BOOTLOADER_VERSION
#define BOOTLOADER_VERSION           0x00000001
#endif
#define BOOTLOADER_START_ADDR        0x00000000
#define BOOTLOADER_MAX_SIZE          0x00004000

// application info.
#define APPLICATION_START_ADDR       0x00004000
#define APPLICATION_MAX_SIZE         0x0000C000

// data storage info.
#define EEPROM_DATA_ADDR             0x00077000 - FLASH_ROM_MAX_SIZE

// jump app def.
#define jumpApp              ((void (*)(void))((uint32_t *)APPLICATION_START_ADDR))

// Main Task Events.
#define MAIN_TASK_INIT_EVENT         0x01
#define MAIN_TASK_TIMEOUT_EVENT      0x02
#define MAIN_TASK_WRITERSP_EVENT     0x04

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
// Supervision timeout value (units of 10ms, 500=5s) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_CONN_TIMEOUT         500
// Whether to enable automatic parameter update request when a connection is formed
#define DEFAULT_ENABLE_UPDATE_REQUEST        TRUE
// Connection Pause Peripheral time value (in seconds)
#define DEFAULT_CONN_PAUSE_PERIPHERAL        6

// the init packet definition (my own intepretation).
typedef struct
{
    uint8_t type; // type of firmware according to definition.
    uint8_t is_debug; // when true, the OTA will not check for firmware version and will always update.
    uint8_t hash_type; // currently we only support SHA256 hash so this field is ignored.
    uint8_t signature_type; // currently we only support HMAC with SHA256 so this field is ignored.
    uint32_t fw_version; // the version of the included firmware. Must be higher than the on chip one to proceed if not debugging.
    uint32_t hw_version; // the hardware version. MUST match exactly.
    uint32_t lib_version; // the minimum bluetooth lib version allowed.
    uint32_t bin_size; // binary file size. to check if it can be fitted.
    uint8_t fw_hash[SHA256_DIGEST_SIZE];
    uint8_t obj_signature[SHA256_DIGEST_SIZE];

} CmdObject_t;

typedef struct
{
    uint32_t boot_app; // make it 32 bit just to be dword aligned.
    uint32_t app_version;
    uint32_t bl_version;
    uint8_t hmac_key[SHA256_DIGEST_SIZE];
} EEPROM_Data_t;


// -- Function Declarations -- //
void OTA_Init();
uint16_t Main_Task_ProcessEvent(uint8_t task_id, uint16_t events);


#endif /* SRC_PERIPHERAL_H_ */
