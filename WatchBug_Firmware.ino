/*****************************************************************************
The Geogram ONE is an open source tracking device/development board based off 
the Arduino platform.  The hardware design and software files are released 
under CC-SA v3 license.
*****************************************************************************/

#include <AltSoftSerial.h>
#include <PinChangeInt.h>
#include "GeogramONE.h"
#include <EEPROM.h>
#include <I2C.h>
#include "eepromAnything.h"

#define USEFENCE1			1  //set to zero to free up code space if option is not needed
#define USEFENCE2			1  //set to zero to free up code space if option is not needed
#define USEFENCE3			1  //set to zero to free up code space if option is not needed
#define USESPEED			1  //set to zero to free up code space if option is not needed
#define USEMOTION			1  //set to zero to free up code space if option is not needed

#define USEPOWERSWITCH		1


GeogramONE ggo;
AltSoftSerial GSM;
SIM900 sim900(&GSM);
geoSmsData smsData;
PA6C gps(&Serial); 
goCoord lastValid;
geoFence fence;


volatile uint8_t call;
volatile uint8_t move;
volatile uint8_t battery = 0;
volatile uint8_t charge = 0x02; // force a read of the charger cable
volatile uint8_t d4Switch = 0x00;
volatile uint8_t d10Switch = 0x00;

#if USEPOWERSWITCH
volatile uint8_t d11PowerSwitch;
#endif

uint8_t cmd0 = 0;
uint8_t cmd1 = 0;
uint8_t cmd3 = 0;
uint8_t udp = 0x00; 

//#if USEFENCE1
uint8_t fence1 = 0;
uint8_t breach1Conf = 0;
//#endif
//#if USEFENCE2
uint8_t fence2 = 0;
uint8_t breach2Conf = 0;
//#endif
//#if USEFENCE3
uint8_t fence3 = 0;
uint8_t breach3Conf = 0;
//#endif

uint8_t breachSpeed = 0;
uint8_t breachReps = 0;

uint32_t smsInterval = 0;
uint32_t udpInterval = 0;
uint32_t sleepTimeOn = 0;
uint32_t sleepTimeOff = 0;
uint8_t sleepTimeConfig = 0;

uint8_t speedHyst = 0;
uint16_t speedLimit = 0;

char udpReply[11];
uint8_t smsPowerProfile = 0;
uint8_t udpPowerProfile = 0;
uint8_t smsPowerSpeed = 0;
uint8_t udpPowerSpeed = 0;

bool gsmPowerStatus = true;

void goesWhere(char *, uint8_t replyOrStored = 0);
bool engMetric;

unsigned long   miniTimer ; 
#define TIMER_POLL_WITH_DATA        1000*60*1
#define TIMER_POLL_HEART_BEAT       1000*60*60
#define ENABLE_DEBUG_MESSAGES       false 
#define SETTING_WEBSERVER_URL       "http://salty-beyond-5800.herokuapp.com/hi"

// Need to put your provider's APN here
#define SETTING_GSM_APN             "giffgaff.com"
#define MAX_PHONENUMBER_SIZE 25 
char phoneNumber[MAX_PHONENUMBER_SIZE];
uint32_t apn;

void httpPost()
 
