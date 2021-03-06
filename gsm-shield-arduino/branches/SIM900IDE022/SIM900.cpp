/*
This is a Beta version.
last modified 08/11/2011.

This library is based on one developed by Arduino Labs
and it is modified to preserve the compability
with the Arduino's product.

The library is modified to use the GSM Shield,
developed by www.open-electronics.org
(http://www.open-electronics.org/arduino-gsm-shield/)
and based on SIM900 chip,
with the same commands of Arduino Shield,
based on QuectelM10 chip.
*/

#include "SIM900.h"  
#include "Streaming.h"

#define _GSM_CONNECTION_TOUT_ 5
#define _TCP_CONNECTION_TOUT_ 20
#define _GSM_DATA_TOUT_ 10

#define RESETPIN 7

QuectelM10 gsm;

QuectelM10::QuectelM10(){};

QuectelM10::~QuectelM10(){};
  
int QuectelM10::restart(char* pin)
{
  pinMode(RESETPIN, OUTPUT);
  digitalWrite(RESETPIN, HIGH);
  delay(10000);
  digitalWrite(RESETPIN, LOW);
  delay(1000);

  return configandwait(pin);
}

int QuectelM10::start(char* pin)
{

  _tf.setTimeout(_TCP_CONNECTION_TOUT_);

  //_cell.flush();
// Just for old style software restart();  
//  _cell << "AT+CFUN=1" <<  _BYTE(cr) << endl; //Comprobar
//   if (!_tf.find("OK")) 
//   {
//     setStatus(IDLE);
//     return 0;
//   }

  pinMode(RESETPIN, OUTPUT);
  digitalWrite(RESETPIN, HIGH);
  delay(10000);
  digitalWrite(RESETPIN, LOW);
  delay(1000);

  return configandwait(pin);
}

int QuectelM10::configandwait(char* pin)
{
  int connCode;
  _tf.setTimeout(_GSM_CONNECTION_TOUT_);

  if(pin) setPIN(pin); //syv

  // Try 10 times to register in the network. Note this can take some time!
  for(int i=0; i<10; i++)
  {  	
    //Ask for register status to GPRS network.
    _cell << "AT+CGREG?" <<  _BYTE(cr) << endl; 

    //Se espera la unsolicited response de registered to network.
    while (_tf.find("+CGREG: 0,"))  // CHANGE!!!!
	{
		connCode=_tf.getValue();
		if((connCode==1)||(connCode==5))
		{
		  setStatus(READY);
		  
		_cell << "AT+CMGF=1" <<  _BYTE(cr) << endl; //SMS text mode.
		delay(200);
		  // Buah, we should take this to readCall()
		_cell << "AT+CLIP=1" <<  _BYTE(cr) << endl; //SMS text mode.
		delay(200);
		//_cell << "AT+QIDEACT" <<  _BYTE(cr) << endl; //To make sure not pending connection.
		//delay(1000);
	  
		  return 1;
		}
	}
  }
  return 0;
}

int QuectelM10::shutdown()
{
  pinMode(RESETPIN, OUTPUT);
  digitalWrite(RESETPIN, HIGH);
  delay(800);
  digitalWrite(RESETPIN, LOW);
  delay(1000);

  _tf.setTimeout(_TCP_CONNECTION_TOUT_);
  //_cell.flush();
  _cell << "AT+CFUN=4" <<  _BYTE(cr) << endl; //Comprobar
   if (_tf.find("OK")) 
   {
     setStatus(IDLE);
     return 1;
   }
   // After shutdown the modem may accept commands giving no answer.
   // We'll play safe
   delay(1000);
   return 0;
}     
  
int QuectelM10::sendSMS(const char* to, const char* msg)
{

  //Status = READY or ATTACHED.
  /*
  if((getStatus() != READY)&&(getStatus() != ATTACHED))
    return 0;
  */    

  _tf.setTimeout(_GSM_DATA_TOUT_);	//Timeout for expecting modem responses.

  //_cell.flush();

  //AT command to send a SMS. Destination telephone number 
  _cell << "AT+CMGS=\"" << to << "\"" <<  _BYTE(cr) << endl; // Establecemos el destinatario

  //Expect for ">" character.
  
  switch(WaitResp(5000, 50, ">")){
	case RX_TMOUT_ERR: 
		return 0;
	break;
	case RX_FINISHED_STR_NOT_RECV: 
		return 0; 
	break;
  }

  //SMS text.
  _cell << msg << _BYTE(ctrlz) << _BYTE(cr) << "\"\r"; 
   if (!_tf.find("OK")) 
	return 0;
  return 1;
}

