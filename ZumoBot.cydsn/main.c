/**
* @mainpage ZumoBot Project
* @brief    You can make your own ZumoBot with various sensors.
* @details  <br><br>
    <p>
    <B>General</B><br>
    You will use Pololu Zumo Shields for your robot project with CY8CKIT-059(PSoC 5LP) from Cypress semiconductor.This 
    library has basic methods of various sensors and communications so that you can make what you want with them. <br> 
    <br><br>
    </p>
    
    <p>
    <B>Sensors</B><br>
    &nbsp;Included: <br>
        &nbsp;&nbsp;&nbsp;&nbsp;LSM303D: Accelerometer & Magnetometer<br>
        &nbsp;&nbsp;&nbsp;&nbsp;L3GD20H: Gyroscope<br>
        &nbsp;&nbsp;&nbsp;&nbsp;Reflectance sensor<br>
        &nbsp;&nbsp;&nbsp;&nbsp;Motors
    &nbsp;Wii nunchuck<br>
    &nbsp;TSOP-2236: IR Receiver<br>
    &nbsp;HC-SR04: Ultrasonic sensor<br>
    &nbsp;APDS-9301: Ambient light sensor<br>
    &nbsp;IR LED <br><br><br>
    </p>
    
    <p>
    <B>Communication</B><br>
    I2C, UART, Serial<br>
    </p>
*/

#include <project.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "Motor.h"
#include "Ultra.h"
#include "Nunchuk.h"
#include "Reflectance.h"
#include "Gyro.h"
#include "Accel_magnet.h"
#include "LSM303D.h"
#include "IR.h"
#include "Beep.h"
#include "mqtt_sender.h"
#include <time.h>
#include <sys/time.h>
#include "serial1.h"
#include <unistd.h>
#include <stdlib.h>
/**
 * @file    main.c
 * @brief   
 * @details  ** Enable global interrupt since Zumo library uses interrupts. **<br>&nbsp;&nbsp;&nbsp;CyGlobalIntEnable;<br>
*/
#if 0 //tehtävä 1 vko 2

void zmain(void)
{   
    uint8 button;
    while(true)
    {
        button = SW1_Read();
        if (button == 0)
        {
            printf("Button pressed\n");
            for (int i = 0; i < 17; i++)
            {
                if (i%2 == 0) //Piippaukset
                {
                    if (i>5 && i<12) //PitkäPiippaus
                    {
                        Beep(1500, 150);
                    }else //LyhytPiippaus
                    {
                        Beep(500, 150);
                    }
                }else  //välit
                {
                    vTaskDelay(500);
                }
            }
        }
    }  
 }   
#endif
#if 0
/* Example of how to use te Accelerometer!!!*/
void zmain(void)
{
    struct accData_ data;
    
    printf("Accelerometer test...\n");

    if(!LSM303D_Start()){
        printf("LSM303D failed to initialize!!! Program is Ending!!!\n");
        vTaskSuspend(NULL);
    }
    else {
        printf("Device Ok...\n");
    }
    
    while(true)
    {
        LSM303D_Read_Acc(&data);
        printf("%8d %8d %8d\n",data.accX, data.accY, data.accZ);
        vTaskDelay(50);
    }
 }   
#endif    

