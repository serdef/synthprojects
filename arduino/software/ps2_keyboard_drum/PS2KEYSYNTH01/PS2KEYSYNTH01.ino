
/*
PS2 KEYBOARD SYNTH/MIDI/DRUM CONTROL
EARLY TEST VERSION BASED ON BICYCLE DRUMMER

26.07.2014
By Serge Defever
Arduino NANO + Dreamblaster S1

Dreamblaster S1 : http://www.serdashop.com/waveblaster
Arduino NANO : http://store.arduino.cc/

*/
#include <Button.h>
#include <TimerOne.h>
#include <PS2Keyboard.h>
const int DataPin = 2;
const int IRQpin =  3;
PS2Keyboard keyboard;


long calctempodelay = 0;
long tempodelay = 500;
long curtimestamp = 0;
long prevtimestamp = 0;
long lasttickstamp =0;
long wheelfactor = 6; // adjust this to modify the pulseperiod to tempo ratio (wheel radius), higher = faster
int  tickflag = 0; // indicate if time tick has passed

const int bicyclesetup = 0;
const int pot1setup = 1;
const int pot2setup = 1;
const int button3Pin = 4;
const int button4Pin = 5;
const int dreamblaster_enable_pin = 6;
int potentiopin = 2;
long potentiovalue = 0;
int buttonState = 0; 
int prev_buttonState = 0; 
int enabledrumming = 0;
int enablebassline = 0;
int enablesynthline = 0;
long seq_poscnt = 0;
byte resonantchannel= 1;
byte channel1_prog = 54;
byte channel2_prog = 81;
byte bassvolume = 0x50;
byte drumvolume = 0x50;
byte synthvolume = 0x50;
byte leadvolume  = 0x70;

#define _BASSDRUM_NOTE 0x24
#define _SNAREDRUM_NOTE 0x26
#define _CLOSEDHIHAT_NOTE 0x2A
#define _PEDALHIHAT_NOTE 0x2C
#define _OPENHIHAT_NOTE 0x2E
#define _CYMBAL_NOTE 0x31
#define _DRUMPRESS_STATUSCODE 0x99
#define _DRUMRELEASE_STATUSCODE 0x89
#define _SYNTHPRESS_STATUSCODE 0x90
#define _SYNTHRELEASE_STATUSCODE 0x80

void midiwrite(int cmd, int pitch, int velocity) {

  Serial.write(cmd);
  Serial.write(pitch);
  Serial.write(velocity);

}


void midisetup_sam2195_nrpn_send(int channel, int control1,int control2, int value)
{
   Serial.write((byte)(0xB0+channel));
   Serial.write(0x63);
   Serial.write((byte)control1);
   Serial.write(0x62);
   Serial.write((byte)control2);
   Serial.write(0x06);
   Serial.write((byte)value);
}

void midisetup_sam2195_sysexcfg_send(int channelprefix,int channel, int control, int value)
{
   Serial.write(0xF0);
   Serial.write(0x41);
   Serial.write(0x00);
   Serial.write(0x42);
   Serial.write(0x12);
   Serial.write(0x40);
   Serial.write((byte)(channelprefix*16+channel));   
   Serial.write((byte)control);
   Serial.write((byte)value);   
   Serial.write(0x00);   
   Serial.write(0xF7);   
   
}

void midisetup_sam2195_gsreset(void)
{
  midisetup_sam2195_sysexcfg_send(0,0,0x7F,0);
}

void midiprogchange(int cmd, int prog) {
  Serial.write(cmd);
  Serial.write(prog);
}


Button button3 = Button(button3Pin,BUTTON_PULLDOWN);
Button button4 = Button(button4Pin,BUTTON_PULLDOWN);


void basssubseq(long relativpos) 
{ 
  switch(relativpos)
    {
    case 0 :
      midiwrite(0x81, 64, 0x00); 
      midiwrite(0x91, 28, bassvolume); 
      break;
    case 4 :
      midiwrite(0x81, 28, 0x00); 
      midiwrite(0x91, 40, bassvolume);     
      break;
    case 8 :
      midiwrite(0x81, 40, 0x00); 
      midiwrite(0x91, 52, bassvolume);     
      break;
    case 12:
      midiwrite(0x81, 52, 0x00); 
      midiwrite(0x91, 64, bassvolume);   
      break;        
    default :
      break;
    }    

}

