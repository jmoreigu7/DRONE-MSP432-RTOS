/*
 * mpu9250.c
 *
 *  Created on: 20 may. 2018
 *      Author: PEPEs TEAM
 */

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>

#include <stdint.h>
#include <math.h>

/* Board Header files and libraries */
#include "Board.h"
#include "driverlib.h"
#include "mpu9250.h"
#include "quaternionFilters.h"

/*
 * Inicializacion de la estructura mpu9250, escojemos las escalas de trabajo para el MPU9250
 * y el modo de trabajo. Configure the magnetometer for continuous read and highest resolution
 * set Mscale bit 4 to 1 (0) to enable 16 (14) bit resolution in CNTL register,
 * and enable continuous mode data acquisition Mmode (bits [3:0]), 0010 for 8 Hz and 0110 for 100 Hz sample rates
 */
void init_struct(mpu9250 * in){	
	in->Gscale = GFS_2000DPS;
	in->Ascale = AFS_16G;
	in->Mscale = MFS_16BITS;
	in->Mmode 	= 6; //  2 for 8 Hz, 6 for 100 Hz continuous magnetometer data read
	in->lastUpdate = 0;
}

/*
 * Escalas posibles del gyro y sus bits de configuración de registros son: 250 DPS (00),
 * 500 DPS (01), 1000 DPS (10), and 2000 DPS (11). Aqui tenemos un algoritmo para calcular
 * DPS/(ADC tick) basado en valores de 2 bits.
 */
void getGres(mpu9250 *foo){
    switch (foo->Gscale){
    case GFS_250DPS:
          foo->gRes = 250.0/32768.0;
          break;
    case GFS_500DPS:
          foo->gRes = 500.0/32768.0;
          break;
    case GFS_1000DPS:
          foo->gRes = 1000.0/32768.0;
          break;
    case GFS_2000DPS:
          foo->gRes = 2000.0/32768.0;
          break;
  }
}

/*
 * Escalas posibles del accelerometer y sus bits de configuración de registros son: 2 Gs (00),
 * 4 Gs (01), 8 Gs (10), and 16 Gs  (11).  Aqui tenemos un algoritmo para calcular
 * DPS/(ADC tick) basado en valores de 2 bits.
 */
void getAres(mpu9250 *foo){
    switch (foo->Ascale){
    case AFS_2G:
          foo->aRes = 2.0/32768.0;
          break;
    case AFS_4G:
          foo->aRes = 4.0/32768.0;
          break;
    case AFS_8G:
          foo->aRes = 8.0/32768.0;
          break;
    case AFS_16G:
          foo->aRes = 16.0/32768.0;
          break;
  }
}

/*
 * Escalas posibles del magnetometer y sus bits de configuración de registros son: 14 bits
 * de resolucion (0) y 16 bits de resolucion (1)
 */
void getMres(mpu9250 * foo){
	switch(foo->Mscale){
	case MFS_14BITS:
	    foo->mRes = 10.*4912./8190.; // Proper scale to return milliGauss
	    break;
    case MFS_16BITS:
	    foo->mRes = 10.*4912./32760.0; // Proper scale to return milliGauss
	    break;
  }
}

/*
 * Autocomprobacion del MPU6050
 *      Modulos utilizados: EUSCI B1 (I2C)
 */