#if 0 //tehtävä 1 vko 4
void zmain(void)
{
    int button = 1;
    int counter = 0;
    int change = 1;
    
    struct sensors_ ref;
    struct sensors_ dig;

    reflectance_start();
    reflectance_set_threshold(9000, 9000, 11000, 11000, 9000, 9000); // set center sensor threshold to 11000 and others to 9000

    motor_start();              // enable motor controller
    motor_forward(0,0);         // set speed to zero to stop motors
    
    while (true)
    {
        button = SW1_Read();
        while (button == 0)
        {         
            motor_forward(100,10);
            reflectance_digital(&dig); // when blackness value is over threshold the sensors reads 1, otherwise 0
            // print out each period of reflectance sensors
            //printf("%5d %5d %5d %5d %5d %5d \r\n", dig.l3, dig.l2, dig.l1, dig.r1, dig.r2, dig.r3);
            printf("%d\n", counter);
            if (change == 1 && dig.l3 == 1 && dig.l2 == 1 && dig.l1 == 1 && dig.r1 == 1 && dig.r2 == 1 && dig.r3 == 1) //detects and counts sections
            {
                counter++;
                if(counter == 1) //run if first section
                {
                    motor_forward(0,0);
                    printf("%d\n", counter);
                    //IR_wait();
                    vTaskDelay(3000);
                } else if(counter == 4)
                {
                    motor_forward(0,0);
                    button = 1;
                    counter = 0;
                    change = 1;
                }
                printf("pipari\n");
                change = 0;
            }
            if(dig.l3 == 0 || dig.l2 == 0 || dig.l1 == 0 || dig.r1 == 0 || dig.r2 == 0 || dig.r3 == 0)
            {
                printf("kahvi\n");
                change = 1;
            }
        }
    }
}

#endif

#if 0 //tehtävä 2 vko 4
void zmain(void)
{
    int button = 1;
    int counter = 0;
    int change = 1;
    
    struct sensors_ ref;
    struct sensors_ dig;

    reflectance_start();
    reflectance_set_threshold(9000, 9000, 11000, 11000, 9000, 9000); // set center sensor threshold to 11000 and others to 9000

    motor_start();              // enable motor controller
    motor_forward(0,0);         // set speed to zero to stop motors
    
    while (true)
    {
        button = SW1_Read();
        while (button == 0) //runs when button is pressed
        {         
            motor_forward(100,10);
            reflectance_digital(&dig); 
            //printf("%d\n", counter);
            if (change == 1 && dig.l3 == 1 && dig.l2 == 1 && dig.l1 == 1 && dig.r1 == 1 && dig.r2 == 1 && dig.r3 == 1) //detects and counts sections
            {
                counter++;
                if(counter == 1) //run if first section and stops 
                {
                    motor_forward(0,0);
                    motor_turn(125,0,900); //turn right 90 degrees
                    //IR_wait();
                    vTaskDelay(3000);
                } else if (counter == 2)
                {
                    motor_turn(0,100,900); //turn left 90 degrees
                } else if (counter < 5 && counter > 2)
                {
                    motor_turn(125,0,900); //turn right 90 degrees
                } else if(counter == 5)
                {
                    motor_forward(0,0);
                    button = 1;
                    counter = 0;
                    change = 1;
                }
                change = 0;
            }
            if(dig.l3 == 0 || dig.l2 == 0 || dig.l1 == 0 || dig.r1 == 0 || dig.r2 == 0 || dig.r3 == 0)
            {
                change = 1;
            }
        }
    }
}
#endif