int QuectelM10::attachGPRS(char* domain, char* dom1, char* dom2)
{

   delay(5000);
   
  _tf.setTimeout(_GSM_DATA_TOUT_);	//Timeout for expecting modem responses.
  _cell << "AT+CIFSR\r";
  if(WaitResp(5000, 50, "ERROR")!=RX_FINISHED_STR_RECV){
	_cell << "AT+CIPCLOSE\r";
	delay(2000);
	_cell << "AT+CIPSERVER=0\r";
	return 1;
  }
  else{
//  _cell.flush();

  _cell << "AT+CSTT=\"";
  _cell << domain;
  _cell << "\",\"";
  _cell << dom1;
  _cell << "\",\"";
  _cell << dom2;
  _cell << "\"\r";
  
  switch(WaitResp(5000, 50, "OK")){
	case RX_TMOUT_ERR: 
		return 0;
	break;
	case RX_FINISHED_STR_NOT_RECV: 
		return 0; 
	break;
  }
  delay(1000);

  _cell << "AT+CIICR\r";
  switch(WaitResp(5000, 50, "OK")){
	case RX_TMOUT_ERR: 
		return 0; 
	break;
	case RX_FINISHED_STR_NOT_RECV: 
		return 0; 
	break;
  }
  delay(1000);

/*

  //Expect "OK". 
  if(_tf.find("OK"))
  {
    setStatus(ATTACHED); 
    delay(1000);
    return 1;
  }
  else
  {
    //setStatus(ATTACHED); 
    //delay(1000);
    //return 1;

    // In this case we dont know the modem mental position
    setStatus(ERROR);
    return 0;   
  }*/
 _cell << "AT+CIFSR\r";
 if(WaitResp(5000, 50, "ERROR")==RX_FINISHED_STR_RECV)
	return 0;
 setStatus(ATTACHED);
 delay(5000);
 return 1;
 }
}

int QuectelM10::dettachGPRS()
{
  if (getStatus()==IDLE) return 0;
   
  _tf.setTimeout(_GSM_CONNECTION_TOUT_);

  //_cell.flush();

  //GPRS dettachment.
  _cell << "AT+CGATT=0" <<  _BYTE(cr) << endl;
  
  if(!_tf.find("OK")) 
  {
    setStatus(ERROR);
    return 0;
  }
  delay(500);
  
  // Commented in initial trial code!!
  //Stop IP stack.
  //_cell << "AT+WIPCFG=0" <<  _BYTE(cr) << endl;
  //	if(!_tf.find("OK")) return 0;
  //Close GPRS bearer.
  //_cell << "AT+WIPBR=0,6" <<  _BYTE(cr) << endl;

  setStatus(READY);
  return 1;
}

int QuectelM10::connectTCP(const char* server, int port)
{
  _tf.setTimeout(_TCP_CONNECTION_TOUT_);

  //Status = ATTACHED.
  //if (getStatus()!=ATTACHED)
    //return 0;

  //_cell.flush();
  
  //Visit the remote TCP server.
  _cell << "AT+CIPSTART=\"TCP\",\"" << server << "\"," << port <<  "\r";
  
  switch(WaitResp(1000, 200, "OK")){
	case RX_TMOUT_ERR: 
		return 0;
	break;
	case RX_FINISHED_STR_NOT_RECV: 
		return 0; 
	break;
  }

  switch(WaitResp(15000, 200, "CONNECT")){
	case RX_TMOUT_ERR: 
		return 0;
	break;
	case RX_FINISHED_STR_NOT_RECV: 
		return 0; 
	break;
  }


  _cell << "AT+CIPSEND\r";
  switch(WaitResp(5000, 200, ">")){
	case RX_TMOUT_ERR: 
		return 0;
	break;
	case RX_FINISHED_STR_NOT_RECV: 
		return 0; 
	break;
  }

  Serial.println("CONNESSO");
  delay(2000);
  return 1;
}

