/*
 *  ====================== PEPEs TEAM ======================
 *  Universidad de Barcelona 2018-2019
 *  Grado de Ingenieria - Electronica de Telecomunicaciones
 *  Laboratorio de Sistemas Electronico II
 *  Autores: Alvaro Baucells Costa, Arnau Vicente Puiggros,
 *  Imanol Fernandez Moreno y Jefferson Moreira Guaycha
 */

#include "header.h"

//ADC_Handle   adc;
//ADC_Params   params_ADC;
//int_fast16_t res;

unsigned char temps;
float lim_max=1400;
float lim_min=1000;


//Clock
Clock_Struct clk0Struct;
Clock_Handle clk0Handle;

// Magnetic, Angular Rate and Gravity, sistema de medicion de gravedad, velocidad angular y magnetismo
Void MARGFxn(UArg arg0, UArg arg1);
// PWM
Void PROPELLERS();                //Funcio per configurar els pwm
// RF
Void nRF24_RXTaskFxn (UArg arg0, UArg arg1);

/*
 * ============== clk0Fxn ================
 * Esta funcion nos sirve para generar una
 * interrupcion cada 1s y poder canviar el
 * modula de RF de RX a TX. 
 */ 
Void clk0Fxn(UArg arg0, UArg arg1)
{
    temps = 1;  		//Ponemos a 1 el tiempo
}

/*
* ======== main ========
* El main nos sirve para iniciar todos los parametros del codigo
*/  
int main(void){

    Clock_Params clkParams;		//Parametros para configurar el clock
    temps=0;					//Iniciamos la varaible temps con valor 0

    /* Llamamos a la funciones de la Placa */
    Board_initGeneral();
    Board_initGPIO();       	// Puertos GPIOs
    Board_initI2C();        	// Comunicación sensor MPU9250
    Board_initPWM();        	// Power motores
    Board_initSPI();        	// Comunicación transceptor nRF24L01
    Board_initADC();        	// ADC para medir el nivel de la bateria

    /* Construimos la instancia del un clock periodico*/
    Clock_Params_init(&clkParams);
    clkParams.period = 1000000/Clock_tickPeriod;
    clkParams.startFlag = FALSE;

    Clock_construct(&clk0Struct,(Clock_FuncPtr)clk0Fxn,
                    1000000/Clock_tickPeriod, &clkParams);
                    
    clk0Handle = Clock_handle(&clk0Struct);					//handle del clock

    /* Al encender el LED0, sabemos que ha recorrido las tascas y todas las inicializaciones de funciones */
    GPIO_write(Board_LED0, Board_LED_ON);
    GPIO_write(Board_LED1, Board_LED_ON);
    GPIO_write(Board_LED2, Board_LED_ON);

    System_printf("Inicio del programa \n");
    
    /* SysMin will only print to the console when you call flush or exit */
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);			//Nunca llegamos a esta linea
}

/* =================================  TASKs  ========================================== */

/*
 * ==================== MARG TASK ======================
 * Extraemos los datos del sensor MPU9250 los cuales son procesados para obetener
 * los angulos de euler, para ello utilizaremos el algoritmo de fusión de datos
 * Mahony y Madgwick, al realizar varias pruebas hemos detectado que el filtro Madgwick
 * tiene más error que el filtro Mahony.
 *
 * Comunicación I2C a 400 KHz, pines utilizados:
 *      P6.5 - SCL
 *      P6.4 - SDA
 */
