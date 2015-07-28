#define SETTING_WEBSERVER_URL       "http://watchbug-api.herokuapp.com"

// Need to put your provider's APN here
#define SETTING_GSM_APN             "giffgaff.com"
void httpPosition()  //send coordinates
{
        if (lastValid.signalLock){
          sim900.gsmSleepMode(0);
          Serial.println("Ready to send the gps position");  
  	  uint16_t geoDataFormat;
  	  uint8_t rssi = sim900.signalQuality();
          Serial.print("RSSI: ");  
          Serial.println(rssi);  
  	  if(rssi)
  	  {	
  	    sendHTTP(rssi);		
  	  }
  	  sim900.gsmSleepMode(2);
       }else{
       Serial.println("THE SIGNAL IS NOT LOCKED");   
     }
	
}

void sendHTTP(uint8_t rssi)
{
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
      GSM.print("/geolocations/100"); 
         
      GSM.print("?lat=");
        if(lastValid.ns == 'S')
		GSM.print("-");
	GSM.print(lastValid.latitude[0]);
	GSM.print(lastValid.latitude[1]);
	GSM.print("+");
	GSM.print(lastValid.latitude + 2);
          
        GSM.print("&long=");
         if(lastValid.ew == 'W')
		GSM.print("-");
	GSM.print(lastValid.longitude[0]);
	GSM.print(lastValid.longitude[1]);
	GSM.print(lastValid.longitude[2]);
	GSM.print("+");
	GSM.print(lastValid.longitude + 3);
         
        GSM.println("\"");    
        sim900.confirmAtCommand("OK",5000);
  
    
        GSM.println("AT+HTTPDATA=2,10000"); 
        sim900.confirmAtCommand("DOWNLOAD",5000);
        GSM.println("0"); // ToDo: But prameters here instead of the url. 
  
        sim900.confirmAtCommand("OK",5000);
  
        GSM.println("AT+HTTPACTION=1"); //POST the data
        sim900.confirmAtCommand("ACTION:",5000);
   Serial.println("The Data have been sent");  
        delay(3000); 
           Serial.println("About to shut the gprs");  
        GSM.println("AT+HTTPTERM"); //terminate http
        sim900.confirmAtCommand("OK",5000);
  
        GSM.println("AT+SAPBR=0,1");// Disconnect GPRS
        sim900.confirmAtCommand("OK",5000);
        sim900.confirmAtCommand("DEACT",5000);
           Serial.println("all done!!!!!");  
}