void synthsubseq(long relativpos) 
{ 
  switch(relativpos)
    {
    case 0 :
       midiwrite(0x90, 40, synthvolume); 
      break;
    case 7:
      midiwrite(0x80, 40, synthvolume);    
      break;
    case 8 :
      midiwrite(0x90, 52, synthvolume);     
      break;
    case 15:
      midiwrite(0x80, 52, synthvolume);
      break;        
    default :
      break;
    }    

}
void drumseq(long seqpos) 
{ 
  char relativpos;
  char seq_poscnt_mod2;
  relativpos =seqpos%16;
  seq_poscnt_mod2 = seqpos%2;
  if(enabledrumming)
  {
    if((seq_poscnt_mod2 == 0) && (relativpos < 14))
    {
      midiwrite(0x99, _PEDALHIHAT_NOTE, 0x65);     // fast hi hat midi note
    }

    switch(relativpos)
    {
    case 0 :
      midiwrite(0x99, _BASSDRUM_NOTE, drumvolume);     
      break;
    case 4:
      midiwrite(0x99, _SNAREDRUM_NOTE, drumvolume);    
      break;
    case 6 :
      midiwrite(0x99, _BASSDRUM_NOTE, drumvolume);     
      break;
    case 8 :
      midiwrite(0x99, _BASSDRUM_NOTE, drumvolume);     
      break;
    case 12:
      midiwrite(0x99, _SNAREDRUM_NOTE, drumvolume);    
      break;
    case 14:
      midiwrite(0x99,  _OPENHIHAT_NOTE, drumvolume);   

      break;        
    default :
      break;
    }    
    if(enablebassline)
    {
      basssubseq(relativpos);
    }
   if(enablesynthline)
   {
      synthsubseq(relativpos);     
   }
  } 
 };


void timercallback()
{
  lasttickstamp = millis();
  Timer1.setPeriod(tempodelay * 1000);     
  tickflag = 1;
}

void setupvoices(void)
{
    midiprogchange(0xC0,channel1_prog);
    midiprogchange(0xC1,channel2_prog);
}
void SomeButtonPressHandler(Button& butt)
{  
 if(enabledrumming)
  {
    if(butt == button3)
    {
      enablebassline = enablebassline?0:1;
    };

  }
  else
  {
    
    if(butt == button3)
    {  
      midiwrite(0x99, _SNAREDRUM_NOTE, 0x65);   
    }
    if(butt == button4)
    {  
      midiwrite(0x99, _BASSDRUM_NOTE, 0x65);   
    }
  }
}
void stopallnotes(byte selection = 0)
{  

if (selection == 0 || selection ==2)
{
      midiwrite(0x90, 52, 0x00); // switch off possible synth notes
      midiwrite(0x90, 40, 0x00); // switch off possible synth notes     
}
if (selection == 0 || selection ==1)
{
      midiwrite(0x91, 28, 0x00); // switch off possible bass notes
      midiwrite(0x91, 40, 0x00); // switch off possible bass notes           
      midiwrite(0x91, 52, 0x00); // switch off possible bass notes
      midiwrite(0x91, 64, 0x00); // switch off possible bass notes
}
}



void SomeButtonReleaseHandler(Button& butt)
{
  if(enabledrumming)
  {
    //..
  } 
  else
  {
  
  }
  
  
}

void PotControl(int potnr, int ctrlVal)
{
  if(potnr == 1)
  {
     //Serial.println(ctrlVal);
      midisetup_sam2195_nrpn_send(resonantchannel,0x01,0x20,(byte)ctrlVal); 
      //midiwrite(0xB0, 0x01,(byte) ctrlVal);  // modulation wheel
  }
   if(potnr == 2)
  {
     midisetup_sam2195_nrpn_send(resonantchannel,0x01,0x21,(byte)ctrlVal);  // set resonance for channel resonantchannel
  }
}


