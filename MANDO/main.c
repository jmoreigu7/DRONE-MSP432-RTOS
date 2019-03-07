/*
 *  ====================== PEPEs TEAM ======================
 *  Universidad de Barcelona 2018-2019
 *  Grado de Ingenieria - Electronica de Telecomunicaciones
 *  Laboratorio de Sistemas Electronico II
 *  Autores: Alvaro Baucells Costa, Arnau Vicente Puiggros,
 *  Imanol Fernandez Moreno y Jefferson Moreira Guaycha
 */
 
#include "msprf24.h"            //libreria de funciones del transceptor nRF24L01
#include "nrf_userconfig.h"     //include specific configuration for the nrf24L01 module
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>

/* Graphic library context */
Graphics_Context g_sContext;

/* ADC results buffer */
static uint16_t resultsBufferJoy[2];
static uint16_t resultsBufferAcc[2];
volatile unsigned int user;     //variable usada para debug (nRF24L01)


uint8_t buf[32];        //-- variable to store and send data
uint8_t dato[32];

uint8_t dut_1[4];
uint8_t dut_2[4];
uint8_t dut_3[4];
uint8_t dut_4[4];
uint8_t val_b[5];

int32_t longitud=4; //longitud del buffer a printar

int MENU;

void drawAccelData(void);
void drawJoyData(void);
void MenuIni(void);

/*
 * Funcio delay 1ms
 */
void delayMs(int n) {
    int i, j;

    for (j = 0; j < n; j++)
        for (i = 750; i > 0; i--);      /* Delay 1 ms */

}

/*
 * Main function
 */
int main(void)
{
    /* Halting WDT and disabling master interrupts */
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     //-- stop watchdog timer

    /* Set the core voltage level to VCORE1 */
    MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);

    /* Set 2 flash wait states for Flash bank 0 and 1*/
    MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
    MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);

    /* Configuring GPIO as an output , LED SLOW BATTERY*/
    P2->SEL1 &=~0x06;
    P2->SEL0 &=~0x06;
    P2->DIR |=0x06;
    P2->OUT &=~0x06;

    /* Initializes display */
    Crystalfontz128x128_Init();

    /* Set default screen orientation */
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    /* Initializes graphics context */
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128, &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);

    Graphics_clearDisplay(&g_sContext);

    MenuIni();


    /* Configures Pin 6.1, 4.0 and 4.2 as ADC input for Accelerometer*/
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN2, GPIO_TERTIARY_MODULE_FUNCTION);
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN1, GPIO_TERTIARY_MODULE_FUNCTION);

    /* Configures Pin 6.0 and 4.4 as ADC input for joystick*/
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN0, GPIO_TERTIARY_MODULE_FUNCTION);
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN4, GPIO_TERTIARY_MODULE_FUNCTION);

    /* Initializing ADC (ADCOSC/64/8) */
    MAP_ADC14_enableModule();
    MAP_ADC14_initModule(ADC_CLOCKSOURCE_ADCOSC, ADC_PREDIVIDER_64, ADC_DIVIDER_8, 0);

    /* Configuring ADC Memory (ADC_MEM0 - ADC_MEM2 (A11, A13, A14)  with no repeat) for accelerometer
         * with internal 2.5v reference */
    MAP_ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM3, true);
    MAP_ADC14_configureConversionMemory(ADC_MEM0, ADC_VREFPOS_AVCC_VREFNEG_VSS,
                                        ADC_INPUT_A14, ADC_NONDIFFERENTIAL_INPUTS);

    MAP_ADC14_configureConversionMemory(ADC_MEM1, ADC_VREFPOS_AVCC_VREFNEG_VSS,
                                        ADC_INPUT_A13, ADC_NONDIFFERENTIAL_INPUTS);

    /* Configuring ADC Memory (ADC_MEM0 - ADC_MEM1 (A15, A9)  with repeat) for joystick
         * with internal 2.5v reference */

    MAP_ADC14_configureConversionMemory(ADC_MEM2, ADC_VREFPOS_AVCC_VREFNEG_VSS,
                                        ADC_INPUT_A15, ADC_NONDIFFERENTIAL_INPUTS);

    MAP_ADC14_configureConversionMemory(ADC_MEM3, ADC_VREFPOS_AVCC_VREFNEG_VSS,
                                        ADC_INPUT_A9, ADC_NONDIFFERENTIAL_INPUTS);

    /* Enabling the interrupt when a conversion on channel 1 (end of sequence)
     *  is complete and enabling conversions */
    MAP_ADC14_enableInterrupt(ADC_INT1);//joy
    MAP_ADC14_enableInterrupt(ADC_INT2);//acc

    /* Enabling Interrupts */
    MAP_Interrupt_enableInterrupt(INT_ADC14);
    MAP_Interrupt_enableMaster();

    /* Setting up the sample timer to automatically step through the sequence
     * convert.
     */
    MAP_ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

    /* Triggering the start of the sample */
    MAP_ADC14_enableConversion();
    MAP_ADC14_toggleConversionTrigger();

