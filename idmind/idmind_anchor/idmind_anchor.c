/*! ----------------------------------------------------------------------------
 *  @file       idmind_anchor.c
 *  @brief      Code for Anchor device. It should listen for blinks and initiate 
 *  @author     cneves
 */

#include "idmind_anchor.h"

#define LOG_LEVEL 3
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

#define DEV_ID 0x0001
#define PAN_ID 0x6380

/* Example application name and version to display on console. */
#define APP_NAME "IDMind Anchor TWR INIT v0.1"
#define APP_AUTHOR "Carlos Neves"
#define APP_DATE "March 2023"
#define APP_WIDTH 50

/* Default communication configuration. */
static dwt_config_t config = {
    5,               /* Channel number. */
    DWT_PRF_64M,     /* Pulse repetition frequency. */
    DWT_PLEN_128,    /* Preamble length. Used in TX only. */
    DWT_PAC8,        /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    1,               /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_6M8,      /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    (129)            /* SFD timeout (preamble length + 1 + SFD length - PAC size). 
                        Used in RX only. */           
};

/* Buffer to store received response message.
 * Its size is adjusted to longest frame that this example code is supposed 
 * to handle.
 */
#define RX_BUF_LEN 20
static uint8 rx_buffer[RX_BUF_LEN];

#define PERIOD 500
#define SPEED_OF_LIGHT 299702547

/*! --------------------------------------------------------------------------
 * @fn print_header()
 * @brief Routine to print header with name, author and date, if defined
 * @param  none
 * @return none
 */
void print_header(void)
{
    /* Print header */
    char separator[APP_WIDTH+1];
    for (int i = 0; i < APP_WIDTH; i++) separator[i] = '*';
    separator[APP_WIDTH+1] = '\0';
    printk("/*%s*/\n", separator);
    /* Print centered app name header */
    char name[APP_WIDTH+1];
    int stop1 = (int)((APP_WIDTH-strlen(APP_NAME))/2);
    for (int i = 0; i < APP_WIDTH; i++) name[i] = ' ';
    for (int i = 0; i < strlen(APP_NAME); i++) name[stop1+i] = APP_NAME[i];
    name[APP_WIDTH] = '\0';
    printk("/*%s*/\n", name);
    /* Print Author */
    #ifdef APP_AUTHOR
    char author[APP_WIDTH+1];
    int stop2 = (int)((APP_WIDTH-strlen(APP_AUTHOR))/2);
    for (int i = 0; i < APP_WIDTH; i++) author[i] = ' ';
    for (int i = 0; i < strlen(APP_AUTHOR); i++) author[stop2+i] = APP_AUTHOR[i];
    author[APP_WIDTH] = '\0';
    printk("/*%s*/\n", author);
    #endif
    /* Print Date */
    #ifdef APP_DATE
    char date[APP_WIDTH+1];
    int stop3 = (int)((APP_WIDTH-strlen(APP_DATE))/2);
    for (int i = 0; i < APP_WIDTH; i++) date[i] = ' ';
    for (int i = 0; i < strlen(APP_DATE); i++) date[stop3+i] = APP_DATE[i];
    date[APP_WIDTH] = '\0';
    printk("/*%s*/\n", date);
    #endif
    /* Print footer */
    printk("/*%s*/\n",separator);
    return;
}

/*! --------------------------------------------------------------------------
 * @fn start_dwm()
 * @brief Opens communication with DWM, reset and initialize
 * @param  none
 * @return 0 if success, error code otherwise
 */

int start_dwm(void)
{
    printk("Starting DWM Communication with ");
    /* Open SPI to communicate with DWM1001 */
    openspi();
    /* Configure device */
    port_set_dw1000_slowrate();
    if (dwt_initialise(DWT_LOADUCODE) == DWT_ERROR) {
        printk("INIT FAILED");
        k_sleep(K_MSEC(500)); // allow logging to run.
        while (1) { };
    }
    port_set_dw1000_fastrate();
    printk("Success!\n");
    return 0;
}

/*! --------------------------------------------------------------------------
 * @fn config_dwm()
 * @brief Configures DWM
 * @param  none
 * @return 0 if success, error code otherwise
 */

int config_dwm(void)
{
    printk("Configuring DWM... ");
    /* Configure DW1000. */
    dwt_configure(&config);

    /* Configure DW1000 LEDs */
    dwt_setleds(3);

    /* Apply default antenna delay value. */
    dwt_setrxantennadelay(TX_ANT_DLY);
    dwt_settxantennadelay(RX_ANT_DLY);

    /* Set expected response's delay and timeout. */
    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    dwt_setrxtimeout(RX_RESP_TIMEOUT_UUS);

    k_yield();
    printk("Success!\n");
    return 0;
}

/*! --------------------------------------------------------------------------
 * @fn     main()
 * @brief  Application entry point.
 * @param  none
 * @return none
 */
int dw_main(void)
{
    /* Print Header */
    print_header();
    /* Start communication with DWM */
    start_dwm();
    /* Configure DWM */
    config_dwm();

    /* Initialization of main loop */
    bool discovery = false;
    int anchor_id = 1;
    int seq_nr = 0;
    uint32 dev_id = dwt_readdevid();
    uint32 dev_list[20];
    
    while(1)
    {
        if(!discovery){
            discovery = (discovery_phase(&seq_nr, dev_id, &anchor_id)==0);
        }
        else{
            ranging_phase();
            discovery = false;
        }
        Sleep(PERIOD);
    }
    return 0;
}