int QuectelM10::disconnectTCP()
{
  //Status = TCPCONNECTEDCLIENT or TCPCONNECTEDSERVER.
  /*
  if ((getStatus()!=TCPCONNECTEDCLIENT)&&(getStatus()!=TCPCONNECTEDSERVER))
     return 0;
  */
  _tf.setTimeout(_GSM_CONNECTION_TOUT_);


  //_cell.flush();

  //Switch to AT mode.
  //_cell << "+++" << endl;
  
  //delay(200);
  
  //Close TCP client and deact.
  _cell << "AT+CIPCLOSE\r";

  //If remote server close connection AT+QICLOSE generate ERROR
  /*if(_tf.find("OK"))
  {
    if(getStatus()==TCPCONNECTEDCLIENT)
      setStatus(ATTACHED);
    else
      setStatus(TCPSERVERWAIT);
    return 1;
  }
  setStatus(ERROR);
  
  return 0;    */
  if(getStatus()==TCPCONNECTEDCLIENT)
      	setStatus(ATTACHED);
   elsehttp://www.facebook.com/photo.php?fbid=2486533357849&set=a.2316317142550.2138694.1088100793&type=1&ref=nf
        setStatus(TCPSERVERWAIT);   
    return 1;
}

int QuectelM10::connectTCPServer(int port)
{
/*
  if (getStatus()!=ATTACHED)
     return 0;
*/
  _tf.setTimeout(_GSM_CONNECTION_TOUT_);

  //_cell.flush();

  // Set port
  _cell << "AT+CIPSERVER=1," << port << "\r";
/*
  switch(WaitResp(5000, 50, "OK")){
	case RX_TMOUT_ERR: 
		return 0;
	break;
	case RX_FINISHED_STR_NOT_RECV: 
		return 0; 
	break;
  }

  switch(WaitResp(5000, 50, "SERVER")){ //Try SERVER OK
	case RX_TMOUT_ERR: 
		return 0;
	break;
	case RX_FINISHED_STR_NOT_RECV: 
		return 0; 
	break;
  }
*/
  //delay(200);  

  return 1;

}

boolean QuectelM10::connectedClient()
{
  /*
  if (getStatus()!=TCPSERVERWAIT)
     return 0;
  */
   _cell << "AT+CIPSTATUS" << "\r";
  // Alternative: AT+QISTAT, although it may be necessary to call an AT 
  // command every second,which is not wise
  /*
  switch(WaitResp(1000, 200, "OK")){
	case RX_TMOUT_ERR: 
		return 0;
	break;
	case RX_FINISHED_STR_NOT_RECV: 
		return 0; 
	break;
  }*/
  _tf.setTimeout(1);
  if(_tf.find("CONNECT OK")) 
  {
    setStatus(TCPCONNECTEDSERVER);
    return true;
  }
  else
    return false;
 }

int QuectelM10::write(const uint8_t* buffer, size_t sz)
{
/*
   if((getStatus() != TCPCONNECTEDSERVER)&&(getStatus() != TCPCONNECTEDCLIENT))
    return 0;
    
   if(sz>1460)
     return 0;
*/
  _tf.setTimeout(_GSM_DATA_TOUT_);

//  _cell.flush();
    
  for(int i=0;i<sz;i++)
    _cell << _BYTE(buffer[i]);
  
  //Not response for a write.
  /*if(_tf.find("OK"))
    return sz;
  else
    return 0;*/
    
  return sz;  
}