void ADC_handle(void)
{
  static int prevsensorVal1 = -1000;  
  static int prevsensorVal2 = -1000;
  if(pot1setup)
  {
    int sensorValue = analogRead(A4);
    int ctrlValue;
    if(abs(sensorValue-prevsensorVal1)>3)  // filter noise from adc input
    {
        prevsensorVal1 = sensorValue;
        ctrlValue = sensorValue/8;
        PotControl(1,ctrlValue);
    }
  }
  if(pot2setup)
  {
    int sensorValue = analogRead(A2);
    int ctrlValue;
    if(abs(sensorValue-prevsensorVal2)>3)  // filter noise from adc input
    {
        prevsensorVal2 = sensorValue;
        ctrlValue = sensorValue/8;
        PotControl(2,ctrlValue);
    }
  }
}


void SomeButtonHoldHandler(Button& butt)
{
  if(butt==button4)
  {
        stopdrum(); 
  }
}

void KeyboardHandler(char key,bool iskeyrelease)
{
  static bool space_pressed = false;  
  static bool enter_pressed = false;  
  static bool zero_pressed = false;  
  static bool c_pressed = false;  
  static bool v_pressed = false;  
  static bool b_pressed = false;  
  byte drumstatuscode;
  byte synthstatuscode;
  byte volumecode;

  drumstatuscode = iskeyrelease ? _DRUMRELEASE_STATUSCODE : _DRUMPRESS_STATUSCODE;
  synthstatuscode = iskeyrelease ? _SYNTHRELEASE_STATUSCODE : _SYNTHPRESS_STATUSCODE;
  volumecode = iskeyrelease ? 0x00 : leadvolume;
  if((key==PS2_PAGEUP)|| (key==PS2_PAGEDOWN) || (key=='+')|| (key=='-'))
  {
    if(!iskeyrelease) 
     {
        if(key=='+') 
        {
          channel2_prog++;
          if(channel2_prog > 0x7F)
          {
            channel2_prog = 0x00; 
          }       
        } 
       if(key=='-') 
        {
          channel2_prog--;
          if(channel2_prog > 0x7F)
          {
            channel2_prog = 0x7F; 
          }       
        } 
       if(key==PS2_PAGEUP) 
        {
          channel1_prog++;
          if(channel1_prog > 0x7F)
          {
            channel1_prog = 0x00; 
          }       
        } 
       if(key==PS2_PAGEDOWN)
        {
          channel1_prog--;
          if(channel1_prog > 0x7F)
          {
            channel1_prog = 0x7F; 
          }       
        } 

       setupvoices();
     }
  }
  if(key==' ') 
   {
     if(!space_pressed || iskeyrelease) 
     {     
        midiwrite(drumstatuscode, _BASSDRUM_NOTE, volumecode);      
     }
     space_pressed = !iskeyrelease;
     return;
   }
  if(key==PS2_ENTER) 
   {
     if(!enter_pressed  || iskeyrelease) 
     { 
      midiwrite(drumstatuscode,_CLOSEDHIHAT_NOTE, volumecode); 
     }
     enter_pressed = !iskeyrelease;
     return;
   }
  if(key=='0') 
   {
     if(!zero_pressed || iskeyrelease) 
     { 
      midiwrite(drumstatuscode,  _SNAREDRUM_NOTE, volumecode);      
     }
     zero_pressed = !iskeyrelease;
     return;
   }
   if(key=='c') 
   {
     if(!c_pressed || iskeyrelease) 
     { 
      midiwrite(synthstatuscode,  40, volumecode);      
     }
     c_pressed = !iskeyrelease;
     return;
   }
   if(key=='v') 
   {
     if(!v_pressed || iskeyrelease) 
     { 
      midiwrite(synthstatuscode,  45, volumecode);      
     }
     v_pressed = !iskeyrelease;
     return;
   }
   
   if(key=='b') 
   {
     if(!b_pressed || iskeyrelease) 
     { 
      midiwrite(synthstatuscode,  50, volumecode);      
     }
     b_pressed = !iskeyrelease;
     return;
   }
   
   if(key=='1') 
   {
     if(!iskeyrelease) 
     {        
       buttonState = (buttonState==0) ? 1 : 0;        
     }
     return;
   }
   
  
   if(key=='2') 
   {
     if(!iskeyrelease) 
     { 
        enablebassline = enablebassline?0:1;
        if(!enablebassline) {stopallnotes(1);};
     }
     return;
   }
   
   if(key=='3') 
   {
     if(!iskeyrelease) 
     {        
       enabledrumming = (enabledrumming==0) ? 1 : 0;        
     }
     return;
   }
   
}