//RF
//------------------------------------------------------------------------------------------
    //nrf24L01 variables (direccion y buffer para almacenar datos)
    uint8_t addr[5];        //-- address of the pipe

    user = 0xFE;    //variable for debug

    //LEDS for report the status of the transmision

    P2->SEL1 &=~0x01; //configuracion del led rojo
    P2->SEL0 &=~0x01;
    P2->DIR |=0x01;
    P2->OUT |= 0x01; //encendemos el led rojo
    P2->OUT &=~0x01; //apagamos el led rojo

    P2->SEL1 &=~0x02;  //configuramos led verde
    P2->SEL0 &=~0x02;
    P2->DIR |=0x02;
    P2->OUT |= 0x02; //dejamos encendido el led verde

    //Initial values for nRF24L01+ library config variables
    rf_crc = RF24_EN_CRC | RF24_CRCO; //-- CRC enabled, 16-bit
    rf_addr_width      = 5;         //definimos la longitud de la direccion del transceptor. Estos parametros tienen que coincidir tambien en el receptor.
    rf_speed_power     = RF24_SPEED_2MBPS | RF24_POWER_0DBM;  //configuramos el transceptor a 2MBbps y potencia maxima para garantizar fiabilidad de transmision
    rf_channel         = 120;       //

    msprf24_init();  //All RX pipes closed by default
    msprf24_set_pipe_packetsize(0, 32);  //se elige la longitud del paquete de datos de la "pipe" correspondiente
    msprf24_open_pipe(0, 1);  //-- Open pipe#0 with Enhanced ShockBurst enabled for receiving Auto-ACKs
    //-- Note: Pipe#0 is hardcoded in the transceiver hardware as the designated "pipe" for a TX node to receive
    //-- auto-ACKs.  This does not have to match the pipe# used on the RX side.

    msprf24_standby();      //en este modo, el transceptor consume menos y esta a la espera de ser activado en modo RX o TX.
    user = msprf24_current_state(); //esta funcion nos retorna el estado del transceptor. Si esta en modo power down, standby, en modo RX o TX..

    addr[0] = 0xDC; addr[1] = 0xBA; addr[2] = 0xAB; addr[3] = 0xCD; addr[4] = 0x00; //direccion del transceptor, tiene que coincidir con la del receptor

    w_tx_addr(addr);     //escribe la direccion del transmisor
    w_rx_addr(0, addr);  //-- Pipe 0 receives auto-ack's, autoacks are sent back to the TX addr so the PTX node
                         //-- needs to listen to the TX addr on pipe#0 to receive them.
    //msprf24_activate_tx();

   //Aquesta lina ens marca la separacio entre les dades rebudes i la informacio del menu


    dut_1[0]='0';dut_1[1]='0';dut_1[2]='0';dut_1[3]='0';
    dut_2[0]='0';dut_2[1]='0';dut_2[2]='0';dut_2[3]='0';
    dut_3[0]='0';dut_3[1]='0';dut_3[2]='0';dut_3[3]='0';
    dut_4[0]='0';dut_4[1]='0';dut_4[2]='0';dut_4[3]='0';
    while(1)
    {
        //MAP_PCM_gotoLPM0();
        msprf24_activate_rx();
        flush_tx();
        memset(buf, 0, sizeof(buf));    //borra el contenido de la variable buf
        delayMs(5);


        if (rf_irq & RF24_IRQ_RX || msprf24_rx_pending()){

            r_rx_payload(32, dato);
            //Aqui ja podem veure el valor del buffer dato
            msprf24_irq_clear(RF24_IRQ_RX);

            P2->OUT |= 0x01; //encendemos el led rojo
            delayMs(500);
            P2->OUT &=~0x01; //apagamos el led rojo

            //duty1
            dut_1[0]=dato[0];
            dut_1[1]=dato[1];
            dut_1[2]=dato[2];
            dut_1[3]=dato[3];

            //duty2
            dut_2[0]=dato[4];
            dut_2[1]=dato[5];
            dut_2[2]=dato[6];
            dut_2[3]=dato[7];

            //duty3
            dut_3[0]=dato[8];
            dut_3[1]=dato[9];
            dut_3[2]=dato[10];
            dut_3[3]=dato[11];

            //duty4
            dut_4[0]=dato[12];
            dut_4[1]=dato[13];
            dut_4[2]=dato[14];
            dut_4[3]=dato[15];

            //bateria
            val_b[0]=dato[16];
            val_b[1]=dato[17];
            val_b[2]=dato[18];
            val_b[3]=dato[19];
            val_b[4]=dato[20];


    }
        /*Si el dato recibido empieza por todo 1, lo descartamos (ruido)*/
        if (!(dut_1[0]=='1' && dut_1[1]=='1' && dut_1[2]=='1' && dut_1[3]=='1')){

                    Graphics_drawStringCentered(&g_sContext,
                                                (int8_t *) dut_1,
                                                longitud,
                                                    30,
                                                    80,
                                                    OPAQUE_TEXT);

                    Graphics_drawStringCentered(&g_sContext,
                                                (int8_t *) dut_2,
                                                longitud,
                                                    80,
                                                    80,
                                                    OPAQUE_TEXT);

                    Graphics_drawStringCentered(&g_sContext,
                                                (int8_t *) dut_3,
                                                longitud,
                                                    30,
                                                    100,
                                                    OPAQUE_TEXT);

                    Graphics_drawStringCentered(&g_sContext,
                                                (int8_t *) dut_4,
                                                longitud,
                                                    80,
                                                    100,
                                                    OPAQUE_TEXT);

                }

        /*Si el valor de la bateria es mas bajo que 12000 enciende el LED azul*/
        if (val_b[0]<'1' || val_b[1]<='2'){
            P2->OUT |= 0x06; //ponemos el led rojo a azul
            Graphics_drawStringCentered(&g_sContext,            //Printa por pantalla el aviso
                                            "WNG BATT",
                                            AUTO_STRING_LENGTH,
                                            95,
                                            120,
                                            OPAQUE_TEXT);
        }
        /*Si el valor de la bateria es mas alto que 12000 apaga el LED azul*/
        if (val_b[0]=='1' && val_b[1]>'2'){

            P2->OUT &=~0x06;
            P2->OUT |= 0x02;
            Graphics_drawStringCentered(&g_sContext,            //Printa por pantalla el aviso
                                                        "                ",
                                                        AUTO_STRING_LENGTH,
                                                        95,
                                                        120,
                                                        OPAQUE_TEXT);
        }

        MAP_Interrupt_enableInterrupt(INT_ADC14);
    }
}