int QuectelM10::read(char* result, int resultlength)
{
  // Or maybe do it with AT+QIRD

  int charget;
  _tf.setTimeout(3);
  // Not well. This way we read whatever comes in one second. If a CLOSED 
  // comes, we have spent a lot of time
    //charget=_tf.getString("",'\0',result, resultlength);
    charget=_tf.getString("","",result, resultlength);
  /*if(strtok(result, "CLOSED")) // whatever chain the Q10 returns...
  {
    // TODO: use strtok to delete from the chain everything from CLOSED
    if(getStatus()==TCPCONNECTEDCLIENT)
      setStatus(ATTACHED);
    else
      setStatus(TCPSERVERWAIT);
  }  */
  
  return charget;
}

 int QuectelM10::readCellData(int &mcc, int &mnc, long &lac, long &cellid)
{
  if (getStatus()==IDLE)
    return 0;
    
   _tf.setTimeout(_GSM_DATA_TOUT_);
   //_cell.flush();
  _cell << "AT+QENG=1,0" << endl; 
  _cell << "AT+QENG?" << endl; 
  if(!_tf.find("+QENG:"))
    return 0;

  mcc=_tf.getValue(); // The first one is 0
  mcc=_tf.getValue();
  mnc=_tf.getValue();
  lac=_tf.getValue();
  cellid=_tf.getValue();
  _tf.find("OK");
  _cell << "AT+QENG=1,0" << endl; 
  _tf.find("OK");
  return 1;
}


boolean QuectelM10::readSMS(char* msg, int msglength, char* number, int nlength)
{
  long index;
  /*
  if (getStatus()==IDLE)
    return false;
  */
  _tf.setTimeout(_GSM_DATA_TOUT_);
  //_cell.flush();
  _cell << "AT+CMGL=\"REC UNREAD\",1" << endl;
  if(_tf.find("+CMGL: "))
  {
    index=_tf.getValue();
    _tf.getString("\"+", "\"", number, nlength);
    _tf.getString("\n", "\nOK", msg, msglength);
    _cell << "AT+CMGD=" << index << endl;
    _tf.find("OK");
    return true;
  };
  return false;
};


boolean QuectelM10::readCall(char* number, int nlength)
{
  int index;

  if (getStatus()==IDLE)
    return false;
  
  _tf.setTimeout(_GSM_DATA_TOUT_);

  if(_tf.find("+CLIP: \""))
  {
    _tf.getString("", "\"", number, nlength);
    _cell << "ATH" << endl;
    delay(1000);
    //_cell.flush();
    return true;
  };
  return false;
};

boolean QuectelM10::call(char* number, unsigned int milliseconds)
{ 
  if (getStatus()==IDLE)
    return false;
  
  _tf.setTimeout(_GSM_DATA_TOUT_);

  _cell << "ATD" << number << ";" << endl;
  delay(milliseconds);
  _cell << "ATH" << endl;

  return true;
 
}

int QuectelM10::setPIN(char *pin)
{
  //Status = READY or ATTACHED.
  if((getStatus() != IDLE))
    return 2;
      
  _tf.setTimeout(_GSM_DATA_TOUT_);	//Timeout for expecting modem responses.

  //_cell.flush();

  //AT command to set PIN.
  _cell << "AT+CPIN=" << pin <<  _BYTE(cr) << endl; // Establecemos el pin

  //Expect "OK".
  if(!_tf.find("OK"))
    return 0;
  else  
    return 1;
}

int QuectelM10::write(uint8_t c)
{
  if ((getStatus() == TCPCONNECTEDCLIENT) ||(getStatus() == TCPCONNECTEDSERVER) )
    return write(&c, 1);
  else
    return 0;
}

int QuectelM10::write(const char* str)
{
  if ((getStatus() == TCPCONNECTEDCLIENT) ||(getStatus() == TCPCONNECTEDSERVER) )
      return write((const uint8_t*)str, strlen(str));
  else
      return 0;
}

int QuectelM10::changeNSIPmode(char mode) ///SYVV
{
    _tf.setTimeout(_TCP_CONNECTION_TOUT_);
    
    //if (getStatus()!=ATTACHED)
    //    return 0;

    //_cell.flush();

    _cell << "AT+QIDNSIP=" << mode <<  _BYTE(cr) << endl;

    if(!_tf.find("OK")) return 0;
    
    return 1;
}

int QuectelM10::getCCI(char *cci)
{
  //Status must be READY
  if((getStatus() != READY))
    return 2;
      
  _tf.setTimeout(_GSM_DATA_TOUT_);	//Timeout for expecting modem responses.

  //_cell.flush();

  //AT command to get CCID.
  _cell << "AT+QCCID" << _BYTE(cr) << endl; // Establecemos el pin
  
  //Read response from modem
  _tf.getString("AT+QCCID\r\r\r\n","\r\n",cci, 21);
  
  //Expect "OK".
  if(!_tf.find("OK"))
    return 0;
  else  
    return 1;
}
  
