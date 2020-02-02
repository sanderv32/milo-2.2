/*
 * file: smc.h
 * author: Ken Curewitz
 * date: 7-aug-95
 *
 */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

extern void SMCInit(void);

/*
 * definitions of registers and macros for accessing macros
 * for the SMC FDC37C93X Plug and Play Compatible Ultra I/O Controller
 *
 */

/*
 *  configuration on/off keys
 */
#define CONFIG_ON_KEY  0x55
#define CONFIG_OFF_KEY 0xAA

/*
 * define devices in "configuration" space
 */
#define FDC   0
#define IDE1  1
#define IDE2  2
#define PARP  3
#define SER1  4
#define SER2  5
#define RTCL  6
#define KYBD  7
#define AUXIO 8

/*
 * chip level register offsets
 */
#define CONFIG_CONTROL        0x00
#define INDEX_ADDRESS         0x03
#define LOGICAL_DEVICE_NUMBER 0x07
#define DEVICE_ID             0x20
#define DEVICE_REV            0x21
#define POWER_CONTROL         0x22
#define POWER_MGMT            0x23
#define OSC                   0x24
#define ACTIVATE              0x30
#define ADDR_HI               0x60
#define ADDR_LOW              0x61
#define INTERRUPT_SEL         0x70

#define FDD_MODE_REGISTER     0x90
#define FDD_OPTION_REGISTER   0x91

#define DEVICE_ON             0x01
#define DEVICE_OFF            0x00
/*
 * values that we read back that we expect...
 */
#define VALID_DEVICE_ID       0x02

/*
 *  default device addresses
 */
#define COM1_BASE             0x3f8
#define COM1_INTERRUPT        (4 + 1)
#define COM2_BASE             0x2f8
#define COM2_INTERRUPT        (3 + 1)
