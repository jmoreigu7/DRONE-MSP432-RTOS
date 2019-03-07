/*
 * mpu9250.h
 *
 *  Created on: 20 may. 2018
 *      Author: PEPEs TEAM
 */

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>

#ifndef _mpu9250_h_
#define _mpu9250_h_

I2C_Handle                      i2c;
I2C_Params                      i2cParams;
I2C_Transaction                 i2cTransaction;

unsigned char                   st1[10];
unsigned char                   st2[10];

uint8_t                         bufferclear;
uint8_t                         magn;
uint8_t                         whomagn;
uint8_t                         whompu;

#define GYRO_SCALE              GFS_1000DPS
#define ACCEL_SCALE             AFS_4G
#define MAG_SCALE               MFS_16BITS
#define MAG_MODE_8Hz            0x02 // 100Hz
#define MAG_MODE_100Hz          0x06 // 100Hz
#define MPU9250_ADDRESS         0x68
#define AK8975_ADDRESS         0x0C
#define MPU9250_ID              0x73 // o 71, depende del fabricante
#define AK8975_ID               0x48

// See also MPU-9250 Register Map and Descriptions, Revision 4.0,
// RM-MPU-9250A-00, Rev. 1.4, 9/9/2013 for registers not listed in above
// document; the MPU9250 and MPU9150 are virtually identical but the latter has
// a different register map

// SelfTest Gyroscop ==========================================================
#define SELF_TEST_X_GYRO        0x00
#define SELF_TEST_Y_GYRO        0x01
#define SELF_TEST_Z_GYRO        0x02
#define SELF_TEST_X_ACCEL       0x0D
#define SELF_TEST_Y_ACCEL       0x0E
#define SELF_TEST_Z_ACCEL       0x0F

#define XG_OFFSET_H             0x13  // User-defined trim values for gyroscope
#define XG_OFFSET_L             0x14
#define YG_OFFSET_H             0x15
#define YG_OFFSET_L             0x16
#define ZG_OFFSET_H             0x17
#define ZG_OFFSET_L             0x18

#define SMPLRT_DIV              0x19
#define CONFIG                  0x1A
#define GYRO_CONFIG             0x1B
#define ACCEL_CONFIG            0x1C
#define ACCEL_CONFIG2           0x1D
#define LP_ACCEL_ODR            0x1E
#define WOM_THR                 0x1F

#define FIFO_EN                 0x23
#define I2C_MST_CTRL            0x24
#define I2C_SLV0_ADDR           0x25
#define I2C_SLV0_REG            0x26
#define I2C_SLV0_CTRL           0x27
#define I2C_SLV1_ADDR           0x28
#define I2C_SLV1_REG            0x29
#define I2C_SLV1_CTRL           0x2A
#define I2C_SLV2_ADDR           0x2B
#define I2C_SLV2_REG            0x2C
#define I2C_SLV2_CTRL           0x2D
#define I2C_SLV3_ADDR           0x2E
#define I2C_SLV3_REG            0x2F
#define I2C_SLV3_CTRL           0x30
#define I2C_SLV4_ADDR           0x31
#define I2C_SLV4_REG            0x32
#define I2C_SLV4_DO             0x33
#define I2C_SLV4_CTRL           0x34
#define I2C_SLV4_DI             0x35
#define I2C_MST_STATUS          0x36

#define INT_PIN_CFG             0x37
#define INT_ENABLE              0x38
#define DMP_INT_STATUS          0x39  // Check DMP interrupt
#define INT_STATUS              0x3A

#define ACCEL_XOUT_H            0x3B
#define ACCEL_XOUT_L            0x3C
#define ACCEL_YOUT_H            0x3D
#define ACCEL_YOUT_L            0x3E
#define ACCEL_ZOUT_H            0x3F
#define ACCEL_ZOUT_L            0x40

#define TEMP_OUT_H              0x41
#define TEMP_OUT_L              0x42

#define GYRO_XOUT_H             0x43
#define GYRO_XOUT_L             0x44
#define GYRO_YOUT_H             0x45
#define GYRO_YOUT_L             0x46
#define GYRO_ZOUT_H             0x47
#define GYRO_ZOUT_L             0x48