/*
 * MenuIni
 * Esta funcion printa el menu inicial
 */
void MenuIni(){
    Graphics_clearDisplay(&g_sContext);             //Clear del display
    Graphics_drawStringCentered(&g_sContext,
                                    (int8_t *)"MENU DRON PEPES:",
                                    AUTO_STRING_LENGTH,
                                    64,
                                    10,
                                    OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext,
                                    "ESCOJA UNA OPCION:",
                                    AUTO_STRING_LENGTH,
                                    64,
                                    25,
                                    OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext,
                                    "S1 -> MANUAL",
                                    AUTO_STRING_LENGTH,
                                    64,
                                    40,
                                    OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext,
                                    " S2 -> ACCEL ",
                                    AUTO_STRING_LENGTH,
                                    64,
                                    55,
                                    OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext,
                                "---------------------------------------",
                                AUTO_STRING_LENGTH,
                                    64,
                                    65,
                                    OPAQUE_TEXT);

}

/*
 * ---- drawJoyData ----
 *
 * Esta funcion ejecuta las instrucciones para cada valor del joystick
 */
void drawJoyData(){

    unsigned char k=0;

    Graphics_drawStringCentered(&g_sContext,
                                    " MENU JOYSTICK:  ",
                                    AUTO_STRING_LENGTH,
                                    64,
                                    20,
                                    OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext,
                                "---------------------------------------",
                                AUTO_STRING_LENGTH,
                                    64,
                                    65,
                                    OPAQUE_TEXT);
//    if (val_b[0]<1 || val_b[1]<2){
//        Graphics_drawStringCentered(&g_sContext,
//                                        "WNG BATT",
//                                        AUTO_STRING_LENGTH,
//                                        95,
//                                        120,
//                                        OPAQUE_TEXT);
//    }

    if(resultsBufferJoy[0]>14000){          //--- D E R E C H A -----

        Graphics_drawStringCentered(&g_sContext,
                                    "  RIGHT  ",
                                    AUTO_STRING_LENGTH,
                                        64,
                                        50,
                                        OPAQUE_TEXT);
        flush_tx();
        memset(buf, 0, sizeof(buf));    //borra el contenido de la variable buf
        delayMs(5);

          buf[0]='R';
          buf[1]='I';
          for(k=10;k>0;k--)
          {
              w_tx_payload(32, buf);       //cargamos en la TX FIFO el mensaje a enviar
              msprf24_activate_tx();       //Activamos el modo tx haciendo un pulso de 15 micro segundos (minimo 10 micros segun el datsheet) en el pin CE para que pueda transmitir
              msprf24_standby();
              delayMs(5);                //despues de transmitir el mensaje el trasnceptor se pone automaticamente en modo standby I, a la espera de activarse en modo RX o TX

          }

    }
    if (resultsBufferJoy[0]<2100){      //--- I Z Q U I E R D A ---

        Graphics_drawStringCentered(&g_sContext,
                                    "  LEFT  ",
                                    AUTO_STRING_LENGTH,
                                        64,
                                        50,
                                        OPAQUE_TEXT);
        flush_tx();
        memset(buf, 0, sizeof(buf));    //borra el contenido de la variable buf
        delayMs(5);

          buf[0]='L';
          buf[1]='E';
          for(k=10;k>0;k--)
          {
              w_tx_payload(32, buf);       //cargamos en la TX FIFO el mensaje a enviar
              msprf24_activate_tx();       //Activamos el modo tx haciendo un pulso de 15 micro segundos (minimo 10 micros segun el datsheet) en el pin CE para que pueda transmitir
              msprf24_standby();
              delayMs(5);                //despues de transmitir el mensaje el trasnceptor se pone automaticamente en modo standby I, a la espera de activarse en modo RX o TX

          }
}
    if(resultsBufferJoy[1]>12000){      //--- A D E L A N T E ----

            Graphics_drawStringCentered(&g_sContext,
                                        "    FORWARD    ",
                                        AUTO_STRING_LENGTH,
                                            64,
                                            50,
                                            OPAQUE_TEXT);
            flush_tx();
            memset(buf, 0, sizeof(buf));    //borra el contenido de la variable buf
            delayMs(5);

              buf[0]='F';
              buf[1]='O';


              for(k=10;k>0;k--)
              {
                  w_tx_payload(32, buf);       //cargamos en la TX FIFO el mensaje a enviar
                  msprf24_activate_tx();       //Activamos el modo tx haciendo un pulso de 15 micro segundos (minimo 10 micros segun el datsheet) en el pin CE para que pueda transmitir
                  msprf24_standby();
                  delayMs(5);                //despues de transmitir el mensaje el trasnceptor se pone automaticamente en modo standby I, a la espera de activarse en modo RX o TX

              }
        }
    if(resultsBufferJoy[1]<2100){        //--- A T R A S ----

        Graphics_drawStringCentered(&g_sContext,
                                    "   BACKWARD   ",
                                    AUTO_STRING_LENGTH,
                                        64,
                                        50,
                                        OPAQUE_TEXT);
        flush_tx();
        memset(buf, 0, sizeof(buf));    //borra el contenido de la variable buf
        delayMs(5);

          buf[0]='B';
          buf[1]='A';

          for(k=10;k>0;k--)
          {
              w_tx_payload(32, buf);       //cargamos en la TX FIFO el mensaje a enviar
              msprf24_activate_tx();       //Activamos el modo tx haciendo un pulso de 15 micro segundos (minimo 10 micros segun el datsheet) en el pin CE para que pueda transmitir
              msprf24_standby();
              delayMs(5);                //despues de transmitir el mensaje el trasnceptor se pone automaticamente en modo standby I, a la espera de activarse en modo RX o TX

          }
    }
}

