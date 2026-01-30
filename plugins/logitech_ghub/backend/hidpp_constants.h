#pragma once

// Feature IDs
#define HIDPP_PAGE_ROOT                     0x0000
#define HIDPP_PAGE_FEATURE_SET              0x0001
#define HIDPP_PAGE_DEVICE_NAME              0x0005
#define HIDPP_PAGE_BATTERY_LEVEL_STATUS     0x1000
#define HIDPP_PAGE_LED_SW_CONTROL           0x1300
#define HIDPP_PAGE_ADJUSTABLE_DPI           0x2201
#define HIDPP_PAGE_ADJUSTABLE_REPORT_RATE   0x8060
#define HIDPP_PAGE_COLOR_LED_EFFECTS        0x8070
#define HIDPP_PAGE_ONBOARD_PROFILES         0x8100

#define CMD_ROOT_GET_FEATURE                0x00 // Func 0
#define CMD_FEATURE_SET_GET_FEATURE_ID      0x10 // Func 1 (GetCount is Func 0)

#define CMD_ADJUSTABLE_DPI_SET_SENSOR_DPI   0x30 // Func 3? ig
#define CMD_ADJUSTABLE_REPORT_RATE_SET_REPORT_RATE 0x20 // Func 2? ig

#define CMD_COLOR_LED_EFFECTS_GET_INFO       0x00
#define CMD_COLOR_LED_EFFECTS_SET_ZONE_EFFECT 0x30 // Guessing, Need to look ig.
