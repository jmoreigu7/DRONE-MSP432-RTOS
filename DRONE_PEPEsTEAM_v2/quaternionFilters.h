/*
 * quaternionFilters.h
 *
 *  Created on: 20 may. 2018
 *      Author: Jefferson
 */

#ifndef QUATERNIONFILTERS_H_
#define QUATERNIONFILTERS_H_

//#include <arm_math.h>

#define Kp 2.0f * 5.0f
#define Ki 0.0f
#define PI 3.1415926f
#define DEG_TO_RAD 	PI/180.0f
#define RAD_TO_DEG 	180.0f/PI

void MadgwickQuaternionUpdate(float ax, float ay, float az,
                              float gx, float gy, float gz,
                              float mx, float my, float mz, float deltat);

void MahonyQuaternionUpdate(float ax, float ay, float az,
                            float gx, float gy, float gz,
                            float mx, float my, float mz, float deltat);

const float * getQ();


#endif /* QUATERNIONFILTERS_H_ */
