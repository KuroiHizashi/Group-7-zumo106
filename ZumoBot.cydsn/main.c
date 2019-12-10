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
#include <math.h>
/**
 * @file    main.c
 * @brief   
 * @details  ** Enable global interrupt since Zumo library uses interrupts. **<br>&nbsp;&nbsp;&nbsp;CyGlobalIntEnable;<br>
*/
void tank_turn_left(uint8, uint32);
void tank_turn_right(uint8, uint32);
double getDistance(void);

//Pitää ottaa distance sensoreista keskiarvo koska tulee välillä luvut hyppelee, jonka takia while loop breikkaa. 
//Esim otetaan 10 arvoa ja niistä keskiarvo jota käytetään while loopin ehto vertauksessa.

#if 0 //Sumo Competion
void zmain(void)
{
    //initialize variables
    int speed = 50; //globab variable for conrtolling speed
    int change = 1; //line detection is 1 when 1 of the sensors turns white after all black
    int counter = 0;
    int distance;
    int attack = 0;
    int loopCounter = 0;
    int angle = 0;
    int turn = 0; //integer for turning after a fight
    int button = 1;
    TickType_t start, end;
    struct sensors_ ref;
    struct sensors_ dig;
    struct accData_ data;

    reflectance_start();
    reflectance_set_threshold(9000, 9000, 11000, 11000, 9000, 9000); // set center sensor threshold to 11000 and others to 9000
    
    //start sensors
    motor_start();
    motor_forward(0,0);
    start = xTaskGetTickCount();
    IR_Start();
    Ultra_Start();
    button = SW1_Read();
    if(!LSM303D_Start()){
        printf("LSM303D failed to initialize!!! Program is Ending!!!\n");
        vTaskSuspend(NULL);
    }
    else {
        printf("Device Ok...\n");
    }
    vTaskDelay(10000);
    while(true) //staging and waiting IR
    {
        motor_forward(speed,1);
        reflectance_digital(&dig);
        
        if(change == 1 && dig.l2 == 1 && dig.l1 == 1 && dig.r1 == 1 && dig.r2 == 1)
        {
            motor_forward(0,0);
            //IR_wait();
            vTaskDelay(3000);
            print_mqtt("ZUMO7/start", "%d", xTaskGetTickCount());// Send message to topic
            speed = 100;
            motor_forward(speed, 1500); // drives to center of the ring #placeholder time
            break;
        }
    }
    
    while(true) //primary program here 
    {
        reflectance_digital(&dig);
        tank_turn_left(speed,1);
        speed = 100;    //100
        
        while(getDistance() < 30) //when enemy is infront push  && dig.l1 == 0 && dig.r1 == 0
        {
            speed = 250; //250
            attack = 1;
            loopCounter = 500;//time in milliseconds for getting back after a fight 500
            if (getDistance() < 10)
            {
                while (true)
                {   
                    motor_forward(speed,1);
                    reflectance_digital(&dig);
                    if(dig.l1 == 1 || dig.r1 == 1)
                    {
                        break;
                    }
                }
                break;
            }
            motor_forward(speed,1);
            reflectance_digital(&dig);
            if(dig.l1 == 1 || dig.r1 == 1)
            {
                break;
            }
        }
        while (attack == 1) { //back to center(future)
            // toimii oikein
        
            if (turn == 0) //does 180 turn on first time on loop
            {
                speed=200;
                tank_turn_left(speed,380);//#placeholder time
                motor_forward(speed,100);
                turn ++;
            }
            // toimii oikein
            if (dig.l3 == 1 || dig.l2 == 1 || dig.l1 == 1 || dig.r1 == 1 || dig.r2 == 1 || dig.r3 == 1) //if while driving forward robot ends on black line do 180 turn
            {
                tank_turn_left(speed,380);//#placeholder time
                motor_forward(speed,100);
            }
            
            motor_forward(speed,1); //#placeholder time
            reflectance_digital(&dig);
            
            if (getDistance() < 20 || loopCounter == 0) //breaks recentering if enemy is detected or "time" runs out
            {
                attack = 0;
                loopCounter = 0;
                speed = 100;
                turn --;
                break;
            }
            loopCounter --;    
        }
        
        LSM303D_Read_Acc(&data);
        
        if (data.accX < -5000 && data.accY > 5000) //Look robot in the eyes, impact from the left front
        {   
            angle = (atan2(abs(data.accX),data.accY)*180/M_PI);
            printf("%8d %8d  Impact from the left front, impact was from %8d degrees\n",data.accX, data.accY,angle );
            print_mqtt("ZUMO7/hit", "%d %d Impact from the left front",xTaskGetTickCount(), angle);// Send message to topic
            //speed = 255;//WE NEED A BRAKE POINT SO THE ROBOT DOES NOT GO OVER THE BLACK LINE AT THE BACKWARD
            //motor_backward(speed,200); 
        } else if(data.accX > 5000 && data.accY > 5000) //Look robot in the eyes, impact from the left back
        {   
            angle = 90 + ((atan2(data.accX,data.accY))*180/M_PI);
            printf("%8d %8d  Impact from the left back, impact was from %8d degrees\n",data.accX, data.accY, angle);
            print_mqtt("ZUMO7/hit", "%d %d Impact from the left back", xTaskGetTickCount(), angle);// Send message to topic
            //speed = 255;//WE NEED A BRAKE POINT SO THE ROBOT DOES NOT GO OVER THE BLACK LINE AT THE BACKWARD
            //motor_forward(speed,200);
        } else if(data.accX < -5000 && data.accY < -5000) //Look robot in the eyes, impact from the right front
        {
            angle = 270 + ((atan2(abs(data.accX),abs(data.accY)))*180/M_PI);
            printf("%8d %8d  Impact from the right front, impact was from %8d degrees\n",data.accX, data.accY,angle );
            print_mqtt("ZUMO7/hit", "%d %d Impact from the right front", xTaskGetTickCount(), angle);// Send message to topic
            //speed = 255;//WE NEED A BRAKE POINT SO THE ROBOT DOES NOT GO OVER THE BLACK LINE AT THE BACKWARD
            //motor_backward(speed,200);
        } else if(data.accX > 5000 && data.accY < -5000) //Look robot in the eyes, impact from the right back
        {
            angle = 180 + ((atan2(abs(data.accX),abs(data.accY)))*180/M_PI);
            printf("%8d %8d  Impact from the right back, impact was from %8d degree\n",data.accX, data.accY, angle);
            print_mqtt("ZUMO7/hit", "%d %d Impact from the right back",xTaskGetTickCount(), angle);// Send message to topic
            //speed = 255;//WE NEED A BRAKE POINT SO THE ROBOT DOES NOT GO OVER THE BLACK LINE AT THE BACKWARD
            //motor_forward(speed,200);
        } 
        button = SW1_Read();
        if (button == 0)
        {
            end  = xTaskGetTickCount();
            print_mqtt("ZUMO7/stop", "%d", (end-start));// Send message to topic
            motor_forward(0,0);
            IR_wait();
        }
    }
}   
#endif
#if 0 //sumo test
    void zmain(void)
    {
    struct sensors_ ref;
    struct sensors_ dig; 
    reflectance_start();
    int change = 1;
    motor_start();
    int speed = 200;
    struct accData_ data;
    motor_forward(0,0);
    int angle = 0;
    reflectance_set_threshold(9000, 9000, 11000, 11000, 9000, 9000); // set center sensor threshold to 11000 and others to 9000
    
    if(!LSM303D_Start()){
        printf("LSM303D failed to initialize!!! Program is Ending!!!\n");
        vTaskSuspend(NULL);
    }
    else {
        printf("Device Ok...\n");
    }
    
    while(true) //staging and waiting IR
    {
        motor_forward(50,1);
        reflectance_digital(&dig);
        
        if(change == 1 && dig.l2 == 1 && dig.l1 == 1 && dig.r1 == 1 && dig.r2 == 1)
        {
            motor_forward(0,0);
            //IR_wait();
            vTaskDelay(3000);
           // motor_forward(50, 1500); // drives to center of the ring #placeholder time
            
        }
        break;
    }
    while(true){
        motor_forward(0,0);
     LSM303D_Read_Acc(&data);
   if (data.accX < -8000 && data.accY > 8000) //Look robot in the eyes, impact from the left front
        {   
            angle = (atan2(abs(data.accX),data.accY)*180/M_PI);
            printf("%8d %8d  Impact from the left front, impact was from %8d degrees\n",data.accX, data.accY,angle );
            speed = 255;//WE NEED A BRAKE POINT SO THE ROBOT DOES NOT GO OVER THE BLACK LINE AT THE BACKWARD
            motor_backward(speed,200);//#place holder time, pakitus   
        }
        else if(data.accX > 8000 && data.accY > 8000) //Look robot in the eyes, impact from the left back
        {   
            angle = 90 + ((atan2(data.accX,data.accY))*180/M_PI);
            printf("%8d %8d  Impact from the left back, impact was from %8d degrees\n",data.accX, data.accY, angle);
            speed = 255;//WE NEED A BRAKE POINT SO THE ROBOT DOES NOT GO OVER THE BLACK LINE AT THE BACKWARD
            motor_forward(speed,200);//#place holder time, pakitus
           
        }
        else if(data.accX < -8000 && data.accY < -8000) //Look robot in the eyes, impact from the right front
        {
            angle = 270 + ((atan2(abs(data.accX),abs(data.accY)))*180/M_PI);
            printf("%8d %8d  Impact from the right front, impact was from %8d degrees\n",data.accX, data.accY,angle );
            speed = 255;//WE NEED A BRAKE POINT SO THE ROBOT DOES NOT GO OVER THE BLACK LINE AT THE BACKWARD
            motor_backward(speed,200);//#place holder time, pakitus 
        }
        else if(data.accX > 8000 && data.accY < -8000) //Look robot in the eyes, impact from the right back
        {
            angle = 180 + ((atan2(abs(data.accX),abs(data.accY)))*180/M_PI);
            printf("%8d %8d  Impact from the right back, impact was from %8d degree\n",data.accX, data.accY, angle);
            speed = 255;//WE NEED A BRAKE POINT SO THE ROBOT DOES NOT GO OVER THE BLACK LINE AT THE BACKWARD
            motor_forward(speed,200);//#place holder time, pakitus
        
        }  }
    }
    