int QuectelM10::getIMEI(char *imei)
{
      
  _tf.setTimeout(_GSM_DATA_TOUT_);	//Timeout for expecting modem responses.

  //_cell.flush();

  //AT command to get IMEI.
  _cell << "AT+GSN" << _BYTE(cr) << endl; 
  
  //Read response from modem
  _tf.getString("AT+GSN\r\r\r\n","\r\n",imei, 15);
  
  //Expect "OK".
  if(!_tf.find("OK"))
    return 0;
  else  
    return 1;
}

uint8_t QuectelM10::read()
{
  return _cell.read();
}

void QuectelM10::SimpleRead()
{
	char datain;
	if(_cell.available()>0){
		datain=_cell.read();
		if(datain>0){
			Serial.print(datain, BYTE);
		}
	}
}

void QuectelM10::SimpleWrite(char *comm)
{
	_cell.println(comm);
}

void QuectelM10::SimpleWriteInt(int comm)
{
	_cell.println(comm);
}

void QuectelM10::SimpleWriteWOln(char *comm)
{
	_cell.print(comm);
}

void QuectelM10::WhileSimpleRead()
{
	char datain;
	while(_cell.available()>0){
		datain=_cell.read();
		if(datain>0){
			Serial.print(datain, BYTE);
		}
	}
}
















//---------------------------------------------
/**********************************************************
Turns on/off the speaker

off_on: 0 - off
        1 - on
**********************************************************/
void GSM::SetSpeaker(byte off_on)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  if (off_on) {
    //SendATCmdWaitResp("AT#GPIO=5,1,2", 500, 50, "#GPIO:", 1);
  }
  else {
    //SendATCmdWaitResp("AT#GPIO=5,0,2", 500, 50, "#GPIO:", 1);
  }
  SetCommLineStatus(CLS_FREE);
}


byte GSM::IsRegistered(void)
{
  return (module_status & STATUS_REGISTERED);
}

byte GSM::IsInitialized(void)
{
  return (module_status & STATUS_INITIALIZED);
}


/**********************************************************
Method checks if the GSM module is registered in the GSM net
- this method communicates directly with the GSM module
  in contrast to the method IsRegistered() which reads the
  flag from the module_status (this flag is set inside this method)

- must be called regularly - from 1sec. to cca. 10 sec.

return values: 
      REG_NOT_REGISTERED  - not registered
      REG_REGISTERED      - GSM module is registered
      REG_NO_RESPONSE     - GSM doesn't response
      REG_COMM_LINE_BUSY  - comm line between GSM module and Arduino is not free
                            for communication
**********************************************************/
byte GSM::CheckRegistration(void)
{
  byte status;
  byte ret_val = REG_NOT_REGISTERED;

  if (CLS_FREE != GetCommLineStatus()) return (REG_COMM_LINE_BUSY);
  SetCommLineStatus(CLS_ATCMD);
  _cell.println("AT+CREG?");
  // 5 sec. for initial comm tmout
  // 50 msec. for inter character timeout
  status = WaitResp(5000, 50); 

  if (status == RX_FINISHED) {
    // something was received but what was received?
    // ---------------------------------------------
    if(IsStringReceived("+CREG: 0,1") 
      || IsStringReceived("+CREG: 0,5")) {
      // it means module is registered
      // ----------------------------
      module_status |= STATUS_REGISTERED;
    
    
      // in case GSM module is registered first time after reset
      // sets flag STATUS_INITIALIZED
      // it is used for sending some init commands which 
      // must be sent only after registration
      // --------------------------------------------
      if (!IsInitialized()) {
        module_status |= STATUS_INITIALIZED;
        SetCommLineStatus(CLS_FREE);
        InitParam(PARAM_SET_1);
      }
      ret_val = REG_REGISTERED;      
    }
    else {
      // NOT registered
      // --------------
      module_status &= ~STATUS_REGISTERED;
      ret_val = REG_NOT_REGISTERED;
    }
  }
  else {
    // nothing was received
    // --------------------
    ret_val = REG_NO_RESPONSE;
  }
  SetCommLineStatus(CLS_FREE);
 

  return (ret_val);
}