/*
 * ---- drawAccelData ----
 *
 * Esta funcion ejecuta las instrucciones para cada valor del Acelerometro
 */
void drawAccelData(){

    unsigned char i=0;  //variable para el for

    Graphics_drawStringCentered(&g_sContext,
                                    "MENU ACCELEROMETER:",          //control mediante el acelerometro
                                    AUTO_STRING_LENGTH,
                                    64,
                                    20,
                                    OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext,
                                "---------------------------------------",
                                AUTO_STRING_LENGTH,
                                    64,
                                    65,
                                    OPAQUE_TEXT);



    if (resultsBufferAcc[0]<7000) {      //--- E S T A B I L I Z A N D O ----
        Graphics_drawStringCentered(&g_sContext,
                                    "                   ",
                                    AUTO_STRING_LENGTH,
                                    64,
                                    40,
                                    OPAQUE_TEXT);
        Graphics_drawStringCentered(&g_sContext,
                                    "      STABLE      ",        //inclinamos con un roll negativo
                                    AUTO_STRING_LENGTH,
                                        64,
                                        50,
                                        OPAQUE_TEXT);

        flush_tx();
        memset(buf, 0, sizeof(buf));    //borra el contenido de la variable buf
        delayMs(5);

          buf[0]='O';
          buf[1]='K';
          for(i=10;i>0;i--)
          {
              w_tx_payload(32, buf);       //cargamos en la TX FIFO el mensaje a enviar
              msprf24_activate_tx();       //Activamos el modo tx haciendo un pulso de 15 micro segundos (minimo 10 micros segun el datsheet) en el pin CE para que pueda transmitir
              msprf24_standby();
              delayMs(5);                //despues de transmitir el mensaje el trasnceptor se pone automaticamente en modo standby I, a la espera de activarse en modo RX o TX

          }

    }

    if (resultsBufferAcc[0]>10000){          //--- P A R A M O S   M O T O R E S  ----

        Graphics_drawStringCentered(&g_sContext,
                                    "!!! STOP !!!",
                                    AUTO_STRING_LENGTH,
                                    64,
                                    40,
                                    OPAQUE_TEXT);
        Graphics_drawStringCentered(&g_sContext,
                                    "!!! MOTORS !!!",
                                    AUTO_STRING_LENGTH,
                                    64,
                                    50,
                                    OPAQUE_TEXT);


        flush_tx();
        memset(buf, 0, sizeof(buf));    //borra el contenido de la variable buf
        delayMs(5);
        //Graphics_clearDisplay(&g_sContext);
          buf[0]='S';
          buf[1]='T';
          for(i=10;i>0;i--)
          {
              w_tx_payload(32, buf);       //cargamos en la TX FIFO el mensaje a enviar
              msprf24_activate_tx();       //Activamos el modo tx haciendo un pulso de 15 micro segundos (minimo 10 micros segun el datsheet) en el pin CE para que pueda transmitir
              msprf24_standby();
              delayMs(5);                //despues de transmitir el mensaje el trasnceptor se pone automaticamente en modo standby I, a la espera de activarse en modo RX o TX

          }

    }

    if (resultsBufferAcc[1]<6500){               //---  D E S P E G A M O S  ----
        Graphics_drawStringCentered(&g_sContext,
                                    "                   ",
                                    AUTO_STRING_LENGTH,
                                    64,
                                    40,
                                    OPAQUE_TEXT);
        Graphics_drawStringCentered(&g_sContext,
                                    "     TAKING OFF     ",
                                    AUTO_STRING_LENGTH,
                                        64,
                                        50,
                                        OPAQUE_TEXT);
        flush_tx();
        memset(buf, 0, sizeof(buf));    //borra el contenido de la variable buf
        delayMs(5);

        buf[0]='U';
        buf[1]='P';
          for(i=10;i>0;i--)
          {
              w_tx_payload(32, buf);       //cargamos en la TX FIFO el mensaje a enviar
              msprf24_activate_tx();       //Activamos el modo tx haciendo un pulso de 15 micro segundos (minimo 10 micros segun el datsheet) en el pin CE para que pueda transmitir
              msprf24_standby();
              delayMs(5);                //despues de transmitir el mensaje el trasnceptor se pone automaticamente en modo standby I, a la espera de activarse en modo RX o TX

          }
    }

        if (resultsBufferAcc[1]>9000){       //--- B A J A M O S    P O T E N C I A    M O T O R E S ----
            Graphics_drawStringCentered(&g_sContext,
                                        "                   ",
                                        AUTO_STRING_LENGTH,
                                        64,
                                        40,
                                        OPAQUE_TEXT);
        Graphics_drawStringCentered(&g_sContext,
                                    "     SLOW MOTORS     ",         //bajamos el throttle del drone, reduce altura
                                    AUTO_STRING_LENGTH,
                                        64,
                                        50,
                                        OPAQUE_TEXT);
        flush_tx();
        memset(buf, 0, sizeof(buf));    //borra el contenido de la variable buf
        delayMs(5);

          buf[0]='D';
          buf[1]='O';
          for(i=10;i>0;i--)
          {
              w_tx_payload(32, buf);       //cargamos en la TX FIFO el mensaje a enviar
              msprf24_activate_tx();       //Activamos el modo tx haciendo un pulso de 15 micro segundos (minimo 10 micros segun el datsheet) en el pin CE para que pueda transmitir
              msprf24_standby();
              delayMs(5);                //despues de transmitir el mensaje el trasnceptor se pone automaticamente en modo standby I, a la espera de activarse en modo RX o TX

          }
    }
}

