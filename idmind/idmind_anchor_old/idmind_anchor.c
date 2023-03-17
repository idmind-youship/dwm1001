/*! ----------------------------------------------------------------------------
 *  @file       idmind_anchor.c
 *  @brief      Code for Anchor device. It should listen for blinks and initiate 
 *  @author     cneves
 */

#include "idmind_anchor.h"

uint8 ranging_init[22];
uint8 resp_msg[16];

#define LOG_LEVEL 3
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

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
                      * Used in RX only. */
};

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
 * @fn enable_flag_interrupt()
 * @brief Activates a flag when an interrupt is detected
 * @param  none
 * @return 0 if success, error code otherwise
 */
void enable_flag_interrupt(void){
    printk("Interrupt detected\n");
    flag_interrupt = true;
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
    /* Prepare for callbacks/interrupts */
    port_set_deca_isr(enable_flag_interrupt);
    
    /* Open SPI to communicate with DWM1001 */
    openspi();
    /* Configure device */
    reset_DW1000(); 
    port_set_dw1000_slowrate();
    if (dwt_initialise(DWT_LOADNONE) == DWT_ERROR) {
    // if (dwt_initialise(DWT_LOADUCODE) == DWT_ERROR) {
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

    /* Set callbacks for TxDone, Rx OK, Rx Timeout, Rx Err */
    // dwt_setcallbacks(tx_done_cb, rx_ok_cb, rx_timeout_cb, rx_err_cb);

    // dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | DWT_INT_RFTO | 
    //                  DWT_INT_RXPTO | DWT_INT_RPHE | DWT_INT_RFCE | 
    //                  DWT_INT_RFSL | DWT_INT_SFDT, 1);

    /* Configure DW1000 LEDs */
    dwt_setleds(3);

    /* Apply default antenna delay value. */
    dwt_setrxantennadelay(TX_ANT_DLY);
    dwt_settxantennadelay(RX_ANT_DLY);

    /* Set expected response's delay and timeout. */
    // dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    // dwt_setrxtimeout(RX_RESP_TIMEOUT_UUS);

    k_yield();
    printk("Success!\n");
    return 0;
}

/**
 * Application entry point.
 */
int dw_main(void)
{
    /* Display application name on console. */
    print_header();
    start_dwm();
    config_dwm();

    /* Initialization of main loop */
    bool discovery = false;
    int seq_nr = 0;
    uint32 dev_id = DEV_ID;
    bool dev_list[MAX_DEVICES];
    for(int i=0; i<MAX_DEVICES; i++) dev_list[i] = false;
    flag_interrupt = false;

    /************************************************/
    /*          Setup Ranging Init Message          */
    /************************************************/
    for(int i=0; i<22; i++) ranging_init[i] = 0;
    // Header
    ranging_init[0] = 0x41;
    ranging_init[1] = 0x8C;
    // Network Name
    ranging_init[3] = PAN_ID & 0xFF;
    ranging_init[4] = (PAN_ID >> 8) & 0xFF;
    // Source address
    ranging_init[13] = (dev_id) & 0xFF;
    ranging_init[14] = (dev_id >> 8) & 0xFF;
    // Action code
    ranging_init[15] = 0x20;
    //Request a response delay
    ranging_init[18] = (int)(POLL_RX_TO_RESP_TX_DLY_UUS/1000) & 0xFF;
    ranging_init[19] = ((int)(POLL_RX_TO_RESP_TX_DLY_UUS/1000) >> 8) & 0xFF;

    /*************************************************/
    /*          Setup Poll Response Message          */
    /*************************************************/
    for(int i=0; i<16; i++) resp_msg[i] = 0;
    // Header
    resp_msg[0] = 0x41;
    resp_msg[1] = 0x88;
    // Network Name
    resp_msg[3] = PAN_ID & 0xFF;
    resp_msg[4] = (PAN_ID >> 8) & 0xFF;
    // Source address
    resp_msg[7] = (DEV_ID) & 0xFF;
    resp_msg[8] = (DEV_ID >> 8) & 0xFF;
    resp_msg[9] = 0x50;

    /* Loop forever receiving frames. */
    while (1) {
        printk("=========================\n");
        
        if(!discovery){
            discovery = (discovery_phase(&seq_nr, dev_id, dev_list, ranging_init)==0);
            if(!discovery){
                // Sleep between ranging attempts (consider putting UWB to sleep mode)
                Sleep(RANGE_PERIOD);
            }
        }
        else{
            ranging_phase(&seq_nr, dev_id, dev_list);
            discovery = false;
            Sleep(RANGE_PERIOD);
        }

    }
}