/**********************************************************
Method sets speaker volume

speaker_volume: volume in range 0..14

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module did not answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0..14 current speaker volume 
**********************************************************/
/*
char GSM::SetSpeakerVolume(byte speaker_volume)
{
  
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // remember set value as last value
  if (speaker_volume > 14) speaker_volume = 14;
  // select speaker volume (0 to 14)
  // AT+CLVL=X<CR>   X<0..14>
  _cell.print("AT+CLVL=");
  _cell.print((int)speaker_volume);    
  _cell.print("\r"); // send <CR>
  // 10 sec. for initial comm tmout
  // 50 msec. for inter character timeout
  if (RX_TMOUT_ERR == WaitResp(10000, 50)) {
    ret_val = -2; // ERROR
  }
  else {
    if(IsStringReceived("OK")) {
      last_speaker_volume = speaker_volume;
      ret_val = last_speaker_volume; // OK
    }
    else ret_val = -3; // ERROR
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}
*/
/**********************************************************
Method increases speaker volume

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module did not answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0..14 current speaker volume 
**********************************************************/
/*
char GSM::IncSpeakerVolume(void)
{
  char ret_val;
  byte current_speaker_value;

  current_speaker_value = last_speaker_volume;
  if (current_speaker_value < 14) {
    current_speaker_value++;
    ret_val = SetSpeakerVolume(current_speaker_value);
  }
  else ret_val = 14;

  return (ret_val);
}
*/
/**********************************************************
Method decreases speaker volume

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module did not answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0..14 current speaker volume 
**********************************************************/
/*
char GSM::DecSpeakerVolume(void)
{
  char ret_val;
  byte current_speaker_value;

  current_speaker_value = last_speaker_volume;
  if (current_speaker_value > 0) {
    current_speaker_value--;
    ret_val = SetSpeakerVolume(current_speaker_value);
  }
  else ret_val = 0;

  return (ret_val);
}
*/

/**********************************************************
Method sends DTMF signal
This function only works when call is in progress

dtmf_tone: tone to send 0..15

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0.. tone
**********************************************************/
/*
char GSM::SendDTMFSignal(byte dtmf_tone)
{
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // e.g. AT+VTS=5<CR>
  _cell.print("AT+VTS=");
  _cell.print((int)dtmf_tone);    
  _cell.print("\r");
  // 1 sec. for initial comm tmout
  // 50 msec. for inter character timeout
  if (RX_TMOUT_ERR == WaitResp(1000, 50)) {
    ret_val = -2; // ERROR
  }
  else {
    if(IsStringReceived("OK")) {
      ret_val = dtmf_tone; // OK
    }
    else ret_val = -3; // ERROR
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}
*/

/**********************************************************
Method returns state of user button


return: 0 - not pushed = released
        1 - pushed
**********************************************************/
byte GSM::IsUserButtonPushed(void)
{
  byte ret_val = 0;
  if (CLS_FREE != GetCommLineStatus()) return(0);
  SetCommLineStatus(CLS_ATCMD);
  //if (AT_RESP_OK == SendATCmdWaitResp("AT#GPIO=9,2", 500, 50, "#GPIO: 0,0", 1)) {
    // user button is pushed
  //  ret_val = 1;
  //}
  //else ret_val = 0;
  //SetCommLineStatus(CLS_FREE);
  //return (ret_val);
}



/**********************************************************
Method reads phone number string from specified SIM position

position:     SMS position <1..20>

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0
        phone_number is empty string

        OK ret val:
        -----------
        0 - there is no phone number on the position
        1 - phone number was found
        phone_number is filled by the phone number string finished by 0x00
                     so it is necessary to define string with at least
                     15 bytes(including also 0x00 termination character)

an example of usage:
        GSM gsm;
        char phone_num[20]; // array for the phone number string

        if (1 == gsm.GetPhoneNumber(1, phone_num)) {
          // valid phone number on SIM pos. #1 
          // phone number string is copied to the phone_num array
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone number: ", 0);
            gsm.DebugPrint(phone_num, 1);
          #endif
        }
        else {
          // there is not valid phone number on the SIM pos.#1
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG there is no phone number", 1);
          #endif
        }
**********************************************************/


