/*
 * header.h
 *
 *  Created on: 23 dic. 2018
 *      Author: PEPEs TEAM
 */

#ifndef HEADER_H_
#define HEADER_H_


/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h> // Necesario para coger las variables global del .cfg
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/ADC.h>
#include <ti/drivers/PWM.h>

/* Example/Board Header files */
#include "Board.h"

//Libreria de funciones matematicas
#include <math.h>

/* ====================== PROPELLERS ====================== */
#include "pid.h"
/* ======================    ADC     ====================== */
#include "adc.h"
/* ======================     RF     ====================== */
#include "nRF24L01_SPI.h"
#include "nRF24L01.h"
#include "nrf_userconfig.h"
/* ======================   MARG     ====================== */
#include "mpu9250.h"
#include "quaternionFilters.h"
#include "driverlib.h"

/*
 * https://www.ngdc.noaa.gov/geomag-web/ --> Magnetic Declination Estimated Value
 * https://bitbucket.org/lse5pepesteam/codi_dron/src/Jefferson/ --> Code
 * https://github.com/SpaceManNate/KIRC_Software --> Code example
 * http://www.eecs.ucf.edu/seniordesign/fa2013sp2014/g14/index.html --> Code example
 */

#endif /* HEADER_H_ */
