/*! ----------------------------------------------------------------------------
 *  @file       idmind_tag_callbacks.c
 *  @brief      Code for TAG device. Definition of callbacks
 *  @author     cneves
 */

#include "idmind_tag.h"

void tx_done_cb(const dwt_cb_data_t *cb_data){}
void rx_ok_cb(const dwt_cb_data_t *cb_data){}
void rx_timeout_cb(const dwt_cb_data_t *cb_data){}
void rx_err_cb(const dwt_cb_data_t *cb_data){}

/*! --------------------------------------------------------------------------
 * @fn rx_message()
 * @brief Function that receives message and stores in rx_buffer
 * @param  none
 * @return  0 if success, -1 otherwise
 */
int rx_message(uint8* rx_buffer){
    uint16 frame_len;
    uint32 status_reg = 0;

    rx_received = false;
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
    while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR))){}
    if (status_reg & SYS_STATUS_ALL_RX_TO){
        printk("Timeout waiting for message\n");
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO);
        return -1;
    }
    else if (status_reg & SYS_STATUS_ALL_RX_ERR){
        printk("Error receiving message\n");
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
        return -1;
    }
    else if (status_reg & SYS_STATUS_RXFCG){        
        /* Clear local RX buffer to avoid having leftovers from previous receptions. */
        for (int i = 0 ; i < FRAME_LEN_MAX; i++ ) {
            rx_buffer[i] = 0;
        }
        frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023;
        printk("Received message with %d frame len.", frame_len);
        if (frame_len <= FRAME_LEN_MAX) {
            dwt_readrxdata(rx_buffer, frame_len, 0);
        }
        
        // print_msg(rx_buffer, 12);
        /* Clear good RX frame event in the DW1000 status register. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);
        return 0;
    }
    else{
        printk("Something else happened...\n");
        return -1;
    }
}