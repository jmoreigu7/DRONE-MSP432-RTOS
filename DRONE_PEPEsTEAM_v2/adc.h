/*
 * adc.h
 *
 *  Created on: 12 ene. 2019
 *      Author: PEPEs TEAM
 */

#ifndef ADC_H_
#define ADC_H_


#define ADC_SAMPLE_COUNT  (10)      // ADC sample count
uint16_t adcValue0;                 //ADC conversion result variables

//void intochar(int numero){
//
//}

//void intochar(int number)
//{
//    char neg=('-');
//    char value[10];
//    int i=0;
//
//    if(number>0)
//    {
//        do
//              {
//                    value[i++] = (char)(number % 10) + '0'; //convert integer to character
//                    number /= 10;
//              } while(number);
//
//        while(i)
//        {
////            printCaracter(value[--i]);
//            value[--i];
//        }
//    }
//    else
//    {
//        do
//        {
//            number=abs(number);
//            value[i++] = (char)(number % 10) + '0'; //convert integer to character
//            number /= 10;
//        } while(number);
////        printCaracter(neg);
//
//        while(i)
//        {
////        printCaracter(value[--i]);
//            neg+value[--i];
//        }
//    }
//}

#endif /* ADC_H_ */
