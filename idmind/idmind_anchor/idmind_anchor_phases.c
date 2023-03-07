/*! ----------------------------------------------------------------------------
 *  @file       idmind_anchor_phases.c
 *  @brief      Code for Anchor device. Definition of phase handling functions
 *  @author     cneves
 */

#include "idmind_anchor.h"

/*! --------------------------------------------------------------------------
 * @fn print_msg()
 * @brief Auxiliary function to print messages
 * @param  none
 * @return none
 */
void print_msg(char* msg, int size)
{
    printk("M: ");
    for(int i = 0; i < size; i++) printk("0x%02X ", msg[i]);
    printk("\n");
}

/*! --------------------------------------------------------------------------
 * @fn discovery_phase()
 * @brief In the discovery phase, the anchor receives blink messages and replies
 *          with ranging init
 * @param  none
 * @return 0 if successful pairing, -1 otherwise
 */
int discovery_phase(int* seq_nr, uint32 dev_id, int* anchor_id)
{
    return 0;
}

/*! --------------------------------------------------------------------------
 * @fn ranging_phase()
 * @brief In the ranging phase, the anchor receives poll message, sends a reply,
 *          receives final message and gets a distance estimation
 * @param  none
 * @return 0 if successful ranging, -1 otherwise
 */
int ranging_phase(void)
{
    printk("Starting ranging.\n");
    return 0;
}