#endif

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

#if 0  //vko 5 tehtävä 3
 void zmain(void)
{
    double aika;
    TickType_t start, end;
    RTC_Start(); // start real time clock
    time_t alotus;
    time_t lopetus;
    int button = 1;
    int counter = 0;
    int change = 1;
    IR_Start();
    
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
            motor_forward(50,1);
            reflectance_digital(&dig);  
            if (change == 1 && dig.l3 == 1 && dig.l2 == 1 && dig.l1 == 1 && dig.r1 == 1 && dig.r2 == 1 && dig.r3 == 1) //detects and counts sections
            {
                counter++;
                if(counter == 1) //run to first section and stops 
                {
                    motor_forward(0,0);
                    IR_wait();
                    start=xTaskGetTickCount();
                    time(&alotus);
                } else if(counter == 2)
                {
                    motor_forward(0,0);
                    time(&lopetus);
                    aika = difftime(lopetus,alotus);
                    end=xTaskGetTickCount();
                    print_mqtt("ZUMO7/output" ,"%d",((end-start)/1000));
                    button = 1;
                    counter = 0;
                    change = 1;
                }
                change = 0;               
            }if(dig.l3 == 0 || dig.l2 == 0 || dig.l1 == 0 || dig.r1 == 0 || dig.r2 == 0 || dig.r3 == 0)
            {
                change = 1;
            }
        }
    }
}   
    
    
    
    
    