char GSM::GetPhoneNumber(byte position, char *phone_number)
{
  char ret_val = -1;

  char *p_char; 
  char *p_char1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // not found yet
  phone_number[0] = 0; // phone number not found yet => empty string
  
  //send "AT+CPBR=XY" - where XY = position
  _cell.print("AT+CPBR=");
  _cell.print((int)position);  
  _cell.print("\r");

  // 5000 msec. for initial comm tmout
  // 50 msec. for inter character timeout
  switch (WaitResp(5000, 50, "+CPBR")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      ret_val = -2;
      break;

    case RX_FINISHED_STR_RECV:
      // response in case valid phone number stored:
      // <CR><LF>+CPBR: <index>,<number>,<type>,<text><CR><LF>
      // <CR><LF>OK<CR><LF>

      // response in case there is not phone number:
      // <CR><LF>OK<CR><LF>
      p_char = strchr((char *)(comm_buf),'"');
      if (p_char != NULL) {
        p_char++;       // we are on the first phone number character
        // find out '"' as finish character of phone number string
        p_char1 = strchr((char *)(p_char),'"');
        if (p_char1 != NULL) {
          *p_char1 = 0; // end of string
        }
        // extract phone number string
        strcpy(phone_number, (char *)(p_char));
        // output value = we have found out phone number string
        ret_val = 1;
      }
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // only OK or ERROR => no phone number
      ret_val = 0; 
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method writes phone number string to the specified SIM position

position:     SMS position <1..20>
phone_number: phone number string for the writing

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - phone number was not written
        1 - phone number was written
**********************************************************/
char GSM::WritePhoneNumber(byte position, char *phone_number)
{
  char ret_val = -1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // phone number was not written yet
  
  //send: AT+CPBW=XY,"00420123456789"
  // where XY = position,
  //       "00420123456789" = phone number string
  _cell.print("AT+CPBW=");
  _cell.print((int)position);  
  _cell.print(",\"");
  _cell.print(phone_number);
  _cell.print("\"\r");

  // 5000 msec. for initial comm tmout
  // 50 msec. for inter character timeout
  switch (WaitResp(5000, 50, "OK")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      break;

    case RX_FINISHED_STR_RECV:
      // response is OK = has been written
      ret_val = 1;
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // other response: e.g. ERROR
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}


/**********************************************************
Method del phone number from the specified SIM position

position:     SMS position <1..20>

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - phone number was not deleted
        1 - phone number was deleted
**********************************************************/
char GSM::DelPhoneNumber(byte position)
{
  char ret_val = -1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // phone number was not written yet
  
  //send: AT+CPBW=XY
  // where XY = position
  _cell.print("AT+CPBW=");
  _cell.print((int)position);  
  _cell.print("\r");

  // 5000 msec. for initial comm tmout
  // 50 msec. for inter character timeout
  switch (WaitResp(5000, 50, "OK")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      break;

    case RX_FINISHED_STR_RECV:
      // response is OK = has been written
      ret_val = 1;
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // other response: e.g. ERROR
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}





/**********************************************************
Function compares specified phone number string 
with phone number stored at the specified SIM position

position:       SMS position <1..20>
phone_number:   phone number string which should be compare

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - phone numbers are different
        1 - phone numbers are the same


an example of usage:
        if (1 == gsm.ComparePhoneNumber(1, "123456789")) {
          // the phone num. "123456789" is stored on the SIM pos. #1
          // phone number string is copied to the phone_num array
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone numbers are the same", 1);
          #endif
        }
        else {
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone numbers are different", 1);
          #endif
        }
**********************************************************/
char GSM::ComparePhoneNumber(byte position, char *phone_number)
{
  char ret_val = -1;
  char sim_phone_number[20];


  ret_val = 0; // numbers are not the same so far
  if (position == 0) return (-3);
  if (1 == GetPhoneNumber(position, sim_phone_number)) {
    // there is a valid number at the spec. SIM position
    // -------------------------------------------------
    if (0 == strcmp(phone_number, sim_phone_number)) {
      // phone numbers are the same
      // --------------------------
      ret_val = 1;
    }
  }
  return (ret_val);
}

//-----------------------------------------------------
