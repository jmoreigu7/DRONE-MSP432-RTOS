/*
 * nRF24L01_SPI.c
 *
 *  Created on: 5/11/2018
 *      Author: Alvaro
 */

#include <xdc/std.h>
#include <ti/drivers/SPI.h>
#include <ti/sysbios/Knl/Clock.h>
#include <ti/drivers/GPIO.h>

#include <Board.h>
#include <msp.h>

#include "nRF24L01.h"
#include <nRF24L01_SPI.h>


/* Configuration parameters used to set-up the RF configuration */
//uint8_t rf_crc;
//uint8_t rf_addr_width;
//uint8_t rf_speed_power;
//uint8_t rf_channel;
/* Status variable updated every time SPI I/O is performed */
//uint8_t rf_status;

/* IRQ state is stored in here after msprf24_get_irq_reason(), RF24_IRQ_FLAGGED raised during
 * the IRQ port ISR--user application issuing LPMx sleep or polling should watch for this to
 * determine if the wakeup reason was due to nRF24 IRQ.
 */

//--------------------------------------------------------------------------
//Funciones spi tranfer

uint8_t spi_transfer(SPI_Handle handle, uint8_t inb)
{

    SPI_Transaction SPI_Transmision;
    uint8_t Buffer_Tx[1] = {0};
    Buffer_Tx[0] = inb;

    uint8_t Buffer_Rx[1];
    SPI_Transmision.count = 1;
    SPI_Transmision.txBuf = (Ptr) Buffer_Tx;
    SPI_Transmision.rxBuf = (Ptr) Buffer_Rx;

    SPI_transfer(handle, &SPI_Transmision);

    return Buffer_Rx[0];
}
//-----------------------------------------
uint16_t spi_transfer16(SPI_Handle handle, uint16_t inw)
{
    SPI_Transaction SPI_Transmision;
    uint8_t Buffer_Tx[2] = {0,0};
    uint8_t Buffer_Rx[2];
    uint16_t var_ret;

    Buffer_Tx[0] = (uint8_t) (inw >> 8);
    Buffer_Tx[1] = (uint8_t) (inw & 0xFF);

    SPI_Transmision.count = 2;
    SPI_Transmision.txBuf = (Ptr) Buffer_Tx;
    SPI_Transmision.rxBuf = (Ptr) Buffer_Rx;

    SPI_transfer(handle, &SPI_Transmision);

    //SPI_close(handle);

    var_ret = ((Buffer_Rx[0] << 8) | Buffer_Rx[1]);

    return var_ret;
}

//---------------------------------------------------------------------------------------