void MPU6050SelfTest(I2C_Handle handle, float *destination){
    uint8_t             rawData[6] = {0, 0, 0, 0, 0, 0};
    uint8_t             selfTest[6];
    uint8_t             FS = 0;
    int32_t             gAvg[3] = {0}, aAvg[3] = {0}, aSTAvg[3] = {0}, gSTAvg[3] = {0};
    float               factoryTrim[6];
    int                 ii, i;

    // Set gyro sample rate to 1 kHz
    writeRegister(handle, Board_MPU6050, SMPLRT_DIV, 0x00);
    // Set gyro sample rate to 1 kHz and DLPF to 92 Hz
    writeRegister(handle, Board_MPU6050, CONFIG, 0x02);
    // Set full scale range for the gyro to 250 dps
    writeRegister(handle, Board_MPU6050, GYRO_CONFIG, 1<<FS);
    // Set accelerometer rate to 1 kHz and bandwidth to 92 Hz
    writeRegister(handle, Board_MPU6050, ACCEL_CONFIG2, 0x02);
    // Set full scale range for the accelerometer to 2 g
    writeRegister(handle, Board_MPU6050, ACCEL_CONFIG, 1<<FS);

    // Get average current values of gyro and acclerometer
    for (ii = 0; ii < 200; ii++){
        // Read the six raw data registers into data array
        readRegisters(handle, Board_MPU6050, ACCEL_XOUT_H, 6, &rawData[0]);
        // Turn the MSB and LSB into a signed 16-bit value
        aAvg[0] += (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;
        aAvg[1] += (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;
        aAvg[2] += (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ;

        // Read the six raw data registers sequentially into data array
        readRegisters(handle, Board_MPU6050, GYRO_XOUT_H, 6, &rawData[0]);
        // Turn the MSB and LSB into a signed 16-bit value
        gAvg[0] += (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;
        gAvg[1] += (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;
        gAvg[2] += (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ;
    }

    // Get average of 200 values and store as average current readings
    for (ii =0; ii < 3; ii++) {
        aAvg[ii] /= 200;
        gAvg[ii] /= 200;
    }

    // Configure the accelerometer for self-test
    // Enable self test on all three axes and set accelerometer range to +/- 2 g
    writeRegister(handle, Board_MPU6050, ACCEL_CONFIG, 0xE0);
    // Enable self test on all three axes and set gyro range to +/- 250 degrees/s
    writeRegister(handle, Board_MPU6050, GYRO_CONFIG,  0xE0);
    Task_sleep(15);  // Delay a while to let the device stabilize

    // Get average self-test values of gyro and acclerometer
    for (ii = 0; ii < 200; ii++) {
        // Read the six raw data registers into data array
        readRegisters(handle, Board_MPU6050, ACCEL_XOUT_H, 6, &rawData[0]);
        // Turn the MSB and LSB into a signed 16-bit value
        aSTAvg[0] += (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;
        aSTAvg[1] += (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;
        aSTAvg[2] += (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ;

        // Read the six raw data registers sequentially into data array
        readRegisters(handle, Board_MPU6050, GYRO_XOUT_H, 6, &rawData[0]);
        // Turn the MSB and LSB into a signed 16-bit value
        gSTAvg[0] += (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;
        gSTAvg[1] += (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;
        gSTAvg[2] += (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ;
    }

    // Get average of 200 values and store as average self-test readings
    for (ii =0; ii < 3; ii++) {
        aSTAvg[ii] /= 200;
        gSTAvg[ii] /= 200;
    }

    // Configure the gyro and accelerometer for normal operation
    writeRegister(handle, Board_MPU6050, ACCEL_CONFIG, 0x00);
    writeRegister(handle, Board_MPU6050, GYRO_CONFIG,  0x00);
    //delay(25);  // Delay a while to let the device stabilize

    // Retrieve accelerometer and gyro factory Self-Test Code from USR_Reg
    // X-axis accel self-test results
    selfTest[0] = readRegister(handle, Board_MPU6050, SELF_TEST_X_ACCEL);
    // Y-axis accel self-test results
    selfTest[1] = readRegister(handle, Board_MPU6050, SELF_TEST_Y_ACCEL);
    // Z-axis accel self-test results
    selfTest[2] = readRegister(handle, Board_MPU6050, SELF_TEST_Z_ACCEL);
    // X-axis gyro self-test results
    selfTest[3] = readRegister(handle, Board_MPU6050, SELF_TEST_X_GYRO);
    // Y-axis gyro self-test results
    selfTest[4] = readRegister(handle, Board_MPU6050, SELF_TEST_Y_GYRO);
    // Z-axis gyro self-test results
    selfTest[5] = readRegister(handle, Board_MPU6050, SELF_TEST_Z_GYRO);

    // Retrieve factory self-test value from self-test code reads
    // FT[Xa] factory trim calculation
    factoryTrim[0] = (float)(2620/1<<FS)*(pow(1.01 ,((float)selfTest[0] - 1.0) ));
    // FT[Ya] factory trim calculation
    factoryTrim[1] = (float)(2620/1<<FS)*(pow(1.01 ,((float)selfTest[1] - 1.0) ));
    // FT[Za] factory trim calculation
    factoryTrim[2] = (float)(2620/1<<FS)*(pow(1.01 ,((float)selfTest[2] - 1.0) ));
    // FT[Xg] factory trim calculation
    factoryTrim[3] = (float)(2620/1<<FS)*(pow(1.01 ,((float)selfTest[3] - 1.0) ));
    // FT[Yg] factory trim calculation
    factoryTrim[4] = (float)(2620/1<<FS)*(pow(1.01 ,((float)selfTest[4] - 1.0) ));
    // FT[Zg] factory trim calculation
    factoryTrim[5] = (float)(2620/1<<FS)*(pow(1.01 ,((float)selfTest[5] - 1.0) ));

    // Report results as a ratio of (STR - FT)/FT; the change from Factory Trim
    // of the Self-Test Response
    // To get percent, must multiply by 100
    for (i = 0; i < 3; i++) {
        // Report percent differences
        destination[i] = 100.0 * ((float)(aSTAvg[i] - aAvg[i])) / factoryTrim[i] - 100.;
        // Report percent differences
        destination[i+3] = 100.0 * ((float)(gSTAvg[i] - gAvg[i])) / factoryTrim[i+3] - 100.;
    }
}

/*
 * MPU9250calibrate - Calibración del modulo MPU y extraccion de bias del accelrometro y giroscopio.
 *      Modulos utilizados: EUSCI B1 (I2C)
 */
void MPU6050calibrate(I2C_Handle handle, float * gyroBias, float *accelBias){
    uint8_t data[12]; // Array de datos para mantener los datos x,y y z del acelerometro y giroscopio
    uint16_t ii, packet_count, fifo_count;
    int32_t gyro_bias[3]  = {0, 0, 0}, accel_bias[3] = {0, 0, 0};

    // Reset del dispositivo
    // Escribir un bit de reinicio de 1 a 7; activar el dispositivo de reinicio
    writeRegister(handle, Board_MPU6050, PWR_MGMT_1, 0x80);
    Task_sleep(100);
    // Obtener una fuente de tiempo estable; seleccionar automaticamente la fuente del reloj para que sea un giroscopio PLL
    // Referencia si esta listo para usar el oscilador interno, bits 2:0 = 001
    writeRegister(handle, Board_MPU6050, PWR_MGMT_1, 0x01);
    writeRegister(handle, Board_MPU6050, PWR_MGMT_2, 0x00);
    Task_sleep(100); // Muere al incorporar el RF-SPI

    // Configurar el dispositivo para el calculo de bias
    // Desactivar todas las interrupciones
    writeRegister(handle, Board_MPU6050, INT_ENABLE, 0x00);
    // Desactivar la FIFO
    writeRegister(handle, Board_MPU6050, FIFO_EN, 0x00);
    // Activar la fuente de reloj interno
    writeRegister(handle, Board_MPU6050, PWR_MGMT_1, 0x00);
    // Desactivar I2C master
    writeRegister(handle, Board_MPU6050, I2C_MST_CTRL, 0x00);
    // Desactivar modos FIFO y I2C master
    writeRegister(handle, Board_MPU6050, USER_CTRL, 0x00);
    // Reset FIFO y DMP
    writeRegister(handle, Board_MPU6050, USER_CTRL, 0x0C);
    Task_sleep(100); // Muere al incorporar el RF-SPI

    // Configurar el giroscopio y el acelerometro del MPU6050 para el calculo de bias
    // Ajustar el filtro pasa bajo a 188 Hz
    writeRegister(handle, Board_MPU6050, CONFIG, 0x01);
    // Ajustar la frecuencia de muestreo a 1 kHz
    writeRegister(handle, Board_MPU6050, SMPLRT_DIV, 0x00);
    // Ajustar la escala completa a 250 grados por segundos, maxima sensibilidad
    writeRegister(handle, Board_MPU6050, GYRO_CONFIG, 0x00);
    // Ajustar la escala completa a 2 g, maxima sensibilidad
    writeRegister(handle, Board_MPU6050, ACCEL_CONFIG, 0x00);

    uint16_t  gyrosensitivity  = 131;   // = 131 LSB/degrees/sec
    uint16_t  accelsensitivity = 16384; // = 16384 LSB/g

    // Configurar FIFO para capturar datos de acelerometros y giroscopios para el calculo de bias
    // Habilitar FIFO
    writeRegister(handle, Board_MPU6050, USER_CTRL, 0x40);
    // Habilitar los sensores gyro i accelerometer para la FIFO  (tamaño maximo de 512 bytes in MPU-9150)
    writeRegister(handle, Board_MPU6050, FIFO_EN, 0x78);
    Task_sleep(100); // Muere al incorporar el RF-SPI

    // Al final de la acumulacion de la muestra, apagar la lectura del sensor FIFO
    // Deshabilitar los sensores gyro i accelerometer para la FIFO
    writeRegister(handle, Board_MPU6050, FIFO_EN, 0x00);
    // Leer el recuentro de muestras FIFO
    readRegisters(handle, Board_MPU6050, FIFO_COUNTH, 2, &data[0]);
    fifo_count = ((uint16_t)data[0] << 8) | data[1];
    // Cuantos conjuntos de datos completos de giroscopios y acelerómetros para promediar
    packet_count = fifo_count/12;

    for (ii = 0; ii < packet_count; ii++) {
        int16_t accel_temp[3] = {0, 0, 0}, gyro_temp[3] = {0, 0, 0};
        // Leer datos para promediar
        readRegisters(handle, Board_MPU6050, FIFO_R_W, 12, &data[0]);
        // Formato de 16-bits enteros para cada muestra en la FIFO
        accel_temp[0] = (int16_t) (((int16_t)data[0] << 8) | data[1]  );
        accel_temp[1] = (int16_t) (((int16_t)data[2] << 8) | data[3]  );
        accel_temp[2] = (int16_t) (((int16_t)data[4] << 8) | data[5]  );
        gyro_temp[0]  = (int16_t) (((int16_t)data[6] << 8) | data[7]  );
        gyro_temp[1]  = (int16_t) (((int16_t)data[8] << 8) | data[9]  );
        gyro_temp[2]  = (int16_t) (((int16_t)data[10] << 8) | data[11]);

        // Suma de bias (16 bits) de forma individual para obtener las bias acumuladas de 32 bits
        accel_bias[0] += (int32_t) accel_temp[0];
        accel_bias[1] += (int32_t) accel_temp[1];
        accel_bias[2] += (int32_t) accel_temp[2];
        gyro_bias[0]  += (int32_t) gyro_temp[0];
        gyro_bias[1]  += (int32_t) gyro_temp[1];
        gyro_bias[2]  += (int32_t) gyro_temp[2];
    }
    // Suma de bias (16 bits) de forma individual para obtener las bias acumuladas de 32 bits
    accel_bias[0] /= (int32_t) packet_count;
    accel_bias[1] /= (int32_t) packet_count;
    accel_bias[2] /= (int32_t) packet_count;
    gyro_bias[0]  /= (int32_t) packet_count;
    gyro_bias[1]  /= (int32_t) packet_count;
    gyro_bias[2]  /= (int32_t) packet_count;

    // Suma de bias (16 bits) de forma individual para obtener las bias acumuladas de 32 bits
    if (accel_bias[2] > 0L) {
        accel_bias[2] -= (int32_t) accelsensitivity;
    }
    else {
        accel_bias[2] += (int32_t) accelsensitivity;
    }

    // Contruir las bias de giroscopio para empujar a los registros de bias de giroscopio por hardware
    // que se reajustan a cero al arrancar el dispositivo
    // Dividir por 4 para obtener 32.9 LSB por grado/s para ajustarse al formato de entrada de bias esperado.
    data[0] = (-gyro_bias[0]/4  >> 8) & 0xFF;
    // Los bias son aditivos, asi que cambie el signo en los bias medios calculados de los giroscopios.
    data[1] = (-gyro_bias[0]/4)       & 0xFF;
    data[2] = (-gyro_bias[1]/4  >> 8) & 0xFF;
    data[3] = (-gyro_bias[1]/4)       & 0xFF;
    data[4] = (-gyro_bias[2]/4  >> 8) & 0xFF;
    data[5] = (-gyro_bias[2]/4)       & 0xFF;

    // Empujar los bias de los giroscopios a los registros de hardware
    writeRegister(handle, Board_MPU6050, XG_OFFSET_H, data[0]);
    writeRegister(handle, Board_MPU6050, XG_OFFSET_L, data[1]);
    writeRegister(handle, Board_MPU6050, YG_OFFSET_H, data[2]);
    writeRegister(handle, Board_MPU6050, YG_OFFSET_L, data[3]);
    writeRegister(handle, Board_MPU6050, ZG_OFFSET_H, data[4]);
    writeRegister(handle, Board_MPU6050, ZG_OFFSET_L, data[5]);

    // Salida de bias del giroscopios a escala para su visualizacion en el programa principal.
    gyroBias[0] = (float) gyro_bias[0]/(float) gyrosensitivity;
    gyroBias[1] = (float) gyro_bias[1]/(float) gyrosensitivity;
    gyroBias[2] = (float) gyro_bias[2]/(float) gyrosensitivity;

    /* Construya los bias del acelerometro para empujar a los registros de bias del acelerometro
     * por hardware. Estos registros contienen valores de ajuste de fabrica que deben añadirse a
     * los bias calculados del acelerometro; al arrancar, estos registros contienen valores distintos
     * de cero. Ademas, el bit 0 del byte inferior debe conservarse, ya que se utiliza para los
     * calculos de compensación de temperatura.  Los registros de bias del acelerometro esperan una
     * entrada de bias de 2048 LSB por g, de modo que los bias del acelerometro calculados anteriormente
     * deben dividirse por 8.
     */

    // Un lugar para sujetar los "trim bias" del acelerometro de fabrica
    int32_t accel_bias_reg[3] = {0, 0, 0};
    // Leer los valores de ajuste (trim values) del acelerometro de fabrica
    readRegisters(handle, Board_MPU6050, XA_OFFSET_H, 2, &data[0]);
    accel_bias_reg[0] = (int32_t) (((int16_t)data[0] << 8) | data[1]);
    readRegisters(handle, Board_MPU6050, YA_OFFSET_H, 2, &data[0]);
    accel_bias_reg[1] = (int32_t) (((int16_t)data[0] << 8) | data[1]);
    readRegisters(handle, Board_MPU6050, ZA_OFFSET_H, 2, &data[0]);
    accel_bias_reg[2] = (int32_t) (((int16_t)data[0] << 8) | data[1]);

    // Definir mascara para el bit de compensacion de temperatura 0 del byte inferior de los registros
    // de bias del acelerómetro
    uint32_t mask = 1uL;
    // Define el array para mantener el bit de la mascara para cada eje de bias del acelerometro
    uint8_t mask_bit[3] = {0, 0, 0};

    for (ii = 0; ii < 3; ii++) {
        // Si se ajusta el bit de compensación de temperatura, registre este hecho en mask_bit
        if ((accel_bias_reg[ii] & mask)) {
            mask_bit[ii] = 0x01;
        }
    }
    /* Construir el bias total del acelerometro, incluyendo el bias promedio calculado del acelerometro
     * desde arriba. Restamos el bias de acelerometro promedio calculado y escalado a 2048 LSB/g
     * (escala completa de 16 g).
     */

    accel_bias_reg[0] -= (accel_bias[0]/8);
    accel_bias_reg[1] -= (accel_bias[1]/8);
    accel_bias_reg[2] -= (accel_bias[2]/8);

    data[0] = (accel_bias_reg[0] >> 8) & 0xFF;
    data[1] = (accel_bias_reg[0])      & 0xFF;
    // Conservar el bit de compensacion de temperatura al escribir de vuelta a los registros de bias del
    // acelerometro
    data[1] = data[1] | mask_bit[0];
    data[2] = (accel_bias_reg[1] >> 8) & 0xFF;
    data[3] = (accel_bias_reg[1])      & 0xFF;
    // Conservar el bit de compensacion de temperatura al escribir de vuelta a los registros de bias del
    // acelerometro
    data[3] = data[3] | mask_bit[1];
    data[4] = (accel_bias_reg[2] >> 8) & 0xFF;
    data[5] = (accel_bias_reg[2])      & 0xFF;
    // Conservar el bit de compensacion de temperatura al escribir de vuelta a los registros de bias del
    // acelerometro
    data[5] = data[5] | mask_bit[2];

    // Aparentemente esto no esta funcionando para los bias de aceleración en el MPU-9250.
    // Empujar los bias del acelerometro a los registros de hardware
    writeRegister(handle, Board_MPU6050, XA_OFFSET_H, data[0]);
    writeRegister(handle, Board_MPU6050, XA_OFFSET_L, data[1]);
    writeRegister(handle, Board_MPU6050, YA_OFFSET_H, data[2]);
    writeRegister(handle, Board_MPU6050, YA_OFFSET_L, data[3]);
    writeRegister(handle, Board_MPU6050, ZA_OFFSET_H, data[4]);
    writeRegister(handle, Board_MPU6050, ZA_OFFSET_L, data[5]);

    // Salida de los bias escalonados del acelerometro para su visualización en el programa principal
    accelBias[0] = (float)accel_bias[0]/(float)accelsensitivity;
    accelBias[1] = (float)accel_bias[1]/(float)accelsensitivity;
    accelBias[2] = (float)accel_bias[2]/(float)accelsensitivity;
}

/*
 * Inicialización del acelerometro y del giroscopio
 *      Modulos utilizados: EUSCI B1 (I2C)
 */

void MPU6050init(I2C_Handle handle){
    writeRegister(handle, Board_MPU6050, PWR_MGMT_1, 0x80);    // Sleep mode bit (6)
    Task_sleep(2); //1-2ms para el reset de todos los registros
    writeRegister(handle, Board_MPU6050, PWR_MGMT_1, 0x00);       // Internal 20MHz oscillator, activamos los sensores
    Task_sleep(2);
    /* Hacemos estable el tiempo de la fuente
     * Selección automática de la fuente de clock que es PLL gyroscope reference if ready, else use the internal oscillator
     * Gyro y accel en modo normal
     */
    writeRegister(handle, Board_MPU6050, PWR_MGMT_1, 0x01);
    Task_sleep(40);
    // Configure Gyro and Thermometer
    // Disable FSYNC and set thermometer and gyro bandwidth to 41 and 42 Hz, respectively;
    // minimum delay time for this setting is 5.9 ms, which means sensor fusion update rates cannot
    // be higher than 1 / 0.0059 = 170 Hz
    // DLPF_CFG = bits 2:0 = 011; this limits the sample rate to 1000 Hz for both
    // With the MPU9250, it is possible to get gyro sample rates of 32 kHz (!), 8 kHz, or 1 kHz
    writeRegister(handle, Board_MPU6050, CONFIG, 0x03);   // Configuración MPU
    // Set sample rate = gyroscope output rate/(1 + SMPLRT_DIV)
    // Use a 200 Hz rate; a rate consistent with the filter update rate , determined inset in CONFIG above
    writeRegister(handle, Board_MPU6050, SMPLRT_DIV, 0x04);
    // Set full scale range for the gyro
    writeRegister(handle, Board_MPU6050, GYRO_CONFIG, 0x10);
    // 0x08 nos indica la susceptibilidad:  4g/sec // Set full scale range for the accelerometer
    writeRegister(handle, Board_MPU6050, ACCEL_CONFIG, 0x08);
    // 0x01, same as CONFIG 0x03
    writeRegister(handle, Board_MPU6050, ACCEL_CONFIG2, 0x03);
    // Configure Interrupts and Bypass Enable
    // Set interrupt pin active high, push-pull, hold interrupt pin level HIGH until interrupt cleared,
    // clear on read of INT_STATUS, and enable I2C_BYPASS_EN so additional chips
    // can join the I2C bus and all can be controlled by the msp as master
    writeRegister(handle, Board_MPU6050, INT_PIN_CFG, 0x02);
    writeRegister(handle, Board_MPU6050, INT_ENABLE, 0x01);
    Task_sleep(20);
}

/*
 * Inicialización del magnetometro
 *      Modulos utilizados: EUSCI B1 (I2C)
 */
void AK8963init(I2C_Handle handle){
    writeRegister(handle, Board_AK8963, AK8963_CNTL, 0x00); // Power down magnetometer
    Task_sleep(20); //con 10 funciona
    writeRegister(handle, Board_AK8963, AK8963_CNTL, 0x0F); // Enter Fuse ROM access mode
    Task_sleep(20);
    writeRegister(handle, Board_AK8963, AK8963_CNTL, MAG_SCALE << 4 | MAG_MODE_100Hz);  // Escala del magnetometro y el modo
    Task_sleep(20);
}

/*
 * Lectura del Accelerometre (MPU6050) en los tres ejes X,Y,Z. Arquitectura Big Endian.
 * Convertimos el MSB y el LSB en un valor de 16 bits. No retorna nada debido a que los datos
 * los procesamos dentro de la estructura para posteriormente tratar estos datos en el
 * algoritmo de fusion de datos Mahony.
 *      Modulos utilizados: EUSCI B1 (I2C)
 */
int readAccel(I2C_Handle handle, int16_t * destination){
    uint8_t             txBuffer[1];
    uint8_t             rawData[6];
    I2C_Transaction     i2cTransaction;

    i2cTransaction.slaveAddress = Board_MPU6050;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.readBuf = rawData;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readCount = 6;

    txBuffer[0] = ACCEL_XOUT_H;

    if (I2C_transfer(handle, &i2cTransaction)){
        destination[0] = ((int16_t)rawData[0] << 8) | rawData[1] ;  // MSB to LSB
        destination[1] = ((int16_t)rawData[2] << 8) | rawData[3] ;
        destination[2] = ((int16_t)rawData[4] << 8) | rawData[5] ;
    }
    return 0;
}


/*
 * Lectura del Gyroscopio (MPU6050) en los tres ejes X,Y,Z. Arquitectura Big Endian.
 * Convertimos el MSB y el LSB en un valor de 16 bits. No retorna nada debido a que los datos
 * los procesamos dentro de la estructura para posteriormente tratar estos datos en el
 * algoritmo de fusion de datos Mahony.
 *      Modulos utilizados: EUSCI B1 (I2C)
 * */
int readGyro(I2C_Handle handle,  int16_t *destination){
    uint8_t             txBuffer[1];
    uint8_t             rawData[6];
    I2C_Transaction     i2cTransaction;

    i2cTransaction.slaveAddress = Board_MPU6050;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.readBuf = rawData;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readCount = 6;

    txBuffer[0] = GYRO_XOUT_H;

    if (I2C_transfer(handle, &i2cTransaction)){
        destination[0] = ((int16_t)rawData[0] << 8) | rawData[1] ;  // MSB to LSB
        destination[1] = ((int16_t)rawData[2] << 8) | rawData[3] ;
        destination[2] = ((int16_t)rawData[4] << 8) | rawData[5] ;
    }
    return 0;
}

/*
 * Lectura del Magnetometro (AK8963) en los tres ejes X,Y,Z. Arquitectura Big Endian.
 * Convertimos el MSB y el LSB en un valor de 16 bits. No retorna nada debido a que los datos
 * los procesamos dentro de la estructura para posteriormente tratar estos datos en el
 * algoritmo de fusion de datos Mahony.
 *      Modulos utilizados: EUSCI B1 (I2C)
 */
int readMagn(I2C_Handle handle, int16_t *destination){
    uint8_t             txBuffer[1];
    uint8_t             rawData[6];
    I2C_Transaction     i2cTransaction;

    i2cTransaction.slaveAddress = Board_AK8963;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.readBuf = rawData;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readCount = 6;

    txBuffer[0] = AK8963_XOUT_L;

    if (I2C_transfer(handle, &i2cTransaction)){
        destination[0] = ((int16_t)rawData[1] << 8) | rawData[0]; // LSB to MSB
        // Se guarda el dato como little Endian (distinto al resto)
        destination[1] = ((int16_t)rawData[3] << 8) | rawData[2];
        destination[2] = ((int16_t)rawData[5] << 8) | rawData[4];
    }
    return 0;
}

/*
 * Una vez hemos leido los datos procedemos a convertirlos en float y con su correspondiente
 * escala: AFS_16G
 *      Modulos utilizados: EUSCI B1 (I2C)
 */
void setAccelData(I2C_Handle handle, mpu9250 *foo){
    getAres(foo);
    readAccel(handle, foo->accelCount);
    foo->ax = (float)foo->accelCount[0]*foo->aRes-foo->accelBias[0];
    foo->ay = (float)foo->accelCount[1]*foo->aRes-foo->accelBias[1];;
    foo->az = (float)foo->accelCount[2]*foo->aRes-foo->accelBias[2];;
}

/*
 * Una vez hemos leido los datos procedemos a convertirlos en float y con su correspondiente
 * escala: GFS_2000DPS
 *      Modulos utilizados: EUSCI B1 (I2C)
 */
void setGyroData(I2C_Handle handle, mpu9250 *foo){
	getGres(foo);
	readGyro(handle, foo->gyroCount);
	foo->gx = (float)foo->gyroCount[0]*foo->gRes-foo->gyroBias[0];
	foo->gy = (float)foo->gyroCount[1]*foo->gRes-foo->gyroBias[1];
	foo->gz = (float)foo->gyroCount[2]*foo->gRes-foo->gyroBias[2];
}

/*
 * Una vez hemos leido los datos procedemos a convertirlos en float y con su correspondiente
 * escala: MFS_16BITS
 *      Modulos utilizados: EUSCI B1 (I2C)
 */
void setMagnData(I2C_Handle handle, mpu9250 *foo){
	getMres(foo);
	readMagn(handle, foo->magCount);
	foo->mx = (float)foo->magCount[0]*foo->mRes;
	foo->my = (float)foo->magCount[1]*foo->mRes;
	foo->mz = (float)foo->magCount[2]*foo->mRes;
}

/*
 * Actualizacion del tiempo, esta funcion nos permite ir actualizando el tiempo mediante el
 * Timer A1. Es necesario para no tener cambios abruptos en adquisicion de datos, hay que
 * evitalo ya si tenemos datos muy abruptos la fusion de datos en el algoritmo Mahony seran
 * poco fiables.
 *      Modulos utilizados: Timer A1
 */
void updateTime(mpu9250* foo){
    uint16_t current_time = TA1R;
    foo->Now = current_time;
    if(foo->Now < foo->lastUpdate){                 // overflow happened
        foo->lastUpdate = 0xFFFF - foo->lastUpdate;
        foo->deltat = ((foo->Now + foo->lastUpdate) / 1000000.0f);
    }
    else{
        foo->deltat = ((foo->Now - foo->lastUpdate) / 1000000.0f);
    }
    foo->lastUpdate = foo->Now;
    foo->sum += foo->deltat;
    foo->sumCount++;
}

/*
 * Comprobacion del Magnetometro (AK8963), comprobamos si esta preparado para darnos un dato,
 * si esta preparado nos nos dara un 0x02, de lo contrario no podremos acceder al dato del
 * Magnetometro. Tenemos que tener introducir el handle del "i2c" i el buffer que queremos
 * comprobar "st1"
 *      Modulos utilizados: EUSCI B1 (I2C)
 */
int mpu_I2c_CheckMag(I2C_Handle handle, uint8_t *data){
    uint8_t             txBuffer[1];
    I2C_Transaction     i2cTransaction;

    i2cTransaction.slaveAddress = Board_AK8963;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.readBuf = data;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readCount = 1;

    txBuffer[0] = AK8963_ST1; // data ready status bit 0, 0x02

    I2C_transfer(handle, &i2cTransaction);
    return *data & 0x01;
}

/*
 * Lectura y limpieza del registro de estado del magnetometro, es necesario limpiar el registro
 * de estado para obtener cuando sea necesario. Tenemos que tener introducir el handle del "i2c"
 * y el buffer que queremos leer y limpiar "st2"
 *      Modulos utilizados: EUSCI B1 (I2C)
 */
int mpu_I2c_ReadAndClearMagStatusRegister(I2C_Handle handle, uint8_t *data){
    uint8_t             txBuffer[1];
    I2C_Transaction     i2cTransaction;

    i2cTransaction.slaveAddress = Board_AK8963;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.readBuf = data;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readCount = 1;

    txBuffer[0] = AK8963_ST2;

    I2C_transfer(handle, &i2cTransaction);
    return 1;
}

/*
 * Escritura en el sensor inercial, es necesario escribir en los registros del sensor
 * para configurarlo a nuestro antojo. Tenemos que tener introducir el handle del "i2c",
 * la direccion de esclavo donde escribiremos (MPU6050 o AK8963), el registro donde
 * queremos escribir y por ultimo el valor que queremos escribir en ese registro.
 *      Modulos utilizados: EUSCI B1 (I2C)
 */
int writeRegister(I2C_Handle handle, uint8_t slaveAddress, uint8_t regAddr, uint8_t value){
    uint8_t             txBuffer[4];
    I2C_Transaction     i2cTransaction;

    i2cTransaction.slaveAddress = slaveAddress;
    /* Write to a 16-bit status register */
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 2;
    i2cTransaction.readCount = 0;

    txBuffer[0] = regAddr;      // ADDR
    txBuffer[1] = value;        // VALUE

    if (!I2C_transfer(handle, &i2cTransaction)) {
        GPIO_write(Board_LED0, Board_LED_OFF);
        GPIO_write(Board_LED1, Board_LED_OFF);
        GPIO_write(Board_LED2, Board_LED_OFF);
        System_abort("Fallo en la escritura a la IMU!");
        System_flush();
        return 0;
    }
    else {return 1;}
}

int readRegister(I2C_Handle handle, uint8_t slaveAddress, uint8_t regAddr){

    uint8_t data;
    readTransaction(handle, slaveAddress, &regAddr, 1, &data, 1);
    return data;

}

void readRegisters(I2C_Handle handle, uint8_t slaveAddress, uint8_t regAddr, uint8_t count, uint8_t *dest){
    readTransaction(handle, slaveAddress, &regAddr, 1, dest, count);
}

bool readTransaction(I2C_Handle handle, uint8_t slaveAddress, uint8_t *txBuffer, uint8_t txSize, uint8_t *rxBuffer, uint8_t rxSize){

    I2C_Transaction     i2cTransaction;

    i2cTransaction.slaveAddress = slaveAddress;
    /* Write to a 16-bit status register */
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.writeCount = txSize;
    i2cTransaction.readCount = rxSize;

    if (!I2C_transfer(handle, &i2cTransaction)) {
        System_abort("Fallo en la lectura!");
        return false;
    }
    else { return true;}
}
