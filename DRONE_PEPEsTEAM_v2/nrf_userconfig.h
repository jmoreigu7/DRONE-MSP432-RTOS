/* nrf_userconfig.h
 * User configuration of nRF24L01+ connectivity parameters, e.g.
 * IRQ, CSN, CE pin assignments, Serial SPI driver type
 *
 *
 * Copyright (c) 2012, Eric Brundick <spirilis@linux.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright notice
 * and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _NRF_USERCONFIG_H
#define _NRF_USERCONFIG_H


#define SPI_DRIVER_EUSCI_B0 1

/* Operational pins -- IRQ, CE, CSN (SPI chip-select)
 *
 */

// IRQ
#define nrfIRQport 6
#define nrfIRQpin BIT7
/*
// CSN SPI chip-select
#define nrfCSNport 5
#define nrfCSNportout P5OUT
#define nrfCSNpin BIT2

// CE Chip-Enable (used to put RF transceiver on-air for RX or TX)
#define nrfCEport 3
#define nrfCEportout P3OUT
#define nrfCEpin BIT0
*/
#endif