Void MARGFxn(UArg arg0, UArg arg1)
{
    System_printf("Inicialización I2C MSP-MPU9250 \n");
    System_flush();

    I2C_Handle          i2c;
    I2C_Params          i2cParams;

    float               offset_roll = -2.1;     	// Offset del sistema
    float               offset_pitch = -10.7;     	// Offset del sistema
    float               offset_yaw = 20;         	// Offset del sistema 

    Semaphore_pend(semMARG,BIOS_WAIT_FOREVER);  	//Hacemos el pend de semMARG. Dado que su valor inicial es 1 saltamos a la siguiente linea

  /*---------------- Config MPU9250 ---------------------------------------*/
    /* Create I2C for usage */
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz; 				// Velocidad de comunicación
    
    /*Abrimos la comunicación i2c, con el modulo EUSCI B1, pines 6.5 (SCL) 6.4 (SDA)*/
    i2c = I2C_open(Board_I2C_IMU_B1, &i2cParams);
    if (i2c == NULL) {System_abort("Error i2c \n");}
    else {System_printf("Establecida correctamente la conexión I2C \n");}

    mpu9250 my_MPU; 								// Llamamos a la estructura mpu9250
    
    /* Funcion inicio de la estructura */
    init_struct(&my_MPU); // El puntero my_MPU recorre la estructura mpu9250 con los valores de la funcion

    whompu = readRegister(i2c, Board_MPU6050, WHO_AM_I_MPU9250); // retorna 0x73 (o 0x71 dependiendo del fabricante)
//    System_printf("Realización del selftest del MPU6050 \n");
//    MPU6050SelfTest(i2c, my_MPU.SelfTest); // No es necesaria para nuestro peoyecto, pero si es necesaria a nivel de comprovación de estado del MPU
    
    Task_sleep(2); 									// Le damos tiempo al sensor
    System_printf("Calibración Giroscopio y Acelerómetro \n");
    System_flush();
    MPU6050calibrate(i2c, my_MPU.gyroBias, my_MPU.accelBias);

    System_printf("Inicialización del MPU6050 \n");
    MPU6050init(i2c);

    whomagn = readRegister(i2c, Board_AK8963, WHO_AM_I_AK8963); // retorna 0x48
    System_printf("Inicialización del AK8963 \n");
    AK8963init(i2c);

    System_printf("Inicialización correcta del I2C MSP-MPU9250!\n");
    System_flush();
  /*---------------- FIN Config MPU9250 ---------------------------------------*/
  
  /*---------------- Config PROPELLERS ---------------------------------------*/
    /*
     * PROPELLERS - Inicio de los motores. Duty cycle minimo para los motores (Brushless EMAX
     * 2213-935KV) de 1050 y maximo de 1500.
     * Periodo de 50 Hz (20000us), pines utilizados:
     *      P2.4 - PWM1 - propeller2_p26 (motor2) (-)
     *      P2.5 - PWM2 - propeller3_p24 (motor3) (+)
     *      P2.6 - PWM3 - propeller4_p24 (motor4) (+)
     *      P2.7 - PWM4 - propeller1_p27 (motor1) (-)
     *
     *      NOTA: (-) corresponde al primer eje (roll) y (+) al segundo eje (pitch)
     */
    System_printf("Inicialización PWM 'Propellers/Motores' \n");
    System_flush();
    PWM_Params_init(&params);
    params.dutyUnits = PWM_DUTY_US;         		// Expresion del duty en micro segundos
    params.dutyValue = 0;		
    params.periodUnits = PWM_PERIOD_US;     		// Expresion del periodo en micro segundos
    params.periodValue = pwmPeriod;         		// Periodo del PWM: 50 Hz

    /* Primer eje PID_Y, ROLL */
    propeller1_p27 = PWM_open(Board_PWM4, &params);
    if (propeller1_p27 == NULL) {System_abort("Fallo con el PWM4 del P2.7 (-)");}
    else {System_printf("Motor1 (-) PWM4-P2.7 \n");}

    propeller2_p24 = PWM_open(Board_PWM1, &params);
    if (propeller2_p24 == NULL) {System_abort("Fallo con el PWM1 del P2.4 (-)");}
    else {System_printf("Motor2 (-) PWM1-P2.4 \n");}

    /* Segundo eje PID_X, PITCH */
    propeller3_p25 = PWM_open(Board_PWM2, &params);
    if (propeller3_p25 == NULL) {System_abort("Fallo con el PWM2 del P2.5 (+)");}
    else {System_printf("Motor3 (+) PWM2-P2.5 \n");}

    propeller4_p26 = PWM_open(Board_PWM3, &params);
    if (propeller4_p26 == NULL) {System_abort("Fallo con el PWM3 del P2.6 (+)");}
    else {System_printf("Motor4 (+) PWM3-P2.6 \n");}
    System_flush();

    /* Iniciamos los propellers*/
    PWM_start(propeller1_p27);
    PWM_start(propeller2_p24);

    PWM_start(propeller3_p25);
    PWM_start(propeller4_p26);
    
  /*---------------- FIN Config PROPELLERS ---------------------------------------*/


    time_prev = Clock_getTicks();					// Referencia temporal previa, justo al empezar
													// los motores para realizar la integracion y la derivacion


    /*
     * ======================== RAMPA ===========================
     * Utilizamos una rampa para realizar una aceleracion progresiva
     * No es recomendable darle un duty cycle grande en el principio, lo mas
     * aconsegable es realizar una rampa progresiva para que los motores respondan
     * de manera correcta. La rampa no ha de superar los 1450 de duty cycle.
     * Pare debugear sin motores es recomendable comentar la rampa, ya que es
     * tiempo de ejecucion perdido.
     *
     */
    for (duty = 1080; duty < 1150; duty += 10){     
        PWM_setDuty(propeller1_p27, duty*error_M1);
        PWM_setDuty(propeller2_p24, duty*error_M2);

        PWM_setDuty(propeller3_p25, duty*error_M3); 
        PWM_setDuty(propeller4_p26, duty*error_M4);
        Task_sleep(800);
    }
    
    /* FIN RAMPA */
    
    /* Asignamos los valores finales de la rampa a su correspondiente variable*/
    duty1 = duty*error_M1;  duty2 = duty*error_M2;
    duty3 = duty*error_M3;  duty4 = duty*error_M4;

    login = 1;										// Ponemos a 1 la variable login			
    
    Semaphore_post(semRF);      					// Post del semRF. Ponemos el verde semRF    
    Semaphore_pend(semMARG, BIOS_WAIT_FOREVER); 	// Pend de semMARG. Ponemos en rojo semMARG
													// Esperamos a que llegue un 'UP' en RX
    
    /* Inciamos el bucle infinito*/
    while(1) {

        if(up == 1 && login == 1){

            for (duty = 1140; duty < 1250; duty += 10){ // hasta 1250

                PWM_setDuty(propeller1_p27, duty*error_M1);
                PWM_setDuty(propeller2_p24, duty*error_M2);

                PWM_setDuty(propeller3_p25, duty*error_M3);
                PWM_setDuty(propeller4_p26, duty*error_M4);
                Task_sleep(800);
            }
            up    = 0;
            duty1 = duty*error_M1;  duty2 = duty*error_M2;
            duty3 = duty*error_M3;  duty4 = duty*error_M4;

            login = 0;
        }

        if(down == 1){
            for (duty = 1250; duty > 1150; duty -= 10){
                PWM_setDuty(propeller1_p27, duty*error_M1);
                PWM_setDuty(propeller2_p24, duty*error_M2);

                PWM_setDuty(propeller3_p25, duty*error_M3);
                PWM_setDuty(propeller4_p26, duty*error_M4);
                Task_sleep(800);
            }
            down  = 0;
            THRO  = 0;
            duty1 = duty*error_M1;  duty2 = duty*error_M2;
            duty3 = duty*error_M3;  duty4 = duty*error_M4;

            login = 1;
        }
	
	/*----------EXTRACCION DE DATOS MPU9250------------------------------------*/
	
        // Extraemos los datos mediante la estructura mpu9250 (mu_MPU)
        setAccelData(i2c, &my_MPU); //  Introducimos los valores del accelerometro en la estructura my_MPU
        setGyroData(i2c, &my_MPU); //  Introducimos los valores del gyroscopio en la estructura my_MPU
        
        /* Para la lecura del magnetometro es necesario comprovar que se puede hacer la lectura, registro AK8963_ST1 */
        if(mpu_I2c_CheckMag(i2c, st1)){ // Comprobamos que podemos hacer una lectura en el magnetometro
        setMagnData(i2c, &my_MPU); //  Introducimos los valores del magnetometro en la estructura my_MPU
        mpu_I2c_ReadAndClearMagStatusRegister(i2c, st2); // Limpiamos la flag del estado del magnetometro para realizar otra lectura
        }

        updateTime(&my_MPU);
        
        // Empiza el algoritmo de fusion de datos para la estimacion de la orientacion: Madgwick
        // https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
        MahonyQuaternionUpdate(my_MPU.ax, my_MPU.ay, my_MPU.az,
                                 my_MPU.gx*DEG_TO_RAD, my_MPU.gy*DEG_TO_RAD, my_MPU.gz*DEG_TO_RAD,
                                 my_MPU.mx, my_MPU.my,  my_MPU.mz, my_MPU.deltat);
                                 
        // Ecuaciones: angulos de euler, mediante quaterniones
        // http://sparkfun28.rssing.com/chan-8070788/all_p25.html
        // http://www.ngdc.noaa.gov/geomag-web/#declination
        my_MPU.yaw   = atan2(2.0f * (*(getQ()+1) * *(getQ()+2) + *getQ() * *(getQ()+3)),
                                     *getQ() * *getQ() + *(getQ()+1) * *(getQ()+1) - *(getQ()+2) * *(getQ()+2) - *(getQ()+3) * *(getQ()+3));
        my_MPU.pitch = -asin(2.0f * (*(getQ()+1) * *(getQ()+3) - *getQ() * *(getQ()+2)));
        my_MPU.roll  = atan2(2.0f * (*getQ() * *(getQ()+1) + *(getQ()+2) * *(getQ()+3)),
                             *getQ() * *getQ() - *(getQ()+1) * *(getQ()+1) - *(getQ()+2) * *(getQ()+2) + *(getQ()+3) * *(getQ()+3));

        my_MPU.yaw 		= (my_MPU.yaw*RAD_TO_DEG+offset_yaw);
        my_MPU.pitch  	= (my_MPU.pitch*RAD_TO_DEG+offset_pitch);
        my_MPU.roll  	= (my_MPU.roll*RAD_TO_DEG+offset_roll);

        my_MPU.sumCount = 0;
        my_MPU.sum = 0;
        
	/*----------FIN EXTRACCION DE DATOS MPU9250------------------------------------*/

    /*---------------------- PID --------------------------------------------- */
        time_act = Clock_getTicks(); // Referencia temporal actual
        dt = time_act - time_prev; // Diferencial de tiempo
        dt = dt / 1000;
        /*
         * Error con referencia 0 grados (pitch y roll), incorporar el mando una buena manera seria
         * variar la variable de referencia.
         */
        error_y = ref_roll - my_MPU.roll;
        error_x = ref_pitch - my_MPU.pitch;
        error_z = ref_yaw - my_MPU.yaw;

        // Realizamos el calculos de la integral, derivativa y proporcional para el control PID
        integral_y = integral_y + error_y*dt;
        integral_x = integral_x + error_x*dt;
        integral_z = integral_z + error_z*dt;

        derivative_y = (error_y - last_error_y)/dt;
        derivative_x = (error_x - last_error_x)/dt;
        derivative_z = (error_z - last_error_z)/dt;

        proporcional_y = error_y;
        proporcional_x = error_x;
        proporcional_z = error_z;

        //Calculem el PID, entero para realizar mejora los incremetos de duty
        PID_Y = (int) (KP_y*proporcional_y + KI_y*integral_y + KD_y*derivative_y);
        PID_X = (int) (KP_x*proporcional_x + KI_x*integral_x + KD_x*derivative_x);
        PID_Z = (int) (KP_z*proporcional_z + KI_z*integral_z + KD_z*derivative_z);

        last_error_y = error_y; // Registramos el ultimo error
        last_error_x = error_x;
        last_error_z = error_z;

        // Incrementos del duty con la compensacion del PID_Y (roll), PID_X (pitch) y  THRO (throttle)
		// https://robotics.stackexchange.com/questions/2964/quadcopter-pid-output?rq=1

        duty1 = THRO - PID_Y;   //PID_Y - PID_X;     // P2.4 / motor 1 (-)
        duty2 = THRO + PID_Y;   //PID_X + PID_Y;     // P2.5 / motor 3 (+)
        duty3 = THRO + PID_X;   //PID_X + PID_Y;     // P2.6 / motor 4 (+)
        duty4 = THRO - PID_X;   //PID_Y - PID_X;     // P2.7 / motor 2 (-)

        // Limites del roll (PID_Y) de pitch (PID_X)
        if(PID_Y > 40){PID_Y  = 40;}
        if(PID_Y < -40){PID_Y = -40;}

        if(PID_X > 40){PID_X  = 40;}
        if(PID_X < -40){PID_X = -40;}

        // Limites del duty en los motores, potencia
        if(duty1 > lim_max*error_M1){duty1 = lim_max*error_M1;}
        if(duty1 < lim_min*error_M1){duty1 = lim_min*error_M1;}
        if(duty2 > lim_max*error_M2){duty2 = lim_max*error_M2;}
        if(duty2 < lim_min*error_M2){duty2 = lim_min*error_M2;}

        if(duty3 > lim_max*error_M3){duty3 = lim_max*error_M3;}
        if(duty3 < lim_min*error_M3){duty3 = lim_min*error_M3;}
        if(duty4 > lim_max*error_M4){duty4 = lim_max*error_M4;}
        if(duty4 < lim_min*error_M4){duty4 = lim_min*error_M4;}

        // Realizamos la compensacion en los motores/propellers
        PWM_setDuty(propeller1_p27, duty1*error_M1);              // P2.4 (-)
        PWM_setDuty(propeller2_p24, duty2*error_M2);              // P2.7 (-)

        PWM_setDuty(propeller3_p25, duty3*error_M3);              // P2.5 (+)
        PWM_setDuty(propeller4_p26, duty4*error_M4);              // P2.6 (+)
        
	/*-------------------FIN PID --------------------------------------------- */

    }
}