void setup() {
  Serial.begin(31250);
  analogReference(DEFAULT); 
  
  pinMode(dreamblaster_enable_pin ,OUTPUT);   // enable the dreamblaster module by pulling high /reset
  digitalWrite(dreamblaster_enable_pin, HIGH);
  delay(300);  // allow 500ms for the dreamblaster to boot
  midisetup_sam2195_gsreset();
  delay(500);  // allow 250 for gs reset
  
  keyboard.begin(DataPin, IRQpin); // ps2 keyboard
  
/*
  midisetup_sam2195_sysexcfg_send(0x02,0x00,0x01,0x50);  // set mod wheel cutoff
  midisetup_sam2195_sysexcfg_send(0x02,0x00,0x04,0x00);  // disable mod wheel pitch variation
  midisetup_sam2195_sysexcfg_send(0x02,0x00,0x05,0x7F);  // set mod wheel tvf depth
*/
  midisetup_sam2195_nrpn_send(resonantchannel,0x01,0x21,(byte)0x7F);  // set resonance for channel resonantchannel
  Timer1.attachInterrupt(timercallback);
  Timer1.initialize(tempodelay*1000);   
  
  button3.pressHandler(SomeButtonPressHandler);
  button3.releaseHandler(SomeButtonReleaseHandler);  
  button4.pressHandler(SomeButtonPressHandler);
  button4.releaseHandler(SomeButtonReleaseHandler);
  button4.holdHandler(SomeButtonHoldHandler,1000);
  setupvoices();
}

void stopdrum(){
  enabledrumming = 0; 
  prevtimestamp = 0;
  curtimestamp = 0;
  seq_poscnt = 0;
  stopallnotes();
}


void loop() {
  long timesincelastbeat;
  long timetonextbeat;  
  
  ADC_handle();
  button3.isPressed(); // trigger handling for button
  button4.isPressed(); // trigger handling for button
  
  if (keyboard.availablerel()) {
      // read the next key
      bool iskeyrelease = false;
      char c = keyboard.readrel(iskeyrelease);      
      KeyboardHandler(c,iskeyrelease);
  }
  
  if(tickflag)
  {
    // sequencing time tick
    drumseq(seq_poscnt);
    seq_poscnt++;
    tickflag = 0;
  }
  
  if(prev_buttonState!=buttonState)
  {
    prev_buttonState = buttonState;
    if(buttonState == 1)
    {
      prevtimestamp =  curtimestamp;
      curtimestamp = millis();
      if(prevtimestamp)
      {           
        calctempodelay = curtimestamp - prevtimestamp;
        if(calctempodelay > 100  && calctempodelay < 3000)  // if tempo is within range, calc tempo and enable drum
        {
          if(enabledrumming == 0)
          {
            seq_poscnt = 0;
            enabledrumming = 1;
          }
          tempodelay  = calctempodelay/wheelfactor;
          timesincelastbeat = curtimestamp - lasttickstamp;
          if(timesincelastbeat > tempodelay)  // is the beat is going faster, tick immediately (within 10ms)
          {
            timetonextbeat = 10; 
          }
          else
          {
            timetonextbeat = tempodelay -timesincelastbeat; // calculate remaining period for next tick
            if (timetonextbeat < 10)
            {
              timetonextbeat = 10;
            }
          }
          Timer1.setPeriod(timetonextbeat * 1000);  // calculate period until next beat            
        }
      }
    }
  } 
  else
  {
    if(enabledrumming)
    {
      if(curtimestamp)
      {
        if( (millis() - curtimestamp  )> 5000)  // automatically stop if no sensor change for longer than 5s
        {
          if(bicyclesetup)
            {
               stopdrum(); 
            }
        }
      }
    }
  }

}








