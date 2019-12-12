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
    }else {
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
        while (attack == 1) { //back to center(future) // toimii oikein
        
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
#endif  //Sumo Competion

#if 0 //Line follow competition
void zmain(void)
{
    int button = 1;
    int counter = 0;
    int change = 1;
    int oppakone = 0;
    int firstRound = 1;
    IR_Start();
    TickType_t start, end, miss, line;
    
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
        start = xTaskGetTickCount();
        print_mqtt("ZUMO7/start", "%d", xTaskGetTickCount());// Send message to topic
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
                    end  = xTaskGetTickCount();
                    print_mqtt("ZUMO7/stop", "%d", (end));// Send message to topic
                    print_mqtt("ZUMO7/lap time", "%d", (end-start));// Send message to topic
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
                if(dig.r1 == 0 && dig.l1 == 0 && oppakone == 0)
                {   
                    miss = xTaskGetTickCount();
                    print_mqtt("ZUMO7/miss", "%d", (miss));// Send message to topic
                    oppakone=1;
                }
                if(dig.r1 == 1 && dig.l1 == 1 && oppakone== 1)
                {   
                    line = xTaskGetTickCount();
                    print_mqtt("ZUMO7/line", "%d", (line));// Send message to topic
                    oppakone=0;
                }
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
                if(dig.r1 == 0 && dig.l1 == 0 && oppakone == 0)
                {   
                    miss = xTaskGetTickCount();
                    print_mqtt("ZUMO7/miss", "%d", (miss));// Send message to topic
                    oppakone=1;
                }
                if(dig.r1 == 1 && dig.l1 == 1 && oppakone== 1)
                {   
                    line = xTaskGetTickCount();
                    print_mqtt("ZUMO7/line", "%d", (line));// Send message to topic
                    oppakone=0;
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
                if(dig.r1 == 0 && dig.l1 == 0 && oppakone == 0)
                {   
                    miss = xTaskGetTickCount();
                    print_mqtt("ZUMO7/miss", "%d", (miss));// Send message to topic
                    oppakone=1;
                }
                if(dig.r1 == 1 && dig.l1 == 1 && oppakone== 1)
                {   
                    line = xTaskGetTickCount();
                    print_mqtt("ZUMO7/line", "%d", (line));// Send message to topic
                    oppakone=0;
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
                if(dig.r1 == 0 && dig.l1 == 0 && oppakone == 0)
                {   
                    miss = xTaskGetTickCount();
                    print_mqtt("ZUMO7/miss", "%d", (miss));// Send message to topic
                    oppakone=1;
                }
                if(dig.r1 == 1 && dig.l1 == 1 && oppakone== 1)
                {   
                    line = xTaskGetTickCount();
                    print_mqtt("ZUMO7/line", "%d", (line));// Send message to topic
                    oppakone=0;
                }
                motor_turn(15,160,1); // turn right
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                change = 1;
            }}
            /*
            while((dig.l1 == 0 && dig.r2 == 0) && counter >0) //Loiva käännös Oikealle
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
#endif //line following competiton

#if 1 // Labyrintti Competition
    void zmain(void)
    {
        int maali = 0;    
        int button = 1;
        int counter = 0;
        int muuttuja=0;
        int turnedLeft = 0;
        int changetwo = 0;
        int change = 1; // change is 1 when robot has moved over the line 
        int x = 0;
        int y = 0;
        int back = 0;
        int forward = 1;
        int uTurn = 0;
        int approachAngle = 0; // get negative value if comming from left and positive if from right
        
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
            while(((dig.l1 == 0 && dig.r1 == 1) || (dig.l2 == 0 && dig.r1 == 1)) && change == 1) //slight right
            {
                motor_turn(100,65,1); // turn right
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                    change = 1;
                }
            }
            while(((dig.r1 == 0 && dig.l1 == 1) || (dig.r2 == 0 && dig.l1 == 1)) && change == 1) //slight left
            {
                motor_turn(65,100,1); // turn left
                reflectance_digital(&dig);
                if(dig.l3 == 0 || dig.r3 == 0){ // tells program that section has been crossed
                    change = 1;
                }
            }          
            reflectance_digital(&dig);
            if (change == 1 && ((dig.l3 == 1 && dig.l2 == 1 && dig.l1 == 1 && dig.r1 == 1 ) || (dig.r3 == 1 && dig.r2 == 1 && dig.l1 == 1 && dig.r1 == 1 ))) //detects and counts sections
            {
                counter++;
                if (y == 11) // finnishline sequence
                {
                    if(x < 0) // turn to middle from left
                    {
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
                            approachAngle = -1;
                            x++;
                        }
                    } else if(x > 0) // turn to middle from right
                    {
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
                            tank_turn_left(100,1); //Left turn
                        }
                        approachAngle = 1;
                        x--;
                    }else if(x == 0 && forward == 1)
                    { //when x = 0 drive to finnish
                        while (true)
                        {
                            while(((dig.l1 == 0 && dig.r1 == 1) || (dig.l2 == 0 && dig.r1 == 1)) && change == 1) //Loiva käännös Oikealle Keskitys
                            {
                                motor_turn(100,65,1); // turn right
                                reflectance_digital(&dig);
                                if(dig.l3 == 0 || dig.r3 == 0)// tells program that section has been crossed
                                { 
                                    change = 1;
                                }
                            }
                            while(((dig.r1 == 0 && dig.l1 == 1) || (dig.r2 == 0 && dig.l1 == 1)) && change == 1) //Loiva käännös Vasemalle Keskitys
                            {
                                motor_turn(65,100,1); // turn left
                                reflectance_digital(&dig);
                                if(dig.l3 == 0 || dig.r3 == 0) // tells program that section has been crossed
                                {
                                    change = 1;
                                }
                            }     
                        }
                    }    
                    motor_forward(100, 1);
                    if (dig.l3 == 0 && dig.l2 == 0 && dig.l1 == 0 && dig.r1 == 0 && dig.r2 == 0 && dig.r3 == 0)
                    {
                        motor_forward(0,0);
                        IR_wait();
                    }
                } 
                if (counter > 1 && forward == 1)
                {
                    y++;
                }
                if(back > 0)
                {
                    back--;
                }
                motor_forward(50, 350);
                if(counter == 1) //run if first section and stops 
                {
                    motor_forward(0,0);
                    vTaskDelay(500);
                    //IR_wait();
                } /*else if(maali == 1) // stops program when robot reaches goal
                {
                    motor_forward(0,0);
                    button = 1;
                    counter = 0;
                    change = 1;
                }*/
                if(turnedLeft == 1 && back == 0) //if left was taken on last intersection turn right
                {
                    forward = 0;
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
                    if (getDistance() > 20) //if right is clear
                    {
                        turnedLeft = 0;
                        forward = 1;
                    }
                }
            
                if(getDistance() < 20 && turnedLeft == 0 && back == 0) //if there is obstacle try left
                {
                    forward = 0;
                    while(true) // 90 degrees to left
                    {
                        reflectance_digital(&dig);
                        if(dig.l1 == 0 && dig.r1 == 0) // spots when robot has moved off the line
                        {
                            changetwo=1;
                        }
                        if(dig.l1 == 1 && dig.r1 == 1 && changetwo==1) // spot when robot has moved back to the line and stops while loop
                        {
                            changetwo=0;
                            break;
                        }
                        tank_turn_left(100,1); //Left turn
                    }
                    
                    if(getDistance() > 20) //no obstacle on left and updates x
                    {
                        x--;                           
                    }
                    turnedLeft = 1;
                }
                    
                if(back == 0 && uTurn == 1) // after 180 degree turn right try left
                {
                    forward = 0;
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
                        tank_turn_left(100,1); //Left turn
                    }
                    
                    if(getDistance() > 20) // checks for obstacle and updates x
                    {    
                        uTurn = 0;
                        x++;
                        forward = 1;
                    }
                }
                    
                if(getDistance() < 10 && back == 0) //if left is not clear try right // 180 degrees to right
                {
                    while(true)
                    {
                        reflectance_digital(&dig);
                        if(dig.l1 == 0 && dig.r1 == 0)
                        {
                            changetwo=1;
                        }
                        if(dig.l1 == 1 && dig.r1 == 1 && muuttuja == 0 && changetwo==1) //detects 90 degree turn
                        {
                            muuttuja=1;
                        }else if(dig.l1 == 1 && dig.r1 == 1 && muuttuja == 1 && changetwo==1)//detects 180 degree turn
                        {
                            changetwo = 0;
                            muuttuja = 0;
                            x++;
                            back = 2;
                            turnedLeft=0;
                            uTurn = 1;                                
                            break;
                        }
                        tank_turn_right(100,1); 
                    }
                }
                change = 0;
                print_mqtt("ZUMO7/koordinaatit", "%d y %d x", y, x);// Send message to topic debugging code
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
    
#endif // Labyrintti Competition


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