{
//  Serial.println("HTTP POST");
    // ToDo: only poll the GPS when we need to. 
    // ToDo: shut down the GPS if we don't need it. 
    // Update the GPS coordinates if possiable
//    gps.getCoordinates(&lastValid); 
//
//    if( millis() - miniTimer < TIMER_POLL_WITH_DATA ) {
//        // We are trying to poll sooner then we should.
//        return  ; // Nothing to do. 
//    }
//    
//    // Are we connected to the GPS?   
//    if( ! lastValid.signalLock  ) {
//        // We are not connected to the GPS. 
//        // Has our heart beat timer expired? 
//        if( millis() - miniTimer < TIMER_POLL_HEART_BEAT ) {
//            // We are trying to poll sooner then we should.
//            return  ; // Nothing to do. 
//        }  
//    } 
//    miniTimer = millis() ; 
//    
//    
    
    // Wake up the modem. 
    // DebugPrint( "Waiting up the GSM modem"); 
  sim900.gsmSleepMode(0);
    
  GSM.println("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  sim900.confirmAtCommand("OK",5000);
  
  GSM.print("AT+SAPBR=3,1,\"APN\",\"");
    GSM.print( SETTING_GSM_APN );
    GSM.println("\""); 
  sim900.confirmAtCommand("OK",5000);

  GSM.println("AT+SAPBR=3,1,\"USER\",\"giffgaff\"");
    sim900.confirmAtCommand("OK",5000);
   
  
  GSM.println("AT+SAPBR=1,1");
  sim900.confirmAtCommand("OK",5000);// Tries to connect GPRS 
  
  GSM.println("AT+HTTPINIT");
  sim900.confirmAtCommand("OK",5000);
  
  GSM.println("AT+HTTPPARA=\"CID\",1");
  sim900.confirmAtCommand("OK",5000);
  
    //web address to send data to
  GSM.print("AT+HTTPPARA=\"URL\",\"");
    GSM.print(SETTING_WEBSERVER_URL);
    GSM.print("?id="); 
    GSM.print(phoneNumber); 
    
    // If we have GPS lock we should send the GPS data. 
    if( lastValid.signalLock ) {
          GSM.print("&lat=");
          if(lastValid.ns == 'S') {
              GSM.print("-");
          }
          GSM.print(lastValid.latitude[0]);
          GSM.print(lastValid.latitude[1]);
          GSM.print("+");
          GSM.print(lastValid.latitude + 2);
          
          GSM.print("&lon=");
          if(lastValid.ew == 'W') {
              GSM.print("-");
          }
          GSM.print(lastValid.longitude[0]);
          GSM.print(lastValid.longitude[1]);
          GSM.print(lastValid.longitude[2]);
          GSM.print("+");
          GSM.print(lastValid.longitude + 3);
          
          GSM.print("&alt=");
          GSM.print(lastValid.altitude);  
        
          GSM.print("&sat=");
          GSM.print(lastValid.satellitesUsed);          
          
          GSM.print("&speed=");
          GSM.print(lastValid.speed);  
          
          GSM.print("&dir=");
          GSM.print(lastValid.courseDirection);  
          
          /*
          // YYYY-MM-DD
          GSM.print("&date=");          
          GSM.print(lastValid.date + 4 );  
          GSM.print("-" );  
          GSM.print(lastValid.date[2] );  
          GSM.print(lastValid.date[3] );  
          GSM.print("-" );  
          GSM.print(lastValid.date[0] );  
          GSM.print(lastValid.date[1] );  
          
          // HH::MM::SS
          GSM.print("&time=");
          GSM.print(lastValid.time[0] );  
          GSM.print(lastValid.time[1] );  
          GSM.print("-" );  
          GSM.print(lastValid.time[2] );  
          GSM.print(lastValid.time[3] );  
          GSM.print("-" );  
          GSM.print(lastValid.time +4 );  
          */
          // Raw
          /*
          GSM.print("&raw1=");
          for( int i = 0 ; i < 10 ; i++ ) {
            GSM.print(lastValid.latitude[i], HEX);
          }
          GSM.print("&raw2=");
          for( int i = 0 ; i < 11 ; i++ ) {
            GSM.print(lastValid.longitude[i], HEX);
          }
          */
          
    } else {
        GSM.print("&err=NoSignalLock");
    }    
    
    // Get the battery state 
      GSM.print("&batp=");          
      GSM.print(MAX17043getBatterySOC()/100);  
      
      GSM.print("&batv=");          
      GSM.print( MAX17043getBatteryVoltage()/1000.0, 2 );      
    
    // All done send the message. 
    GSM.println("\"");    
  sim900.confirmAtCommand("OK",5000);
  
    
  GSM.println("AT+HTTPDATA=2,10000"); 
  sim900.confirmAtCommand("DOWNLOAD",5000);
  GSM.println("0"); // ToDo: But prameters here instead of the url. 
  
  sim900.confirmAtCommand("OK",5000);
  
  GSM.println("AT+HTTPACTION=1"); //POST the data
  sim900.confirmAtCommand("ACTION:",5000);
  
    delay (3000); 
  GSM.println("AT+HTTPTERM"); //terminate http
  sim900.confirmAtCommand("OK",5000);
  
  GSM.println("AT+SAPBR=0,1");// Disconnect GPRS
  sim900.confirmAtCommand("OK",5000);
  sim900.confirmAtCommand("DEACT",5000);
  
    // Put the modem to sleep.
  sim900.gsmSleepMode(2);
}
uint8_t GetPhoneNumber() {

    // Request the phone number from the sim card 
    GSM.println("AT+CNUM");
    
    // Wait for a response. 
  if( sim900.confirmAtCommand("OK",5000) == 0 ) {
        // Extract Phone number from the response. 
        // Search for the start of the string. 
        char * startOfPhoneNumber = strstr( sim900.atRxBuffer, "\",\"" ); 
        if( startOfPhoneNumber != NULL ) { 
            // Found the start of the string 
            startOfPhoneNumber += 3 ; // Move past the header. 
            if( startOfPhoneNumber[0] == '+' ) {
                startOfPhoneNumber++; // Move past the plus
            }
            char * endOfPhoneNumber = strstr( startOfPhoneNumber, "\"" ); 
            if( endOfPhoneNumber != NULL ) {
                // Found the end of the string. 
                if( endOfPhoneNumber - startOfPhoneNumber < MAX_PHONENUMBER_SIZE-1 ) {                
                    // Fits in the buffer 
                    strncpy( phoneNumber, startOfPhoneNumber, endOfPhoneNumber - startOfPhoneNumber ) ;                
                    phoneNumber[ endOfPhoneNumber - startOfPhoneNumber ] = 0 ; 
                    return 1; 
                }
            }
        }
    }
    return 0; 
}

void setup(){
  Serial.begin(9600);
  ggo.init();
  gps.init(115200);
  sim900.init(9600);
        
  MAX17043init(7, 500);
  BMA250init(3, 500);
    
    // Get the phone number from the simcard
    GetPhoneNumber(); 
    
    // Reset the timer 
    miniTimer = 0 ; 
}
void setup_old()
{
	ggo.init();
	gps.init(115200);
	sim900.init();
	MAX17043init(7, 500);
	BMA250init(3, 500);
	attachInterrupt(0, ringIndicator, FALLING);
	attachInterrupt(1, movement, FALLING);
	PCintPort::attachInterrupt(PG_INT, &charger, CHANGE);
	PCintPort::attachInterrupt(FUELGAUGEPIN, &lowBattery, FALLING);
	goesWhere(smsData.smsNumber);
	call = sim900.checkForMessages();
	if(call == 0xFF)
		call = 0;
	battery = MAX17043getAlertFlag();
	EEPROM_readAnything(APN,apn);
	#if USEFENCE1
	ggo.getFenceActive(1, &fence1);
	#endif
	#if USEFENCE2
	ggo.getFenceActive(2, &fence2);
	#endif
	#if USEFENCE3
	ggo.getFenceActive(3, &fence3);
	#endif
	#if USESPEED
	ggo.configureSpeed(&cmd3, &speedHyst, &speedLimit);
	#endif
	ggo.configureBreachParameters(&breachSpeed, &breachReps);
	ggo.configureSleepTime(&sleepTimeOn, &sleepTimeOff, &sleepTimeConfig);
	BMA250enableInterrupts();
	uint8_t swInt = EEPROM.read(IOSTATE0);
	if(swInt == 0x05)
		PCintPort::attachInterrupt(4, &d4Interrupt, RISING);
	if(swInt == 0x06)
		PCintPort::attachInterrupt(4, &d4Interrupt, FALLING);
	swInt = EEPROM.read(IOSTATE1);
	if(swInt == 0x05)
		PCintPort::attachInterrupt(10, &d10Interrupt, RISING);
	if(swInt == 0x06)
		PCintPort::attachInterrupt(10, &d10Interrupt, FALLING);
	#if USEPOWERSWITCH
	pinMode(11,INPUT);
	digitalWrite(11,HIGH);
	PCintPort::attachInterrupt(11, &d11Interrupt, FALLING);
	#endif
}


void loop(){
  httpPost();
}
void loop_old()
{
	if(!gps.getCoordinates(&lastValid))
	{
		int8_t tZ = EEPROM.read(TIMEZONE);
		bool eM = EEPROM.read(ENGMETRIC);
		gps.updateRegionalSettings(tZ, eM, &lastValid);
		EEPROM_readAnything(APN,apn);
	}
	if(call)
	{
		sim900.gsmSleepMode(0);
		char pwd[5];
		EEPROM_readAnything(PINCODE,pwd);
		if(sim900.signalQuality())
		{
			if(!sim900.getGeo(&smsData, pwd))
			{
				if(!smsData.smsPending)
					call = 0; // no more messages
				if(smsData.smsDataValid)
				{
					if(!smsData.smsCmdNum)
						cmd0 = 0x01;
					else if(smsData.smsCmdNum == 1)
						cmd1 = 0x01;
					else if(smsData.smsCmdNum == 2)
						command2();
					else if(smsData.smsCmdNum == 3)
						cmd3 = 0x01;
					else if(smsData.smsCmdNum == 4)
						command4();
					else if(smsData.smsCmdNum == 5)
						command5();
					else if(smsData.smsCmdNum == 6)
						command6();
					else if(smsData.smsCmdNum == 7)
						command7();
					else if(smsData.smsCmdNum == 8)
						command8();
					else if(smsData.smsCmdNum == 255)
					{
						BMA250configureMotion();
						sim900.gsmSleepMode(0);
						sim900.rebootGSM();
						gsmPowerStatus = true;
					}
				}
			}
		}
		sim900.gsmSleepMode(2);	
	}
	if(cmd0)
		command0();
	#if USEMOTION
	if(cmd1)
		command1();
	#endif
	#if USESPEED
	if(cmd3)
		command3();
	#endif
	if(udp)
	{
		if(lastValid.signalLock && (lastValid.updated & 0x01))
		{
			if((udpInterval > 5))
				sim900.gsmSleepMode(0);
			if(!udpOrange())
			{
				udp = 0;
				lastValid.updated &= ~(0x01);
				if(udpInterval > 5)
					sim900.gsmSleepMode(2);
			}
		}
	}
	
	if(battery)
	{
		sim900.gsmSleepMode(0);
		goesWhere(smsData.smsNumber,1);
		if(!sim900.prepareSMS(smsData.smsNumber,apn))
		{
			printEEPROM(BATTERYMSG);
			if(!sim900.sendSMS())
			{
				battery = 0;
				MAX17043clearAlertFlag();
			}
		}
		sim900.gsmSleepMode(2);
	}
	if(charge & 0x02)
		chargerStatus();
	engMetric = EEPROM.read(ENGMETRIC);
	#if USEFENCE1
	if(fence1)
	{
		if((fence1 == 1) && (lastValid.speed >= breachSpeed))
		{
			ggo.configureFence(1,&fence); 
			if(!gps.geoFenceDistance(&lastValid, &fence, engMetric))
			{
				if(lastValid.updated & 0x02)
					breach1Conf++;
				if(breach1Conf > breachReps)
				{
					fence1 = 2;
					breach1Conf = 0;
				}
				lastValid.updated &= ~(0x02); 
			}
			else
				breach1Conf = 0;
		}
		else
			breach1Conf = 0;
		if(fence1 == 2)
		{
			sim900.gsmSleepMode(0);
			goesWhere(smsData.smsNumber,1);
			if(!sim900.prepareSMS(smsData.smsNumber,apn))
			{
				printEEPROM(FENCE1MSG);
				if(!sim900.sendSMS())
					fence1 = 0x00;
			}
			sim900.gsmSleepMode(2);
		}
	}
	#endif
	#if USEFENCE2
	if(fence2)
	{
		if((fence2 == 1) && (lastValid.speed >= breachSpeed))
		{  
			ggo.configureFence(2,&fence);
			if(!gps.geoFenceDistance(&lastValid, &fence, engMetric))
			{
				if(lastValid.updated & 0x04)
					breach2Conf++;
				if(breach2Conf > breachReps)
				{
					fence2 = 2;
					breach2Conf = 0;
				}
				lastValid.updated &= ~(0x04);
			}
			else
				breach2Conf = 0;
		}
		else
			breach2Conf = 0;
		if(fence2 == 2)
		{  
			sim900.gsmSleepMode(0);
			goesWhere(smsData.smsNumber,1);
			if(!sim900.prepareSMS(smsData.smsNumber,apn))
			{
				printEEPROM(FENCE2MSG);
				if(!sim900.sendSMS())
					fence2 = 0x00;
			}
			sim900.gsmSleepMode(2);
		}
	}
	#endif
	#if USEFENCE3
	if(fence3)
	{
		if((fence3 == 1) && (lastValid.speed >= breachSpeed))
		{  
			ggo.configureFence(3,&fence);
			if(!gps.geoFenceDistance(&lastValid, &fence, engMetric))
			{
				if(lastValid.updated & 0x08)
					breach3Conf++;
				if(breach3Conf > breachReps)
				{
					fence3 = 2;
					breach3Conf = 0;
				}
				lastValid.updated &= ~(0x08);
			}
			else
				breach3Conf = 0;
		}
		else
			breach3Conf = 0;
		if(fence3 == 2)
		{  
			sim900.gsmSleepMode(0);
			goesWhere(smsData.smsNumber,1);
			if(!sim900.prepareSMS(smsData.smsNumber,apn))
			{
				printEEPROM(FENCE3MSG);
				if(!sim900.sendSMS())
					fence3 = 0x00;
			}	
			sim900.gsmSleepMode(2);
		}
	}
	#endif
	if(smsInterval)
		smsTimerMenu();
	if(udpInterval)
		udpTimerMenu();
	if(sleepTimeOn && (sleepTimeOff || (sleepTimeConfig & 0x08)))
		sleepTimer();
	if(d4Switch)
	{
		sim900.gsmSleepMode(0);
		goesWhere(smsData.smsNumber,1);
		if(!sim900.prepareSMS(smsData.smsNumber,apn))
		{
			printEEPROM(D4MSG);
			if(!sim900.sendSMS())
				d4Switch = 0x00;
		}
		sim900.gsmSleepMode(2);
	}
	if(d10Switch)
	{
		sim900.gsmSleepMode(0);
		goesWhere(smsData.smsNumber,1);
		if(!sim900.prepareSMS(smsData.smsNumber,apn))
		{
			printEEPROM(D10MSG);
			if(!sim900.sendSMS())
				d10Switch = 0x00;
		}
		sim900.gsmSleepMode(2);
	}
	#if USEPOWERSWITCH
	if(d11PowerSwitch)
		onOffSwitch();
	#endif
	if(gsmPowerStatus)
		sim900.initializeGSM();
} 

void printEEPROM(uint16_t eAddress)
{
	char eepChar;
	for (uint8_t ep = 0; ep < 50; ep++)
	{
		eepChar = EEPROM.read(ep + eAddress);
		if(eepChar == '\0')
			break;
		else
			GSM.print(eepChar);
	}
}

void goesWhere(char *smsAddress, uint8_t replyOrStored)
{
	if(replyOrStored == 3) 
		EEPROM_readAnything(RETURNADDCONFIG,replyOrStored);
	if((replyOrStored == 1))
		for(uint8_t l = 0; l < 39; l++)
		{
				smsAddress[l] = EEPROM.read(l + SMSADDRESS);
				if(smsAddress[l] == NULL)
					break;
		}
}


#if USEPOWERSWITCH
void onOffSwitch()
{
	delay(3000);
	if(digitalRead(11))
	{
		d11PowerSwitch = 0;
		return;
	}
	BMA250sleepMode(0x80);
	MAX17043sleep(false);
	sim900.powerDownGSM();
	gps.sleepGPS();
	BMA250disableInterrupts();
	ggo.goToSleep(SLEEP_MODE_PWR_DOWN, false, true);
	while(1)
	{
		delay(3000);
		if(!digitalRead(11))
			break;
		ggo.goToSleep(SLEEP_MODE_PWR_DOWN, false, true);
	}
	MAX17043sleep(true);
	BMA250sleepMode(0x00);
	BMA250enableInterrupts();
	gps.wakeUpGPS();
	sim900.initializeGSM();
	gsmPowerStatus = true;
	d11PowerSwitch = 0;
}

#endif