#endif
#if 0 //tehtävä 3 vko 5 //Ei toimiva tehtävä 5
void zmain(void)
{
    int button = 1;
    int counter = 0;
    int change = 1;
    IR_Start();
    RTC_Start(); // start real time clock
    
    RTC_TIME_DATE now;
    int hours;
    int minutes;
    int seconds;
    
    now.Hour = hours;
    now.Min = minutes;
    now.Sec = seconds;
    RTC_WriteTime(&now); // write the time to real time clock
    
    
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
                if(counter == 1) //run to the first intersection
                {
                    motor_forward(0,0);
                    printf("%d\n", counter);
                    IR_wait();
                    int seconds = 0;
                    // read the current time
                    RTC_DisableInt(); /* Disable Interrupt of RTC Component */
                    now = *RTC_ReadTime(); /* copy the current time to a local variable */
                    RTC_EnableInt(); /* Enable Interrupt of RTC Component */
                    // print the current time
                    printf("%02d", now.Sec);
                    
                    printf("Hello, motherfucker!\n");// print to screen (PuTTy)
                    printf("%2d\n", now.Sec);// print to screen (PuTTy)
                    print_mqtt("ZUMO7/output", "%2d:%02d", now.Hour, now.Min);// Send message to topic

                    //vTaskDelay(3000);
                } else if(counter == 2)
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

#if 0 //tehtävä 2 vko 5
void zmain(void)
{
int run = 1;
int d = 0;
motor_start();              // enable motor controller
motor_forward(0,0);         // set speed to zero to stop motors
Ultra_Start();
vTaskDelay(3000);
int random;

while (run == 1)
{
    d = Ultra_GetDistance();
    if (d < 10)
    {
        motor_forward(0,2);
        Beep(100,50);
        Beep(110,45);
        Beep(120,40); //You can remove all the beeps but the first one and it will work properly, also remove the stop at the start.
        Beep(130,35);
        Beep(140,30);
        Beep(150,25);
        Beep(160,20);
        Beep(170,15);
        Beep(180,10);
        Beep(190,5);
        
        
        motor_backward(50,1000);    // moving backward
        random = rand() %2;
        //motor_turn(100,200,1000);     // turn left 90 degrees
        if (random == 0)
        {
            motor_turn(150,0,500); //random turn (rand() % 200)
            print_mqtt("ZUMO7/output", "JOSEFIINA IS GOING RIGHT BUT TURNING LEFT" );// Send message to topic
        }
        else if (random == 1)
        {
            motor_turn(0,150,500); //random turn (rand() % 200)
            print_mqtt("ZUMO7/output", "JOSEFIINA IS GOING LEFT BUT TURNING RIGHT" );// Send message to topic
        }
    }
    motor_forward(120,1); 
    //printf("distance = %d\r\n", d);   
}   
}   
#endif

#if 0 //tehtävä 1 vko 5 //need to configure mqqt connection

void zmain(void)
{   
    RTC_Start(); // start real time clock
    
    RTC_TIME_DATE now;
    int hours;
    int minutes;
    vTaskDelay(10000);
    // set current time
    printf("Enter current time: hours ");
    scanf("%d",&hours);
    printf("Enter current time: minutes ");
    scanf("%d",&minutes);
    
    now.Hour = hours;
    now.Min = minutes;
    RTC_WriteTime(&now); // write the time to real time clock
    
    while(true)
    {
       if(SW1_Read() == 0) {
            // read the current time
            RTC_DisableInt(); /* Disable Interrupt of RTC Component */
            now = *RTC_ReadTime(); /* copy the current time to a local variable */
            RTC_EnableInt(); /* Enable Interrupt of RTC Component */

            // print the current time
            printf("%2d:%02d\n", now.Hour, now.Min);
            
            printf("Hello, motherfucker!\n");// print to screen (PuTTy)
            print_mqtt("ZUMO7/output", "%2d:%02d", now.Hour, now.Min);// Send message to topic

            // wait until button is released
            while(SW1_Read() == 0) vTaskDelay(50);
        }
        vTaskDelay(50);
    }
}   
#endif

#if 0 //testi korjaus robotin liikkeelle
void zmain(void)
{
 
    motor_start();              // enable motor controller
    motor_forward(0,0);         // set speed to zero to stop motors
    int button = 1;
    while (true)
    {
        button = SW1_Read();
        while (button == 0)
        {         
            motor_turn(100,90,900);
        }
    }
}   
#endif  
#if 1 //Line follow competition
void zmain(void)
{
    int button = 1;
    int counter = 0;
    int change = 1;
    int firstRound = 1;
    IR_Start();
    
    struct sensors_ ref;
    struct sensors_ dig;

    reflectance_start();
    reflectance_set_threshold(9000, 9000, 11000, 11000, 9000, 9000); // set center sensor threshold to 11000 and others to 9000

    motor_start();              // enable motor controller
    motor_forward(0,0);         // set speed to zero to stop motors
   
    
    while (true)
    {
        //button = SW1_Read();
        vTaskDelay(3000);
        while (true) //runs when button is pressed
        {         
            motor_forward(140,1); //120 toimii kun tank turn delay 5
            reflectance_digital(&dig); 
            //printf("%d\n", counter);
             
            if (change == 1 && dig.l3 == 1 && dig.l2 == 1 && dig.l1 == 1 && dig.r1 == 1 && dig.r2 == 1 && dig.r3 == 1) //detects and counts sections
            {
                counter++;
                motor_forward(100,5);
                if(counter == 1) //run to first section and stops 
                {
                    motor_forward(0,0);
                    //IR_wait();
                    vTaskDelay(3000);
                } else if(counter == 3)
                {
                    motor_forward(0,0);
                    button = 1;
                    counter = 0;
                    change = 1;
                    IR_wait();
                }
                change = 0;
            }
            /*while((dig.l2 == 1 && dig.l3 == 1)&& counter >0) //Tank turn left
            {
                if(firstRound == 1)
                {
                    motor_forward(0,1);
                }
                //Beep(1,100);
                tank_turn_left(150,1);
                reflectance_digital(&dig);
            }
            while((dig.r2 == 1 && dig.r3 == 1)&& counter >0) //Tank turn right
            {
                if(firstRound == 1)
                {
                    motor_forward(0,1);
                }
                //Beep(1,100);
                tank_turn_right(150,1);
                reflectance_digital(&dig);
            }
            firstRound = 1;
            */while((dig.l3 == 1 && dig.r3 == 0) && counter >0) //SUPRAJYRKKÄ käännös Vasemalle
            {
                if(firstRound == 1)
                {
                    motor_forward(0,1);
                }
                //Beep(1,100);
                tank_turn_left(150,10);
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                change = 1;
            }}
            while((dig.r3 == 1 && dig.l3 == 0) && counter >0) //SUPRAJYRKKÄ käännös OIKEALLE
            {
                if(firstRound == 1)
                {
                    motor_forward(0,1);
                }
                //Beep(1,100);
                tank_turn_right(150,10);
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                change = 1;
            }}
            firstRound = 1;
            while((dig.l1 == 0 && dig.r2 == 1) && counter >0) //Jyrkkä käännös oikealle
            {
                if(dig.l3 == 1)
                {
                    break;
                    Beep(1,100);
                }
                motor_turn(160,15,1); // turn right
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                change = 1;
            }}
            while((dig.r1 == 0 && dig.l2 == 1) && counter >0) //Jyrkkä käännös vasemmalle
            {
                if(dig.r3 == 1)
                {
                    break;
                    Beep(1,100);
                }
                motor_turn(15,160,1); // turn right
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                change = 1;
            }}
            /*while((dig.l1 == 0 && dig.r2 == 0) && counter >0) //Loiva käännös Oikealle
            {
                if(dig.l2 == 1)
                {
                    Beep(1,100);
                    break;  
                }
                motor_turn(110,100,1); // turn right
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                change = 1;
            }}
            while((dig.r1 == 0 && dig.l2 == 0) && counter >0) //Loiva käännös Vasemalle
            {
                if(dig.r2 == 1)
                {
                    Beep(1,100);
                    break;
                }
                motor_turn(100,110,1); // turn left
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                change = 1;
            }}*/
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
    IR_Start();
    
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
                    IR_wait();
                    //vTaskDelay(3000);
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

