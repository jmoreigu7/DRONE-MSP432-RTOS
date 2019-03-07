/* msp432_spi.c
 * Library for performing SPI I/O on a wide range of MSP430 chips.
 *
 * Serial interfaces supported:
 * 1. USCI_A0 - developed on MSP432P01R
 * 2. USCI_B0 - developed on MSP432P01R
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

 #include <msp.h>
 #include "msp432p401r.h"
 #include "msp432_spi.h"
 #include "nrf_userconfig.h"
 #include <stdint.h>

//-- EUSCI_A0

//-- SPI Module pin
//-- STE(UCA0)  P1.0
//-- CLK(UCA0)  P1.1
//-- SOMI(UCA0) P1.2
//-- SIMO(UCA0) P1.3


//#if defined(__MCU_HAS_EUSCI_B1__) && defined(SPI_DRIVER_EUSCI_B1)
 void spi_init()
 {
//--falta configurar p1.5, p1.6

    //#ifdef __MSP432P401R
    //P6->SEL0 |= 0x00; // P1.5, P1.6 for UCB1CLK, UCB1MISO
    //P6->SEL1 &=~0x38;
//
   // #endif
  P6SEL0 |= BIT3;
  P6SEL0 |= BIT4;
  P6SEL0 |= BIT5;

  P6SEL1 &= ~BIT3;
  P6SEL1 &= ~BIT4;
  P6SEL1 &= ~BIT5;
  // EUSCI-A0 specific SPI setup
  EUSCI_B1->CTLW0=0x0001; // disable UCA0 during config
  UCB1CTL1|=UCSWRST;
  UCB1CTLW0|= UCCKPH | UCMSB | UCMST | UCMODE_0 | UCSYNC; // SPI mode 0, master
  UCB1BR0|=0x01; // SPI clocked at same speed as SMCLK
  UCB1BR1 |= 0x00;
  UCB1CTLW0 |= UCSSEL_2;  // Clock = SMCLK, clear UCSWRST and enables USCI_A module.
  EUSCI_B1->CTLW0 &=~0x0001; //-- enable UCB1 after config
}

uint8_t spi_transfer(uint8_t inb)
{
UCB1TXBUF = inb;
while ( !(UCB1IFG & UCRXIFG) )  // Wait for RXIFG indicating remote byte received via SOMI
  ;
return UCB1RXBUF;
}

uint16_t spi_transfer16(uint16_t inw)
{
uint16_t retw;
uint8_t *retw8 = (uint8_t *)&retw, *inw8 = (uint8_t *)&inw;

UCB1TXBUF = inw8[1];
while ( !(UCB1IFG & UCRXIFG) )
  ;
retw8[1] = UCB1RXBUF;
UCB1TXBUF = inw8[0];
while ( !(UCB1IFG & UCRXIFG) )
  ;
retw8[0] = UCB1RXBUF;
return retw;
}
//
//#endif


//-- EUSCI_B0

//-- SPI Module pin
//-- STE(UCB1)  P1.4
//-- CLK(UCB1)  P1.5
//-- MOSI(UCB1) P1.6
//-- MISO(UCB1) P1.7
/*
#if defined(__MCU_HAS_EUSCI_B1__) && defined(SPI_DRIVER_EUSCI_B1)
 void spi_init()
 {
     // Configure SCLK for USCI B0 peripheral module operation.
        //
     P6SEL0 |= BIT3;
        //
     P6SEL1 &= ~BIT3;

          // Configure MISO for USCI B0 peripheral module operation.
        //
     P6SEL0  |= BIT5;
        //
     P6SEL1 &= ~BIT5;

          // Configure MOSI for USCI B0 peripheral module operation.
          //
     P6SEL0  |= BIT4;
         //
     P6SEL1 &= ~BIT4;

      //P1->SEL0 |= 0x00; // P1.5, P1.6 for UCB1CLK, UCB1MISO
      //P1->SEL1 &=~0xF0;


  //EUSCI-B0 specific SPI setup
  UCB1CTLW0 =0x0001; // disable UCB1 during config
  UCB1BRW=1;// SPI clocked at same speed as SMCLK
  //UCB1CTL0|=UCSWRST;
  UCB1CTLW0 |= (UCCKPH | UCMSB | UCMST | UCMODE_0 | UCSYNC); // SPI mode 0, master
 // UCB1BR0 |=0x01; // SPI clocked at same speed as SMCLK
 // UCB1BR1 |= 0x00;
  UCB1CTLW0 |= UCSSEL_2;  // Clock = SMCLK, clear UCSWRST and enables USCI_b module.
  UCB1IFG = 0;
  UCB1CTLW0 &=~0x0001;

}

uint8_t spi_transfer(uint8_t inb)
{
UCB1TXBUF = inb;
while ( !(UCB1IFG & UCRXIFG0) )  // Wait for RXIFG indicating remote byte received via MISO
  ;
UCB1IFG &= ~(UCRXIFG + UCTXIFG);
return UCB1RXBUF;
}

uint16_t spi_transfer16(uint16_t inw)
{
uint16_t retw;
uint8_t *retw8 = (uint8_t *)&retw, *inw8 = (uint8_t *)&inw;

UCB1TXBUF = inw8[1];
while ( !(EUSCI_B0->IFG & UCRXIFG0) )//--?
  ;
retw8[1] = UCB1RXBUF;
UCB1TXBUF = inw8[0];
while ( !(EUSCI_B0->IFG & UCRXIFG0) )
  ;
retw8[0] = UCB1RXBUF;
return retw;
}
//
#endif*/
