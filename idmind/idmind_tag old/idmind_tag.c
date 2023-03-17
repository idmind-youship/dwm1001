/*! ----------------------------------------------------------------------------
 *  @file       idmind_tag.c
 *  @brief      Code for TAG device. It should send blinks, wait for ranging init
 *                  send Poll, wait response and send Final
 *  @author     cneves
 */

#include "idmind_tag.h"

#define LOG_LEVEL 3
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

/* Example application name and version to display on console. */
#define APP_NAME "IDMind TAG TWR INIT v0.1"
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
    printk("Starting DWM Communication\n");
    /* Open SPI to communicate with DWM1001 */
    openspi();
    /* Configure device */
    reset_DW1000(); 
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
    dwt_setrxaftertxdelay(TX_TO_RX_DELAY_UUS);
    dwt_setrxtimeout(RX_RESP_TIMEOUT_UUS);

    k_yield();
    printk("Success!\n");
    return 0;
}

/*! --------------------------------------------------------------------------
 * @fn main()
 * @brief Application entry point.
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
    /* Set callbacks for TxDone, Rx OK, Rx Timeout, Rx Err */
    dwt_setcallbacks(tx_done_cb, NULL, NULL, NULL);

    dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | DWT_INT_RFTO | 
                     DWT_INT_RXPTO | DWT_INT_RPHE | DWT_INT_RFCE | 
                     DWT_INT_RFSL | DWT_INT_SFDT, 1);

    /* Initialization of main loop */
    bool discovery = false;
    int anchor_id = -1;
    int seq_nr = 0;
    uint32 dev_id = -1;

    // Preparation of messages
    // Set device id on Blink message
    for(int i = 0; i < 8; i++) blink_msg[2+i] = (DEV_ID >> 8*i) && 0xFF;  
    // Set device ids on Poll and Final message
    poll_msg[3] = PAN_ID & 0xFF;
    poll_msg[4] = (PAN_ID >> 8) & 0xFF;
    final_msg[3] = PAN_ID & 0xFF;
    final_msg[4] = (PAN_ID >> 8) & 0xFF;

    while(1)
    {
        printk("========================\n");
        if(!discovery){
            discovery = (discovery_phase(&seq_nr, &dev_id, &anchor_id)==0);
            if (!discovery){
                // Sleep between ranging attempts (consider putting UWB to sleep mode)
                Sleep(RANGE_PERIOD);
            }
        }
        else{
            ranging_phase(&seq_nr, &dev_id, &anchor_id);
            discovery = false;
            Sleep(RANGE_PERIOD);
        }
        
    }
    return 0;
}