#if 1 //tehtävä 2 vko 4
void zmain(void)
{
    int button = 1;
    int counter = 0;
    int change = 1;
    
    struct sensors_ ref;
    struct sensors_ dig;

    reflectance_start();
    reflectance_set_threshold(9000, 9000, 11000, 11000, 9000, 9000); // set center sensor threshold to 11000 and others to 9000

    motor_start();              // enable motor controller
    motor_forward(0,0);         // set speed to zero to stop motors
    
    while (true)
    {
        button = SW1_Read();
        while (button == 0) //runs when button is pressed
        {         
            motor_forward(100,10);
            reflectance_digital(&dig); 
            //printf("%d\n", counter);
            if (change == 1 && dig.l3 == 1 && dig.l2 == 1 && dig.l1 == 1 && dig.r1 == 1 && dig.r2 == 1 && dig.r3 == 1) //detects and counts sections
            {
                counter++;
                if(counter == 1) //run if first section and stops 
                {
                    motor_forward(0,0);
                    //IR_wait();
                    vTaskDelay(3000);
                } else if(counter == 2)
                {
                    motor_forward(0,0);
                    button = 1;
                    counter = 0;
                    change = 1;
                }
                change = 0;
            }
            if(dig.l3 == 0 || dig.l2 == 0 || dig.l1 == 0 || dig.r1 == 0 || dig.r2 == 0 || dig.r3 == 0) //makes sure that lines are not counted twice #fix 
            {
                change = 1;
            }
            if(dig.l3 == 0 && (dig.l2 == 0 || dig.l1 == 0 || dig.r1 == 0 || dig.r2 == 0 || dig.r3 == 0) //makes sure that lines are not counted twice #fix 
            {
                //turn right
            }
        }
    }
}
#endif

#if 0 //tehtävä 3 vko 3 (Matka Mordoriin)
void zmain(void)
{
int button = 1;
int random = 0;
int treshold = 0;
struct accData_ data;
motor_start();              // enable motor controller
motor_forward(0,0);         // set speed to zero to stop motors
    if(!LSM303D_Start()){
        printf("LSM303D failed to initialize!!! Program is Ending!!!\n");
        vTaskSuspend(NULL);
    }
    else {
        printf("Device Ok...\n");
    }
    while (true)
    {
        button = SW1_Read();
        while (button == 0)
        {   
            if (treshold > 2000 || treshold < -2000)
            {
                Beep(100,50);
                motor_backward(50,1000);    // moving backward
                motor_turn(100,200,1000);     // turn left 90 degrees
            }           
            motor_forward((rand() % 200),(rand() % 1000));
            random = rand() % 2;
            LSM303D_Read_Acc(&data);
            printf("%8d %8d %8d\n",data.accX, data.accY, data.accZ);
            vTaskDelay(50);
            if (random == 1)
            {
            motor_turn((rand() % 200),0,(rand() % 1000));
            }
            else if (random == 0)
            {
            motor_turn(0,(rand() % 200),(rand() % 1000));
            }
            /*else if (random == 2)
            {
            motor_backward((rand() % 200),(rand() % 1000));    // moving backward
            }*/
            treshold = data.accX;
        }
    }
   while(true)
   {
      vTaskDelay(100);
   }
}   
#endif

#if 0 //testi
void zmain(void)
{
int button = 1;
motor_start();              // enable motor controller
motor_forward(0,0);         // set speed to zero to stop motors
Ultra_Start();
vTaskDelay(3000);
while(true)
{
    button = SW1_Read();
    if (button == 0)
    {
        motor_forward(100,3500); 
        motor_turn(125,0,900); //turn right 90 degrees
        motor_forward(100,3200);
        motor_turn(125,0,900); //turn right 90 degrees
        motor_forward(100,3200);
        motor_turn(125,0,900); //turn right 90 degrees
        
        motor_turn(150,75,2000); //fukin banaaaana käännös
        motor_forward(100,1100);
        motor_forward(0,0); 
    }
}
}   
#endif

#if 0 //tehtävä 2 vko 3
void zmain(void)
{
int run = 1;
int d = 0;
motor_start();              // enable motor controller
motor_forward(0,0);         // set speed to zero to stop motors
Ultra_Start();
vTaskDelay(3000);

while (run == 1)
{
    d = Ultra_GetDistance();
    if (d < 10)
    {
        Beep(100,50);
        motor_backward(50,1000);    // moving backward
        //motor_turn(100,200,1000);     // turn left 90 degrees
        motor_turn(0,100,900); //turn left 90 degrees
    }
    motor_forward(120,25); 
    //printf("distance = %d\r\n", d);   
}   
}   
#endif

#if 0 //tehtävä 2 vko 2
void zmain(void)
{
    int age = 0;
    float timeStart = 0;
    float timeStop = 0;
    float time = 0;
    
    timeStart = xTaskGetTickCount();
    
    printf ("How old are you?\n ");
    scanf ("%d",&age);
    
    timeStop = xTaskGetTickCount();
    time = (timeStop - timeStart)/1000;
    printf ("%f\n", time);
    if (time < 3)
    {
        if (age <= 21)
        {
            printf("Super fast dude!\n");
        }else if (age >= 22 && age <= 50)
        {
            printf("Be quick or be dead\n");
        }else if (age > 50)
        {
            printf("Still going strong\n");
        }
    }else if (time >= 3 && time <= 5)
    {
        if (age <= 21)
        {
            printf("So mediocre.\n");
        }else if (age >= 22 && age <= 50)
        {
            printf("You are so average.\n");
        }else if (age > 50)
        {
            printf("You are doing ok for your age.\n");
        }
    
    }else if (time > 5)
    {
        if (age <= 21)
        {
            printf("My granny is faster than you!\n");
        }else if (age >= 22 && age <= 50)
        {
            printf("Have you been smoking something illegal?\n");
        }else if (age > 50)
        {
            printf("Do they still allow you to drive?\n");
        }
    }
    while(true)
    {
    vTaskDelay(100);
    }
}
#endif

#if 0 //testi
void zmain(void)
{
    uint8 button;
    button = SW1_Read();
   while (button == 1)
    {
    BatteryLed_Write(1); // Switch led on 
    vTaskDelay(500);
    BatteryLed_Write(0); // Switch led off 
    vTaskDelay(500);
    } 
    BatteryLed_Write(0); // Switch led off 
}   
#endif

#if 0
// Hello World!
void zmain(void)
{
    printf("\nHello, World!\n");

    while(true)
    {
        vTaskDelay(100); // sleep (in an infinite loop)
    }
 }   
#endif

#if 0
// Name and age
void zmain(void)
{
    char name[32];
    int age;
    
    
    printf("\n\n");
    
    printf("Enter your name: ");
    //fflush(stdout);
    scanf("%s", name);
    printf("Enter your age: ");
    //fflush(stdout);
    scanf("%d", &age);
    
    printf("You are [%s], age = %d\n", name, age);

    while(true)
    {
        BatteryLed_Write(!SW1_Read());
        vTaskDelay(100);
    }
 }   
#endif


#if 0 //tehtävä 3 vko 2
//battery level//
void zmain(void)
{
    ADC_Battery_Start();        

    int16 adcresult =0;
    float volts = 0.0;
    float vmax = 5.0;
    float bitres = 4096.0;
    float divider = vmax/bitres;
    uint8 button = 1;

    printf("\nBoot\n");

    //BatteryLed_Write(1); // Switch led on 
    BatteryLed_Write(0); // Switch led off 
    //uint8 button;
    //button = SW1_Read(); // read SW1 on pSoC board
    // SW1_Read() returns zero when button is pressed
    // SW1_Read() returns one when button is not pressed

    while(true)
    {
        char msg[80];
        ADC_Battery_StartConvert(); // start sampling
        
        if(ADC_Battery_IsEndConversion(ADC_Battery_WAIT_FOR_RESULT)) {   // wait for ADC converted value
            adcresult = ADC_Battery_GetResult16(); // get the ADC value (0 - 4095)
            // convert value to Volts
            volts = adcresult*divider*1.5;
            button = SW1_Read();
            while(volts < 4 && button == 1)
            {
                button = SW1_Read();
                BatteryLed_Write(1); // Switch led on 
                vTaskDelay(50);
                BatteryLed_Write(0); // Switch led off 
                vTaskDelay(50);
                printf("%d %f\r\n",adcresult, volts);
            }
           
            // you need to implement the conversion
            
            // Print both ADC results and converted value
            printf("%d %f\r\n",adcresult, volts);
            vTaskDelay(1000);
        }
       
    }
    
    
 }   
#endif

#if 0
// button
void zmain(void)
{
    while(true) {
        printf("Press button within 5 seconds!\n");
        int i = 50;
        while(i > 0) {
            if(SW1_Read() == 0) {
                break;
            }
            vTaskDelay(100);
            --i;
        }
        if(i > 0) {
            printf("Good work\n");
            while(SW1_Read() == 0) vTaskDelay(10); // wait until button is released
        }
        else {
            printf("You didn't press the button\n");
        }
    }
}
#endif

#if 0
// button
void zmain(void)
{
    printf("\nBoot\n");

    //BatteryLed_Write(1); // Switch led on 
    BatteryLed_Write(0); // Switch led off 
    
    //uint8 button;
    //button = SW1_Read(); // read SW1 on pSoC board
    // SW1_Read() returns zero when button is pressed
    // SW1_Read() returns one when button is not pressed
    
    bool led = false;
    
    while(true)
    {
        // toggle led state when button is pressed
        if(SW1_Read() == 0) {
            led = !led;
            BatteryLed_Write(led);
            if(led) printf("Led is ON\n");
            else printf("Led is OFF\n");
            Beep(1000, 150);
            while(SW1_Read() == 0) vTaskDelay(10); // wait while button is being pressed
        }        
    }
 }   
#endif


#if 0
//ultrasonic sensor//
void zmain(void)
{
    Ultra_Start();                          // Ultra Sonic Start function
    
    while(true) {
        int d = Ultra_GetDistance();
        // Print the detected distance (centimeters)
        printf("distance = %d\r\n", d);
        vTaskDelay(200);
    }
}   
#endif

#if 0
//IR receiverm - how to wait for IR remote commands
void zmain(void)
{
    IR_Start();
    
    printf("\n\nIR test\n");
    
    IR_flush(); // clear IR receive buffer
    printf("Buffer cleared\n");
    
    bool led = false;
    // Toggle led when IR signal is received
    while(true)
    {
        IR_wait();  // wait for IR command
        led = !led;
        BatteryLed_Write(led);
        if(led) printf("Led is ON\n");
        else printf("Led is OFF\n");
    }    
 }   
#endif



#if 0
//IR receiver - read raw data
void zmain(void)
{
    IR_Start();
    
    uint32_t IR_val; 
    
    printf("\n\nIR test\n");
    
    IR_flush(); // clear IR receive buffer
    printf("Buffer cleared\n");
    
    // print received IR pulses and their lengths
    while(true)
    {
        if(IR_get(&IR_val, portMAX_DELAY)) {
            int l = IR_val & IR_SIGNAL_MASK; // get pulse length
            int b = 0;
            if((IR_val & IR_SIGNAL_HIGH) != 0) b = 1; // get pulse state (0/1)
            printf("%d %d\r\n",b, l);
        }
    }    
 }   
#endif


#if 0
//reflectance
void zmain(void)
{
    struct sensors_ ref;
    struct sensors_ dig;

    reflectance_start();
    reflectance_set_threshold(9000, 9000, 11000, 11000, 9000, 9000); // set center sensor threshold to 11000 and others to 9000
    

    while(true)
    {
        // read raw sensor values
        reflectance_read(&ref);
        // print out each period of reflectance sensors
        printf("%5d %5d %5d %5d %5d %5d\r\n", ref.l3, ref.l2, ref.l1, ref.r1, ref.r2, ref.r3);       
        
        // read digital values that are based on threshold. 0 = white, 1 = black
        // when blackness value is over threshold the sensors reads 1, otherwise 0
        reflectance_digital(&dig); 
        //print out 0 or 1 according to results of reflectance period
        printf("%5d %5d %5d %5d %5d %5d \r\n", dig.l3, dig.l2, dig.l1, dig.r1, dig.r2, dig.r3);        
        
        vTaskDelay(200);
    }
}   
#endif


#if 0
//motor
void zmain(void)
{
    motor_start();              // enable motor controller
    motor_forward(0,0);         // set speed to zero to stop motors

    vTaskDelay(3000);
    
    motor_forward(100,2000);     // moving forward
    motor_turn(200,50,2000);     // turn
    motor_turn(50,200,2000);     // turn
    motor_backward(100,2000);    // moving backward
     
    motor_forward(0,0);         // stop motors

    motor_stop();               // disable motor controller
    
    while(true)
    {
        vTaskDelay(100);
    }
}
#endif

#if 0
/* Example of how to use te Accelerometer!!!*/
void zmain(void)
{
    struct accData_ data;
    motor_start();              // enable motor controller
    motor_forward(0,0);         // set speed to zero to stop motors
    
    printf("Accelerometer test...\n");

    if(!LSM303D_Start()){
        printf("LSM303D failed to initialize!!! Program is Ending!!!\n");
        vTaskSuspend(NULL);
    }
    else {
        printf("Device Ok...\n");
    }
    
    while(true)
    {
        //motor_forward(250,50);
        //motor_backward(250,50);
        LSM303D_Read_Acc(&data);
        printf("%8d %8d %8d\n",data.accX, data.accY, data.accZ);
        vTaskDelay(50);
    }
 }   
#endif    

#if 0
// MQTT test
void zmain(void)
{
    int ctr = 0;

    printf("\nBoot\n");
    send_mqtt("Zumo01/debug", "Boot");

    //BatteryLed_Write(1); // Switch led on 
    BatteryLed_Write(0); // Switch led off 

    while(true)
    {
        printf("Ctr: %d, Button: %d\n", ctr, SW1_Read());
        print_mqtt("Zumo01/debug", "Ctr: %d, Button: %d", ctr, SW1_Read());

        vTaskDelay(1000);
        ctr++;
    }
 }   
#endif


#if 0
void zmain(void)
{    
    struct accData_ data;
    struct sensors_ ref;
    struct sensors_ dig;
    
    printf("MQTT and sensor test...\n");

    if(!LSM303D_Start()){
        printf("LSM303D failed to initialize!!! Program is Ending!!!\n");
        vTaskSuspend(NULL);
    }
    else {
        printf("Accelerometer Ok...\n");
    }
    
    int ctr = 0;
    reflectance_start();
    while(true)
    {
        LSM303D_Read_Acc(&data);
        // send data when we detect a hit and at 10 second intervals
        if(data.accX > 1500 || ++ctr > 1000) {
            printf("Acc: %8d %8d %8d\n",data.accX, data.accY, data.accZ);
            print_mqtt("Zumo01/acc", "%d,%d,%d", data.accX, data.accY, data.accZ);
            reflectance_read(&ref);
            printf("Ref: %8d %8d %8d %8d %8d %8d\n", ref.l3, ref.l2, ref.l1, ref.r1, ref.r2, ref.r3);       
            print_mqtt("Zumo01/ref", "%d,%d,%d,%d,%d,%d", ref.l3, ref.l2, ref.l1, ref.r1, ref.r2, ref.r3);
            reflectance_digital(&dig);
            printf("Dig: %8d %8d %8d %8d %8d %8d\n", dig.l3, dig.l2, dig.l1, dig.r1, dig.r2, dig.r3);
            print_mqtt("Zumo01/dig", "%d,%d,%d,%d,%d,%d", dig.l3, dig.l2, dig.l1, dig.r1, dig.r2, dig.r3);
            ctr = 0;
        }
        vTaskDelay(10);
    }
 }   

#endif

#if 0
void zmain(void)
{    
    RTC_Start(); // start real time clock
    
    RTC_TIME_DATE now;

    // set current time
    now.Hour = 12;
    now.Min = 34;
    now.Sec = 56;
    now.DayOfMonth = 25;
    now.Month = 9;
    now.Year = 2018;
    RTC_WriteTime(&now); // write the time to real time clock

    while(true)
    {
        if(SW1_Read() == 0) {
            // read the current time
            RTC_DisableInt(); /* Disable Interrupt of RTC Component */
            now = *RTC_ReadTime(); /* copy the current time to a local variable */
            RTC_EnableInt(); /* Enable Interrupt of RTC Component */

            // print the current time
            printf("%2d:%02d.%02d\n", now.Hour, now.Min, now.Sec);
            
            // wait until button is released
            while(SW1_Read() == 0) vTaskDelay(50);
        }
        vTaskDelay(50);
    }
 }   
#endif

/* [] END OF FILE */
