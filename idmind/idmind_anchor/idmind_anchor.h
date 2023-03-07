/* Do all the includes */
#include <string.h>

#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "port.h"
// zephyr includes
#include <zephyr.h>
#include <sys/printk.h>

/* UWB microsecond (uus) to device time unit (dtu, around 15.65 ps) 
 * conversion factor.
 * 1 uus = 512 / 499.2 usec and 1 usec = 499.2 * 128 dtu. */
#define UUS_TO_DWT_TIME 65536

/* Delay Definitions */
// Delay after Tx to start Rx scan (UWB microseconds)
#define TX_TO_RX_DELAY_UUS 60
// Rx timeout (UWB microseconds)
#define RX_RESP_TIMEOUT_UUS 8000
// TX and Rx Antenna delays
#define TX_ANT_DLY 0
#define RX_ANT_DLY 0

#define POLL_TX_TO_RESP_RX_DLY_UUS 300

#define RESP_RX_TO_FINAL_TX_DLY_UUS 4000

/* idmind_anchor.c */
void print_header(void);
int start_dwm(void);
int config_dwm(void);
int dw_main(void);
