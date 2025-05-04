//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

#include "user_interface.h"

#include "code.h"
#include "siren.h"
#include "smart_home_system.h"
#include "fire_alarm.h"
#include "date_and_time.h"
#include "temperature_sensor.h"
#include "gas_sensor.h"
#include "matrix_keypad.h"
#include "display.h"

//=====[Declaration of private defines]========================================

#define DISPLAY_REFRESH_TIME_MS 1000

//=====[Declaration of private data types]=====================================

//=====[Declaration and initialization of public global objects]===============

DigitalOut incorrectCodeLed(LED3);
DigitalOut systemBlockedLed(LED2);

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

char codeSequenceFromUserInterface[CODE_NUMBER_OF_KEYS];

//=====[Declaration and initialization of private global variables]============

static bool incorrectCodeState = OFF;
static bool systemBlockedState = OFF;

static bool codeComplete = false;
static int numberOfCodeChars = 0;
bool coldStart = true;
//=====[Declarations (prototypes) of private functions]========================

static void userInterfaceMatrixKeypadUpdate();
static void incorrectCodeIndicatorUpdate();
static void systemBlockedIndicatorUpdate();

static void userInterfaceDisplayInit();
static void userInterfaceDisplayUpdate();

//=====[Implementations of public functions]===================================

void userInterfaceInit()
{
    incorrectCodeLed = OFF;
    systemBlockedLed = OFF;
    matrixKeypadInit( SYSTEM_TIME_INCREMENT_MS );
    userInterfaceDisplayInit();
}

void userInterfaceUpdate()
{
    userInterfaceMatrixKeypadUpdate();
    incorrectCodeIndicatorUpdate();
    systemBlockedIndicatorUpdate();
    userInterfaceDisplayUpdate();
}

bool incorrectCodeStateRead()
{
    return incorrectCodeState;
}

void incorrectCodeStateWrite( bool state )
{
    incorrectCodeState = state;
}

bool systemBlockedStateRead()
{
    return systemBlockedState;
}

void systemBlockedStateWrite( bool state )
{
    systemBlockedState = state;
}

bool userInterfaceCodeCompleteRead()
{
    return codeComplete;
}

void userInterfaceCodeCompleteWrite( bool state )
{
    codeComplete = state;
}

//=====[Implementations of private functions]==================================

void reportLCDKey23(char keyReleased)
{
    if (keyReleased == '2')
    {
        //print report of gas detector on line 4 for gas detector state
        displayCharPositionWrite ( 0,3 ); //char 0 line 4
        if ( gasDetectorStateRead() ) {
            displayStringWrite( "Gas Detector State: ON" );
        } else {
            displayStringWrite( "Gas Detector State: OFF" );
        }
        
    }else if(keyReleased == '3')
    {
        //print report of temp detector on line 4 for temp detector state
        displayCharPositionWrite ( 0,3 ); //char 0 line 4
        if ( overTemperatureDetectorStateRead() ) {
            displayStringWrite( "Ov. Temp. State: ON" );
        } else {
            displayStringWrite( "Ov. Temp. State: OFF" );
        }
    }

}
//temperatureSensorReadCelsius() 
void displaypWarningUpdate(float tempClm32)
{
    if(tempClm32>32.0) //warning at 32 degree C
    {
        displayCharPositionWrite ( 0,3 ); //char 0 line 4
        displayStringWrite( "Over Temp. Warning!" );
    }
}
static void userInterfaceMatrixKeypadUpdate()
{
    static int numberOfHashKeyReleased = 0;
    char keyReleased = matrixKeypadUpdate();

    if( keyReleased != '\0' ) {

        reportLCDKey23(keyReleased); //main task i
        if( sirenStateRead() && !systemBlockedStateRead() ) {
            if( !incorrectCodeStateRead() ) {
                codeSequenceFromUserInterface[numberOfCodeChars] = keyReleased;
                numberOfCodeChars++;
                if ( numberOfCodeChars >= CODE_NUMBER_OF_KEYS ) {
                    codeComplete = true;
                    numberOfCodeChars = 0;
                }
            } else {
                if( keyReleased == '#' ) {
                    numberOfHashKeyReleased++;
                    if( numberOfHashKeyReleased >= 2 ) {
                        numberOfHashKeyReleased = 0;
                        numberOfCodeChars = 0;
                        codeComplete = false;
                        incorrectCodeState = OFF;
                    }
                }
            }
        }
    }
}

static void userInterfaceDisplayInit()
{
    displayInit( DISPLAY_CONNECTION_I2C_PCF8574_IO_EXPANDER );
     
    displayCharPositionWrite ( 0,0 );
    displayStringWrite( "Temperature:" );

    displayCharPositionWrite ( 0,1 );
    displayStringWrite( "Gas:" );
    
    displayCharPositionWrite ( 0,2 );
    displayStringWrite( "Alarm:" );
}
void promptUser2DeactivateAlarm(bool siren)
{

    if(siren && coldStart)
    {
        displayCharPositionWrite ( 10,2 );
        displayStringWrite( ",Enter Code to" );
        displayCharPositionWrite ( 0,3 );
        displayStringWrite( "Deactivate Alarm" );
    }
}
static void userInterfaceDisplayUpdate()
{
    static int accumulatedDisplayTime = 0;
    char temperatureString[3] = "";
    
    if( accumulatedDisplayTime >=
        DISPLAY_REFRESH_TIME_MS ) {

        accumulatedDisplayTime = 0;

        sprintf(temperatureString, "%.0f", temperatureSensorReadCelsius());
        displayCharPositionWrite ( 12,0 );
        displayStringWrite( temperatureString );
        displayCharPositionWrite ( 14,0 );
        displayStringWrite( "'C" );

        displayCharPositionWrite ( 4,1 );

        if ( gasDetectorStateRead() ) {
            displayStringWrite( "Detected    " );
        } else {
            displayStringWrite( "Not Detected" );
        }

        displayCharPositionWrite ( 6,2 );
        
        if ( sirenStateRead() ) { //main task ii,iii
            displayStringWrite( "ON " );
        } else {
            displayStringWrite( "OFF" );
        }
        displaypWarningUpdate(temperatureSensorReadCelsius()); //display warning if C>32 on line 4 maint task iv
        promptUser2DeactivateAlarm(sirenStateRead()); //at cold start
    } else {
        accumulatedDisplayTime =
            accumulatedDisplayTime + SYSTEM_TIME_INCREMENT_MS;        
    } 
}

static void incorrectCodeIndicatorUpdate()
{
    incorrectCodeLed = incorrectCodeStateRead();
}

static void systemBlockedIndicatorUpdate()
{
    systemBlockedLed = systemBlockedState;
}