#define EXT_SENS_DATA_00        0x49
#define EXT_SENS_DATA_01        0x4A
#define EXT_SENS_DATA_02        0x4B
#define EXT_SENS_DATA_03        0x4C
#define EXT_SENS_DATA_04        0x4D
#define EXT_SENS_DATA_05        0x4E
#define EXT_SENS_DATA_06        0x4F
#define EXT_SENS_DATA_07        0x50
#define EXT_SENS_DATA_08        0x51
#define EXT_SENS_DATA_09        0x52
#define EXT_SENS_DATA_10        0x53
#define EXT_SENS_DATA_11        0x54
#define EXT_SENS_DATA_12        0x55
#define EXT_SENS_DATA_13        0x56
#define EXT_SENS_DATA_14        0x57
#define EXT_SENS_DATA_15        0x58
#define EXT_SENS_DATA_16        0x59
#define EXT_SENS_DATA_17        0x5A
#define EXT_SENS_DATA_18        0x5B
#define EXT_SENS_DATA_19        0x5C
#define EXT_SENS_DATA_20        0x5D
#define EXT_SENS_DATA_21        0x5E
#define EXT_SENS_DATA_22        0x5F
#define EXT_SENS_DATA_23        0x60

#define MOT_DETECT_STATUS       0x61
#define I2C_SLV0_DO             0x63
#define I2C_SLV1_DO             0x64
#define I2C_SLV2_DO             0x65
#define I2C_SLV3_DO             0x66
#define I2C_MST_DELAY_CTRL      0x67
#define SIGNAL_PATH_RESET       0x68
#define MOT_DETECT_CTRL         0x69
#define USER_CTRL               0x6A  // Bit 7 enable DMP, bit 3 reset DMP
#define PWR_MGMT_1              0x6B  // Device defaults to the SLEEP mode
#define PWR_MGMT_2              0x6C
#define DMP_BANK                0x6D  // Activates a specific bank in the DMP
#define DMP_RW_PNT              0x6E  // Set read/write pointer to a specific start address in specified DMP bank
#define DMP_REG                 0x6F  // Register in DMP from which to read or to which to write
#define DMP_REG_1               0x70
#define DMP_REG_2               0x71
#define FIFO_COUNTH             0x72
#define FIFO_COUNTL             0x73
#define FIFO_R_W                0x74
#define WHO_AM_I_MPU9250        0x75 // return 0x71 or 0x73, depende del distribuidor

#define XA_OFFSET_H             0x77 // User-defined trim values for accelerometer
#define XA_OFFSET_L             0x78
#define YA_OFFSET_H             0x7A
#define YA_OFFSET_L             0x7B
#define ZA_OFFSET_H             0x7D
#define ZA_OFFSET_L             0x7E

// Duration counter threshold for motion interrupt generation, 1 kHz rate,
// LSB = 1 ms
#define MOT_DUR                 0x20
// Zero-motion detection threshold bits [7:0]
#define ZMOT_THR                0x21
// Duration counter threshold for zero motion interrupt generation, 16 Hz rate,
// LSB = 64 ms
#define ZRMOT_DUR               0x22

// Magnetometer Registers (AK8963 Compass) =====================================
#define WHO_AM_I_AK8963         0x00 // ID del dispositivo, returna 0x48
#define INFO                    0x01 // Informacion
#define AK8963_ST1              0x02 // Estado 1, data ready status bit 0

#define AK8963_XOUT_L           0x03
#define AK8963_XOUT_H           0x04
#define AK8963_YOUT_L           0x05
#define AK8963_YOUT_H           0x06
#define AK8963_ZOUT_L           0x07
#define AK8963_ZOUT_H           0x08

#define AK8963_ST2              0x09  // Estado 2, data overflow bit 3 and data read error status bit 2
#define AK8963_CNTL             0x0A  // Power down (0000), single-measurement (0001), self-test (1000) and Fuse ROM (1111) modes on bits 3:0
#define AK8963_ASTC             0x0C  // Self test control

#define AK8963_I2CDIS           0x0F  // I2C disable
#define AK8963_ASAX             0x10  // Fuse ROM x-axis sensitivity adjustment value
#define AK8963_ASAY             0x11  // Fuse ROM y-axis sensitivity adjustment value
#define AK8963_ASAZ             0x12  // Fuse ROM z-axis sensitivity adjustment value