/* This interrupt is fired whenever a conversion is completed and placed in
 * ADC_MEM1. This signals the end of conversion and the results array is
 * grabbed and placed in resultsBufferJoy */
void ADC14_IRQHandler(void)
{

    uint64_t status;
    status = MAP_ADC14_getEnabledInterruptStatus();
    MAP_ADC14_clearInterruptFlag(status);


    /* ADC_MEM1 conversion completed */
    if(status & ADC_INT1)
    {
        /* Store ADC14 conversion results */
        resultsBufferAcc[0] = ADC14_getResult(ADC_MEM0);
        resultsBufferAcc[1] = ADC14_getResult(ADC_MEM1);

        resultsBufferJoy[0] = ADC14_getResult(ADC_MEM2);
        resultsBufferJoy[1] = ADC14_getResult(ADC_MEM3);


        /* Determine if JoyStick button is pressed */

        if (!(P4IN & GPIO_PIN1)){
            Graphics_clearDisplay(&g_sContext);
            Graphics_drawStringCentered(&g_sContext,
                                        "SALIENDO DEL",         //vuelve a la interfaz del menu principal
                                            AUTO_STRING_LENGTH,     //para que el usuario elija modo joystick o acelerometro
                                            64,
                                            40,
                                            OPAQUE_TEXT);
            Graphics_drawStringCentered(&g_sContext,
                                            (int8_t *)"MODO MANUAL",
                                            AUTO_STRING_LENGTH,
                                            64,
                                            60,
                                            OPAQUE_TEXT);

            //__delay_cycles(12000);
            MENU=0;
            MenuIni();
        }

        if (!(P5IN & GPIO_PIN1)){
            Graphics_clearDisplay(&g_sContext);
            MENU=1;

        }

        if (!(P3IN & GPIO_PIN5)){
            Graphics_clearDisplay(&g_sContext);
            MENU=2;

        }

        if (MENU == 1){
            drawJoyData();
        }
        else if (MENU == 2){
            drawAccelData();
        }

    }
    //Proba
    MAP_Interrupt_disableInterrupt(INT_ADC14);

}
