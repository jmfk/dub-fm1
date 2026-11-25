/* SPDX-License-Identifier: GPL-3.0-only */
/* Register-level test double, no operating-system USB/MIDI APIs. */
#include <stdint.h>
#include <string.h>
static uint8_t hw_global[16], hw_ep[5][32], hw_index, hw_data, hw_stall;
static uint8_t control_reply[1024];
static uint32_t control_len, sent_bytes[5], hw_enabled;
static void *rx_dma[5], *tx_dma[5];
static uint32_t rx_rearms[5],rx_flushes[5];
static uint32_t fm1_usb_sie_on(void) { return 1; }
static void fm1_usb_sie_wr_start(uint32_t reg, uint32_t value)
{
    if (reg == 14) { hw_index = value; return; }
    if (reg < 16) { hw_global[reg] = value; return; }
    if (!hw_index && reg == 17) {
        if (value == 0x60) hw_stall = 1;
        if (value & 0x40) hw_ep[0][17] &= ~1u;
        if (value & 2) hw_ep[0][17] |= 2;
        return;
    }
    if (reg == 17 && (value & 0x48)) value = 0; /* TX flush/toggle */
    if (reg == 20 && (value & 0x90)) {rx_flushes[hw_index]++;value = 0;} /* RX flush/toggle */
    hw_ep[hw_index][reg] = (uint8_t)value;
}
static void fm1_usb_sie_rd_start(uint32_t reg)
{
    hw_data = reg < 16 ? hw_global[reg] : hw_ep[hw_index][reg];
    if (reg >= 2 && reg <= 6) hw_global[reg] = 0; /* interrupt status read clears */
}
static uint32_t fm1_usb_sie_done(void) { return 1; }
static uint32_t fm1_usb_sie_data(void) { return hw_data; }
static void fm1_usb_ep_txbuf(uint32_t ep, void *p) { tx_dma[ep] = p; }
static void fm1_usb_ep_rxbuf(uint32_t ep, void *p) { rx_dma[ep] = p;rx_rearms[ep]++; }
static void fm1_usb_ep0_buf(void *p) { rx_dma[0] = p; }
static void fm1_usb_ep0_send(void *p, uint32_t n)
{
    assert(control_len + n <= sizeof control_reply);
    memcpy(control_reply + control_len, p, n);
    control_len += n;
}
static void fm1_usb_ep_send(uint32_t ep, void *p, uint32_t n)
{ tx_dma[ep] = p; sent_bytes[ep] = n; }
static void fm1_usb_ep4_txbuf(void *p) { tx_dma[4] = p; }
static void fm1_usb_ep4_send(void *p, uint32_t n) { fm1_usb_ep_send(4, p, n); }
static void fm1_usb_ep_enable(uint32_t m) { hw_enabled |= m; }
static void fm1_usb_rx_sync(void) {}
static uint32_t fm1_usb_sof_take(void) { return 0; }
static void fm1_usb_reset(void) {}
static void fm1_usb_attach(void *p) { rx_dma[0] = p; }
static void fm1_usb_off(void) {}
static void fm1_delay_ms(uint32_t n) { (void)n; }