#if 0 //tehtävä 2 vko 4 VALMIS
void zmain(void)
{
    int button = 1;
    int counter = 0;
    int change = 1; // change is 1 when robot has moved over the line 
    //IR_Start();
    
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
            motor_forward(50,1);
             while(dig.l3 == 1 && dig.r3 == 0 && change == 1) //SUPRAJYRKKÄ käännös Vasemalle
            {
                motor_turn(15,250,20); // turn left
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0 && change == 1){ // tells program that section has been crossed
                change = 1;
            }}
            while(dig.r3 == 1 && dig.l3 == 0 && change == 1) //SUPRAJYRKKÄ käännös OIKEALLE
            {
                motor_turn(250,15,20); // turn right
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                change = 1;
            }}
            while(dig.l1 == 0 && dig.r2 == 1 && change == 1) //Jyrkkä käännös oikealle
            {
                motor_turn(160,15,1); // turn right
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                change = 1;
            }}
            while(dig.r1 == 0 && dig.l2 == 1 && change == 1) //Jyrkkä käännös vasemmalle
            {
                motor_turn(15,160,1); // turn right
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                change = 1;
            }}
            while(dig.l1 == 0 && dig.r2 == 0 && change == 1) //Loiva käännös Oikealle
            {
                motor_turn(110,100,1); // turn right
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                change = 1;
            }}
            while(dig.r1 == 0 && dig.l2 == 0 && change == 1) //Loiva käännös Vasemalle
            {
                motor_turn(100,110,1); // turn left
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                change = 1;
            }}          
            reflectance_digital(&dig); 
            //printf("%d\n", counter);
            if (change == 1 && dig.l3 == 1 && dig.l2 == 1 && dig.l1 == 1 && dig.r1 == 1 && dig.r2 == 1 && dig.r3 == 1) //detects and counts sections
            {
                counter++;
                if(counter == 1) //run if first section and stops 
                {
                    //IR_wait();
                    motor_forward(0,0);
                    vTaskDelay(3000);
                } else if (counter == 2)
                {
                    motor_forward(50,500);
                    tank_turn_left(100,535); //turn left 90 degrees  0,150,590
                    //motor_backward(150,50);
                } else if (counter < 5 && counter > 2)
                {   
                    motor_forward(50,500);
                    tank_turn_right(100,535); //turn right 90 degrees  150,0,590
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

#if 0 //tehtävä 3 vko 4
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
            motor_forward(100,5);
            reflectance_digital(&dig); 
            //printf("%d\n", counter);
            if (change == 1 && dig.l3 == 1 && dig.l2 == 1 && dig.l1 == 1 && dig.r1 == 1 && dig.r2 == 1 && dig.r3 == 1) //detects and counts sections
            {
                counter++;
                if(counter == 1) //run to first section and stops 
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
            while((dig.l3 == 0 || dig.l2 == 0) && counter >0) //turns left if one of the left sensors are black
            {
                motor_turn(160,15,1); // turn right
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                change = 1;
            }
            }
            while((dig.r2 == 0 || dig.r3 == 0) && counter >0) //turns right if one of the right sensors are black
            {
                motor_turn(15,160,1); // turn left
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                change = 1;
            }
                
            }
        }
    }
}
#endif

