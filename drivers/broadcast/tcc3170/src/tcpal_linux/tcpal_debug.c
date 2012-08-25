
#include "tcpal_os.h"
#include "tcpal_debug.h"

#include <linux/moduleparam.h>

module_param(TcbdDebugClass, int, 0644);

unsigned int TcbdDebugClass = 
    //DEBUG_STREAM_PARSER |
    DEBUG_API_COMMON |
    //DEBUG_DRV_PERI |
    //DEBUG_DRV_IO |
    //DEBUG_DRV_COMP |
    //DEBUG_DRV_RF |
    //DEBUG_TCPAL_OS |
    DEBUG_TCPAL_CSPI |
    //DEBUG_TCPAL_I2C |
    DEBUG_TCHAL |
    DEBUG_INFO |
    DEBUG_ERROR;