/*
 * ==================== RF TASK ======================
 * Configuramos el modulo de RF.
 * Si recibimos un dato lo procesamos, en caso contrario si
 * se ha producido la interrupcion del clock i tiempo=1,
 * enviamos un dato con informacion del sistema.
 */
Void nRF24_RXTaskFxn (UArg arg0, UArg arg1)
{
    ADC_Handle   adc;
    ADC_Params   params_ADC;

    //Afegit
    ADC_Params_init(&params_ADC);
    adc = ADC_open(Board_ADC0, &params_ADC); //DEBUG: FALLA
    volatile long int conversion, conversion_2, conversion_3, conversion_4, conversion_5;
    volatile unsigned int primero, dif, segundo, tercero, cuarto;
    volatile unsigned int primero_2, dif_2, segundo_2, tercero_2, cuarto_2;
    volatile unsigned int primero_3, dif_3, segundo_3, tercero_3, cuarto_3;
    volatile unsigned int primero_4, dif_4, segundo_4, tercero_4, cuarto_4;
    volatile unsigned int primero_5, dif_5, segundo_5, tercero_5, cuarto_5;
    //Fi afegit
    SPI_Params nrf24SpiParams;

    // semafor
    // pend del codi RF
    Semaphore_pend(semRF, BIOS_WAIT_FOREVER);   //1er pend de semRF. Espera a inici MARG

    // Initialize SPI
    SPI_Params_init(&nrf24SpiParams);

    nrf24SpiParams.transferMode = SPI_MODE_BLOCKING;
    nrf24SpiParams.transferCallbackFxn = NULL;
    nrf24SpiParams.mode = SPI_MASTER;
    nrf24SpiParams.bitRate = 3000000;   // 1MHz SPI clock
    nrf24SpiParams.frameFormat= SPI_POL0_PHA0;
    nrf24SpiParams.dataSize = 8;

    user = 0xFE;
    rf_crc = RF24_EN_CRC | RF24_CRCO; // CRC enabled, 16-bit
    rf_addr_width      = 5;
    rf_speed_power     = RF24_SPEED_2MBPS | RF24_POWER_0DBM;
    rf_channel         = 120;

    nrf24Spi = SPI_open(Board_SPI0, &nrf24SpiParams);
    if (nrf24Spi == NULL) {System_abort("Error initializing SPI\n");}
    else {System_printf("SPI initialized\n");}

    msprf24_init();

    msprf24_set_pipe_packetsize(0, 32);
    msprf24_open_pipe(0, 1);  // Open pipe#0 with Enhanced ShockBurst

    // Set our RX address
    addr[0] = 0xDC; addr[1] = 0xBA; addr[2] = 0xAB; addr[3] = 0xCD; addr[4] = 0x00;

    w_tx_addr(addr);
    w_rx_addr(0, addr);

    //COMU
    Clock_start(clk0Handle); //Iniciem el clock

    // Receive mode
    if (!(RF24_QUEUE_RXEMPTY & msprf24_queue_state())) {flush_rx();}

    msprf24_activate_rx();
    dato = r_reg(RF24_RF_SETUP);

    if (dato == 0x0E)
    {
        //semafor
        //post del while de la imu

        msprf24_activate_rx();      //prepara per rebre. Mode RX

        /* Inicio while*/
        while (1){

            /* Si hemos recibido un dato entramos en la condicion */
            if (rf_irq & RF24_IRQ_RX || msprf24_rx_pending())
            {
                r_rx_payload(32, bufrf);
                msprf24_irq_clear(RF24_IRQ_RX);
                user = bufrf[0];

                if (bufrf[0] == 'R'){ // && bufrf[1] == 'I'){        // R I G H T    (ROLL +)
                    ref_roll = descompensation;
                    ref_pitch = 0;
                    GPIO_write(Board_LED1, Board_LED_OFF);
                    GPIO_write(Board_LED0, Board_LED_OFF);
                }
                if (bufrf[0] == 'L'){ // && bufrf[1] == 'E'){        // L E F T      (ROLL -)
                    ref_roll = -descompensation;
                    ref_pitch = 0;
                    GPIO_write(Board_LED1, Board_LED_ON);
                    GPIO_write(Board_LED0, Board_LED_ON);
                }
                // FORDWARE
                if (bufrf[0] == 'F'){
                    ref_roll = 0;
                    ref_pitch = descompensation;
                }
                // BACK
                if (bufrf[0] == 'B'){
                    ref_roll = 0;
                    ref_pitch = -descompensation;
                }
                if (bufrf[0] == 'U'){ // && bufrf[3] == 'P'){        // UP           (PITCH +)

                    ref_roll = 0; // THRO = 1050;
                    ref_pitch = 0;

                    up = 1;

                    GPIO_write(Board_LED1, Board_LED_ON);
                    GPIO_write(Board_LED0, Board_LED_OFF);

                    Semaphore_post(semMARG);    //Activa while MARG
                 }
                if (bufrf[0] == 'D'){ // && bufrf[3] == 'O'){        // D O W N      (PITCH-)
                    ref_roll = 0; // THRO = 1050;
                    ref_pitch = 0;

                    down = 1;

                    GPIO_write(Board_LED1, Board_LED_ON);
                    GPIO_write(Board_LED0, Board_LED_OFF);
                 }
                if (bufrf[0] == 'O' ){                        // O K! ESTABLE
                    ref_roll = 0;
                    ref_pitch = 0;
                    GPIO_write(Board_LED1, Board_LED_OFF);
                    GPIO_write(Board_LED0, Board_LED_ON);
                }
                if (bufrf[0] == 'S'){ // && bufrf[1] == 'T'){        // S T O P
                    ref_roll = 0;
                    ref_pitch = 0;
                    THRO = 0;
                    GPIO_write(Board_LED1, Board_LED_OFF);
                    GPIO_write(Board_LED0, Board_LED_OFF);
                    login = 1;
                }
            }   //if captura RX
            
            /* Si NO hemos recibido un dato*/
            else{
				
				/* Si se ha producido la interrupcion del clock la variable temps=1
				 * y estamos listos para enviar informacion al mando de control
				 */
                if(temps){
                    temps=0;        				// Reseteamos el tiempo

                    //Codi enviar
                    memset(buf, 0, sizeof(buf));    // Borra el contenido de la variable buf

                    /*ADC*/
                    ADC_convert(adc, &adcValue0); 	// valor en adcValue0
                    
                    conversion = (long int)duty1;
                    conversion_2 = (long int)duty2;
                    conversion_3 = (long int)duty3;
                    conversion_4 = (long int)duty4;
                    conversion_5 = (int)adcValue0;

                    primero = conversion/1000; //1
                    dif = conversion - primero*1000; //245
                    segundo = dif/100; //2
                    dif = dif - segundo*100; //45
                    tercero = dif/10; // 0
                    dif = dif - tercero*10;
                    cuarto = dif/1; // 0

                    primero_2 = conversion_2/1000; //1
                    dif_2 = conversion_2 - primero_2*1000; //245
                    segundo_2 = dif_2/100; //2
                    dif_2 = dif_2 - segundo_2*100; //45
                    tercero_2 = dif_2/10; // 0
                    dif_2 = dif_2 - tercero_2*10;
                    cuarto_2 = dif_2/1; // 0

                    primero_3 = conversion_3/1000; //1
                    dif_3 = conversion_3 - primero_3*1000; //245
                    segundo_3 = dif_3/100; //2
                    dif_3 = dif_3 - segundo_3*100; //45
                    tercero_3 = dif_3/10; // 0
                    dif_3 = dif_3 - tercero_3*10;
                    cuarto_3 = dif_3/1; // 0

                    primero_4 = conversion_4/1000; //1
                    dif_4 = conversion_4 - primero_4*1000; //245
                    segundo_4 = dif_4/100; //2
                    dif_4 = dif_4 - segundo_4*100; //45
                    tercero_4 = dif_4/10; // 0
                    dif_4 = dif_4 - tercero_4*10;
                    cuarto_4 = dif_4/1; // 0

                    primero_5 = conversion_5/10000; //1
                    dif_5 = conversion_5 - primero_5*10000; //245
                    segundo_5 = dif_5/1000; //2
                    dif_5 = dif_5 - segundo_5*1000; //45
                    tercero_5 = dif_5/100; // 0
                    dif_5 = dif_5 - tercero_5*100;
                    cuarto_5 = dif_5/10; // 0

                    primero += '0';  primero_2 += '0';  primero_3 += '0';  primero_4 += '0'; primero_5 += '0';
                    segundo += '0';  segundo_2 += '0';  segundo_3 += '0';  segundo_4 += '0'; segundo_5 += '0';
                    tercero += '0';  tercero_2 += '0';  tercero_3 += '0';  tercero_4 += '0'; tercero_5 += '0';
                    cuarto  += '0';  cuarto_2  += '0';  cuarto_3  += '0';  cuarto_4  += '0'; cuarto_5  += '0';

                    buf[0]  = primero;
                    buf[1]  = segundo;
                    buf[2]  = tercero;
                    buf[3]  = cuarto;
                    buf[4]  = primero_2;
                    buf[5]  = segundo_2;
                    buf[6]  = tercero_2;
                    buf[7]  = cuarto_2 ;
                    buf[8]  = primero_3;
                    buf[9]  = segundo_3;
                    buf[10] = tercero_3;
                    buf[11] = cuarto_3 ;
                    buf[12] = primero_4;
                    buf[13] = segundo_4;
                    buf[14] = tercero_4;
                    buf[15] = cuarto_4 ;
                    buf[16] = primero_5;    
                    buf[17] = segundo_5;    
                    buf[18] = tercero_5;
                    buf[19] = cuarto_5;

                    w_tx_payload(32, buf);      	// Cargamos en la TX FIFO el mensaje a enviar
													// con un payload de 32 Bytes, esto nos sierve para mejorar la robustez del sistema (tamanyo fijo)
                    
                    /* Activamos el modo TX haciendo un pulso de 15 micro segundos (minimo 10 micros segun el datsheet) en el pin CE 
                     * para que pueda transmitir despues de transmitir el mensaje el trasnceptor se pone en modo standby I, 
                     * a la espera de activarse en modo RX o TX
                     */													
                    msprf24_activate_tx();
                          
                    msprf24_activate_rx();      	// Volvemos a poner el modulo en modo RX.

                }   //if temps
            }   //else
        }   //while
    }   //if config OK
}
