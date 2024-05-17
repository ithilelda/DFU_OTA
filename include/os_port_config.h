#ifndef OS_PORT_CONFIG_H
#define OS_PORT_CONFIG_H


#include "config.h"
#define USE_NO_RTOS
#define osMemcpy tmos_memcpy
#define osMemset tmos_memset
#define osMemcmp tmos_memcmp


#endif /* OS_PORT_CONFIG_H */