// Settings of AK8975 Compass ==================================================
// Operation Mode
#define AK9875_POWER_DOWN           0x00
#define AK9875_SINGLE_MEASUREMENT   0x01
#define AK9875_SELF_TEST            0x08
#define AK9875_FUSE_ROM_ACCES       0x0F

enum Ascale {
  AFS_2G = 0,
  AFS_4G,
  AFS_8G,
  AFS_16G
};

enum Gscale {
  GFS_250DPS = 0,
  GFS_500DPS,
  GFS_1000DPS,
  GFS_2000DPS
};

enum Mscale {
  MFS_14BITS = 0, // 0.6 mG per LSB
  MFS_16BITS      // 0.15 mG per LSB
};

typedef struct {
    // Especificaciones del fondo de escala del sensor
    // Resolucion del giroscopio 250 DPS, 500 DPS, 1000 DPS, and 2000 DPS
    uint8_t Gscale;
    // Resolucion del accelerometro 2 Gs, 4 Gs, 8 Gs, and 16 Gs
    uint8_t Ascale;
    // Resolucion del magnetometro en 14 o 16 bits
    uint8_t Mscale;
    // 2 para 8 Hz, 6 para 100 Hz datos del magnetometro en constante lectura
    uint8_t Mmode;

    // Angulos de Euler - Telemetria del cuadricopeto
    float pitch, yaw, roll;
    // Diferencial de tiempo para el calculo de
    uint32_t delt_t;
    // Se utiliza para controlar la velocidad de salida de datos por pantalla
    uint32_t count, sumCount;
    float deltat, sum;  // Intervalo de integracion para los dos filtro
    // Se utiliza para calcular el intervalo de integracion
    uint32_t lastUpdate, firstUpdate;
    // Se utiliza para calcular el intervalo de integracion
    uint32_t Now;


    // Resoluciones de escala por LSB para los sensores
    float aRes, gRes, mRes;
    // Variables para mantener los ultimos valores de los datos de los sensores
    float ax, ay, az, gx, gy, gz, mx, my, mz;
    // Calibración del mag de fabrica y polarizacion magnetica (mag bias)
    float magCalibration[3], magbias[3];
    // Correcciones de bias para giroscopios y acelerómetros
    float gyroBias[3], accelBias[3];
    // Datos de autotest
    float SelfTest[6];
    // Almacena la salida del sensor de acelerometro firmado de 16 bits
    int16_t accelCount[3];
    // Almacena la salida del sensor de giroscopio firmado de 16 bits
    int16_t gyroCount[3];
    // Almacena la salida del sensor de magnetometro firmado de 16 bits
    int16_t magCount[3];

}mpu9250;

void init_struct(mpu9250*);
void getAres(mpu9250*);
void getGres(mpu9250*);
void getMres(mpu9250*);

void MPU6050SelfTest(I2C_Handle handle, float *destination);
void MPU6050calibrate(I2C_Handle handle, float * gyroBias, float * accelBias);
void MPU6050init(I2C_Handle handle);
void AK8963init(I2C_Handle handle);

int readAccel(I2C_Handle handle, int16_t *);
int readGyro(I2C_Handle handle, int16_t *);
int readMagn(I2C_Handle handle, int16_t *);

void setAccelData(I2C_Handle handle, mpu9250*);
void setGyroData(I2C_Handle handle, mpu9250*);
void setMagnData(I2C_Handle handle, mpu9250*);

void updateTime(mpu9250*);

int mpu_I2c_CheckMag(I2C_Handle handle, uint8_t *data);
int mpu_I2c_ReadAndClearMagStatusRegister(I2C_Handle handle, uint8_t *data);

int writeRegister(I2C_Handle handle, uint8_t slaveAddress, uint8_t regAddr, uint8_t value);
int readRegister(I2C_Handle handle, uint8_t slaveAddress, uint8_t regAddr);
void readRegisters(I2C_Handle handle, uint8_t slaveAddress, uint8_t regAddr, uint8_t count, uint8_t *dest);
bool readTransaction(I2C_Handle handle, uint8_t slaveAddress, uint8_t *txBuffer, uint8_t txSize, uint8_t *rxBuffer, uint8_t rxSize);

#endif
