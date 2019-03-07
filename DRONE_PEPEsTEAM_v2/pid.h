/*
 * pid.h
 *
 *  Created on: 12 ene. 2019
 *      Author: PEPEs TEAM
 */

#ifndef PID_H_
#define PID_H_


PWM_Params  params;                 // Estructura con los parametros del PWM
PWM_Handle  propeller1_p27;         // P2.4 (-) ROLL
PWM_Handle  propeller2_p24;         // p2.7 (-) ROLL
PWM_Handle  propeller3_p25;         // P2.5 (+) PITCH
PWM_Handle  propeller4_p26;         // P2.6 (+) PITCH


uint16_t    pwmPeriod = 20000;      // Periodo 20000 us (50 Hz)
float       PID_Y, PID_X,  PID_Z;   // Compensación en Y (roll) y en X (pitch) y en Z (yaw)
float       THRO = 1270;            // Altura, THRO (Throttle) = 1290 - Variable a modificar por el mando RF
float       duty = 1000;            // duty para la rampa
float       duty1 = 1000;           // duty para el P2.4-motor1 (-) ROLL
float       duty2 = 1000;           // duty para el P2.5-motor3 (+) PITCH
float       duty3 = 1000;           // duty para el P2.6-motor4 (+) PITCH
float       duty4 = 1000;           // duty para el P2.7-motor2 (-) ROLL

// error: es la diferencia entre el valor desitjat i el real, i per calcular l'error hem de guardar l'ultim error (las_error)
float       last_error_y = 0, last_error_x = 0, last_error_z = 0;
float       error_y = 0, error_x = 0, error_z;
float       ref_roll = 0 , ref_pitch = 0, ref_yaw = 0;          // Variables de la funcio - Variable a modificar por el mando RF
float       descompensation = 10;
float       derivative_y, integral_y, proporcional_y;
float       derivative_x, integral_x, proporcional_x;
float       derivative_z, integral_z, proporcional_z;

float       KP_y = 0.02, KI_y = 0.00008, KD_y = 28;  // ROLL
float       KP_x = 0.02, KI_x = 0.00008, KD_x = 28;  // PITCH
unsigned char login;

float       KP_z = 1, KI_z = 0, KD_z = 0;       // No se han comprobado
uint32_t    time_prev, time_act;                // Tiempo previo, al iniciar los PWMs y Tiempo actual, al inicial el calculo
double      dt;

////// VARIABLES para el cualculo del PID /////////

float               error_M1 = 1.024;    // 1.024
float               error_M2 = 1.022;    // 1.026
float               error_M3 = 1;
float               error_M4 = 1.052;    // 1.054

unsigned int up;
unsigned int down;

#endif /* PID_H_ */