#if 0 //tehtävä 3 vko 3 (Matka Mordoriin) VALMIS
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
            motor_forward(150 , 1);
            random = rand() % 1000;
            LSM303D_Read_Acc(&data);
            printf("%8d %8d %8d\n",data.accX, data.accY, data.accZ);
            if (treshold > 100000 || treshold < -10000)
            {
                Beep(100,50);
                motor_backward(50,400);    // moving backward
                motor_turn(100,200,400);     // turn left 90 degrees
            }                      
            if (random == 1)
            {
            motor_turn((rand() % 100),0,(rand() % 400));
            }
            else if (random == 0)
            {
            motor_turn(0,(rand() % 100),(rand() % 400));
            }
            treshold = data.accX;
        }
    }
   while(true)
   {
      vTaskDelay(100);
   }
}   
#endif

#if 0 //tehtävä 1 vko 3
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
        //printf("distance = %d\r\n", d);
        printf("%f\n",getDistance());
        vTaskDelay(200); //200
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
#if 0 // Labyrintti Competition
    void zmain(void)
    {
        int koordinaatisto[6][13];
        int maali = 0;    
        int button = 1;
        int counter = 0;
        int muuttuja=0;
        int turnedLeft = 0;
        int changetwo = 0;
        int change = 1; // change is 1 when robot has moved over the line 
        
        struct sensors_ ref;
        struct sensors_ dig;
        
        Ultra_Start();

        reflectance_start();
        reflectance_set_threshold(9000, 9000, 11000, 11000, 9000, 9000); // set center sensor threshold to 11000 and others to 9000

        motor_start();              // enable motor controller
        motor_forward(0,0);         // set speed to zero to stop motors
        
        while (true)
        {
            button = SW1_Read();
            while (button == 0) //runs when button is pressed
            {        
                motor_forward(50,1);
                reflectance_digital(&dig);
                
                
                while(((dig.l1 == 0 && dig.r1 == 1) || (dig.l2 == 0 && dig.r1 == 1)) && change == 1) //Loiva käännös Oikealle
                {
                    motor_turn(110,80,1); // turn right
                    reflectance_digital(&dig);
                    if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                    change = 1;
                }}
                while(((dig.r1 == 0 && dig.l1 == 1) || (dig.r2 == 0 && dig.l1 == 1)) && change == 1) //Loiva käännös Vasemalle
                {
                    motor_turn(80,110,1); // turn left
                    reflectance_digital(&dig);
                    if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                    change = 1;
                }}          
                reflectance_digital(&dig);
                if (change == 1 && dig.l3 == 1 && dig.l2 == 1 && dig.l1 == 1 && dig.r1 == 1 && dig.r2 == 1 && dig.r3 == 1) //detects and counts sections
                {
                    //Ultra_GetDistance();
                    counter++;
                    motor_forward(50, 500);
                    if(counter == 1) //run if first section and stops 
                    {
                        motor_forward(0,0);
                        vTaskDelay(500);
                        //IR_wait();
                    } else if(maali == 1) // stops program when robot reaches goal
                    {
                        motor_forward(0,0);
                        button = 1;
                        counter = 0;
                        change = 1;
                    }
                    if(turnedLeft == 1) //if left was taken on last intersection turn right
                    {
                       // tank_turn_right(100,450); // 90 degrees to right
                        while(true)
                        {
                            reflectance_digital(&dig);
                            if(dig.l1 == 0 && dig.r1 == 0)
                            {
                                changetwo=1;
                            }
                            if(dig.l1 == 1 && dig.r1 == 1 && changetwo==1)
                            {
                                changetwo=0;
                                break;
                            }
                            
                            tank_turn_right(100,1);
                        }
                        turnedLeft = 0;
                    }
                    if(Ultra_GetDistance() < 20 && turnedLeft == 0) //if there is obstacle try left
                    {
                        //tank_turn_left(100,450); // 90 degrees to left
                        while(true)
                        {
                            reflectance_digital(&dig);
                             if(dig.l1 == 0 && dig.r1 == 0)
                            {
                                changetwo=1;
                            }
                            if(dig.l1 == 1 && dig.r1 == 1 && changetwo==1)
                            {
                                changetwo=0;
                                break;
                            }
                            tank_turn_left(100,1);
                        }
                       
                        if(Ultra_GetDistance() > 20)
                        {
                            turnedLeft = 1;
                        }
                    }
                    if(Ultra_GetDistance() < 20) //if left is not clear try right
                    {
                        //tank_turn_right(100,760); // 180 degrees to right
                        while(true)
                        {
                            reflectance_digital(&dig);
                             if(dig.l1 == 0 && dig.r1 == 0)
                            {
                                changetwo=1;
                            }
                            if(dig.l1 == 1 && dig.r1 == 1 && muuttuja == 0 && changetwo==1)
                            {
                                muuttuja++;
                            }else if(dig.l1 == 1 && dig.r1 == 1 && muuttuja == 1 && changetwo==1)
                            {
                                changetwo=0;
                                break;
                            }
                            tank_turn_right(100,1);
                        }
                        
                    }
                    change = 0;
                }
                while(dig.l3 == 0 && dig.l2 == 0 && dig.l1 == 0 && dig.r1 == 0 && dig.r2 == 0 && dig.r3 == 0) //if drives over bounds back up
                {
                    reflectance_digital(&dig);
                    motor_backward(50,1);
                }
                if(dig.l3 == 0 || dig.l2 == 0 || dig.l1 == 0 || dig.r1 == 0 || dig.r2 == 0 || dig.r3 == 0) //isolates double positive from section
                {
                    change = 1;
                }     
        }
    }
}
#endif

#if 0 //tehtävä 3 vko 4
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
            motor_forward(100,5);
            reflectance_digital(&dig); 
            //printf("%d\n", counter);
            if (change == 1 && dig.l3 == 1 && dig.l2 == 1 && dig.l1 == 1 && dig.r1 == 1 && dig.r2 == 1 && dig.r3 == 1) //detects and counts sections
            {
                counter++;
                if(counter == 1) //run to first section and stops 
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
            while((dig.l3 == 0 || dig.l2 == 0) && counter >0) //turns left if one of the left sensors are black
            {
                motor_turn(160,15,1); // turn right
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                change = 1;
            }
            }
            while((dig.r2 == 0 || dig.r3 == 0) && counter >0) //turns right if one of the right sensors are black
            {
                motor_turn(15,160,1); // turn left
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                change = 1;
            }
                
            }
        }
    }
}
    }    
#endif

//#perse
void tank_turn_left(uint8 speed,uint32 delay)
{
    MotorDirLeft_Write(1);      // set LeftMotor forward mode
    MotorDirRight_Write(0);     // set RightMotor forward mode
    PWM_WriteCompare1(speed); 
    PWM_WriteCompare2(speed); 
    vTaskDelay(delay);
}
void tank_turn_right(uint8 speed,uint32 delay)
{
    MotorDirLeft_Write(0);      // set LeftMotor forward mode
    MotorDirRight_Write(1);     // set RightMotor forward mode
    PWM_WriteCompare1(speed); 
    PWM_WriteCompare2(speed); 
    vTaskDelay(delay);
}
double getDistance(void)
{
    double summa = 0.0;
    int array[10];
    double keskiarvo = 0.0;
    
    for(int i = 0; i < 10; i++)
    {
        array[i] = Ultra_GetDistance();
    }
    for(int i = 0; i < 10; i++)
    {
        summa += array[i]; 
    }
    keskiarvo = summa/10;
    
    return keskiarvo;
    
}
/* [] END OF FILE */
