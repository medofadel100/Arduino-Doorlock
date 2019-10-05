#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

//============================================
//wifi variable
//char ssid[] = "Kyrana Cell - Online";          //  your network SSID (name) 
char ssid[] = "Brisik Boss !!";
char pass[] = "distorsi";   // your network password
int status = WL_IDLE_STATUS;
WiFiClient  client;

//============================================
//000webhost variable
String dat;
const char* host = "iot-pnj.net16.net";
const char* api   = "ZXC321ASD";

//============================================
// RFID Variable
#define SS_PIN 16
#define RST_PIN 10

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key; 

byte nuidPICC[3];
byte A[4];
void bacaRFID();
void regisRFID();
//============================================
//EEPROM Variable
void clearEEP(int minADDR,int maxADDR,int Value);
byte value;
int address = 999;
int minaddr=1000;
int maxaddr=2000;
int maxcard=1;
int ncard;
boolean match=false;

//============================================
//Relay
int q=0;
int door_stat;
#define relay 5

//============================================
//Global Variable
char x;
int i=0;
String in;

void setup() 
{
  //============================================
  //inisialisasi
  Serial.begin(115200);      // Init Serial
  SPI.begin();               // Init SPI bus
  rfid.PCD_Init();           // Init MFRC522 
  EEPROM.begin(maxaddr);     // Define max address
  clearEEP(998,maxaddr,0); // Uncomment to clear EEPROM
  pinMode(relay,OUTPUT);     // Define relay as output
  //===========================================
  // wait for Serial Communication
  while (!Serial){;}
  Serial.println();  
  //============================================
  //init WiFi
  //Waiting for ESP to connect wifi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
  //============================================
  //check EEPROM
  Serial.println(); 
     for ( int j = 0; j < 1+(maxcard*4); j++ )
     {
      value = EEPROM.read(address+j);
      Serial.print(address+j);
      Serial.print("\t");
      Serial.print(value, DEC);
      Serial.println();
      delay(10);
     }
  //============================================
  //clear nuid variable    
  for (byte i = 0; i < 4; i++) 
  {
    //key.keyByte[i] = 0xFF;
    nuidPICC[i]= 0xFF;
  }
  //rfid.PCD_SetAntennaGain(rfid.RxGain_max); // Uncomment to increase detection range
                                              // of RC522 module
  //===========================================
  //Wait for RFID read and register to EEPROM
  while(1)
  {
  regisRFID();
  if(EEPROM.read(999)==maxcard){delay(10);break;}
  }
  //===========================================
  digitalWrite(relay,LOW);// set relay to low
  Serial.println("ready");// print ready to indicate device is ready to use
}

void loop() 
{
  bacaRFID();                        // Read RFID
  in=read_web();                     // Read Web
  q=in.toInt();              // convert value from string to integer
  if(cariRFID(nuidPICC))             // Check if RFID has been registered before
  {
    q=!q;
    Serial.println("kartu dikenali");//card is recognised
  }
  else
  {
    if(nuidPICC[0]!=255 && 
       nuidPICC[1]!=255 && 
       nuidPICC[2]!=255 && 
       nuidPICC[3]!=255)
    {
      Serial.println("kartu tidak dikenal");//card is not recognised
    };
  };
  if(q)
  {
    digitalWrite(relay,HIGH);
  }
  else
  {
    digitalWrite(relay,LOW);
  }
  Serial.println(q);
  //===========================================
  // Clear nuid variable
  for (byte i = 0; i < 6; i++) 
  {
    nuidPICC[i]= 0xFF;
  }
  delay(1000);
  send_web(q);
  delay(1000);
}

void readID( int number ) 
{
  int start = (number * 4 ) + 1000;     // Figure out starting position
  for ( int i = 0; i < 4; i++ ) {     // Loop 4 times to get the 4 Bytes
    A[i] = EEPROM.read(start + i);   // Assign values read from EEPROM to array
  }
}
boolean cariRFID(byte B[])
{
  int count=EEPROM.read(999);
  for ( int i = 0; i <= count; i++ )
  {
    readID(i);
    if(compRFID(B,A))
    {
      return true;
      break;
    }
  }
  return false;
}
boolean compRFID( byte a[], byte b[] )
{
  if ( a[0] != NULL )       // Make sure there is something in the array first
    match = true;       // Assume they match at first
  for ( int k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] )     // IF a != b then set match = false, one fails, all fail
      match = false;
  }
  if ( match ) {      // Check to see if if match is still true
    return true;      // Return true
  }
  else  {
    return false;       // Return false
  }
}
void regisRFID()
{
  while(EEPROM.read(999)<maxcard)//if no card registered in EEPROM
  {
    Serial.println("Silahkan tap kartu yang ingin didaftarkan");
    ncard=EEPROM.read(999);
    Serial.print("kartu ");
    Serial.println(ncard+1);
     //========================================
     //read RFID card, loop if no card detected
     while(1)
     {
      bacaRFID();
      delay(10);
      if(nuidPICC[0]!=255 && nuidPICC[1]!=255 && nuidPICC[2]!=255)
      {
        if(cariRFID(nuidPICC))
        {
          Serial.println("Kartu sudah terdaftar");
          for (byte i = 0; i < 4; i++) 
         {
          nuidPICC[i]= 0xFF;
         }
        }
        else
        {
          break;
        }
      }
     }
     //========================================
     //write card to EEPROM
     int dptr = (ncard*4)+minaddr;
     for ( int j = 0; j < 4; j++ )          // Loop 4 times
     {        
      EEPROM.write(j+dptr, nuidPICC[j] );  // Write scanned PICC's UID to EEPROM, start from address 3
      delay(10);
     }
     for (byte i = 0; i < 4; i++) 
     {
      nuidPICC[i]= 0xFF;
     }
   EEPROM.write(999, ncard+1);
   EEPROM.commit(); 
   delay(10);
  }
  //========================================
  //check the EEPROM value
  for ( int j = 0; j < 1+(maxcard*4); j++ )
  {
    value = EEPROM.read(address+j);
    Serial.print(address+j);
    Serial.print("\t");
    Serial.print(value, DEC);
    Serial.println();
    delay(10);
  }
}
void bacaRFID()//read RFID card
{
    // Look for new cards
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;
    
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Kartu tidak kompatibel, Coba kartu lain"));
    return;
  }
  
    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) 
    {
      nuidPICC[i] = rfid.uid.uidByte[i];
      Serial.print(nuidPICC[i]);
      Serial.print(" ");
    }
    Serial.println();

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  //rfid.PCD_StopCrypto1();
  //return 1;
}
void clearEEP(int minADDR,int maxADDR,int Value)//to erase data from EEPROM
{
    for (int i = minADDR; i < maxADDR; i++)
    {
      EEPROM.write(i, Value);
    }
}

void send_web(int data)
{
   String Buff;
   Buff=String(data,DEC); //convert int to string

   WiFiClient client;
   const int httpPort = 80;
   if (!client.connect(host, httpPort)) 
   {
     Serial.println("connection failed");
     return;
   }
   String url = "/web.php?";
   url += "variable1=";
   url += Buff;
   url += "&api_key=";
   url += api;
   
   Serial.print("Requesting URL: ");
   Serial.print(host);
   Serial.println(url);

   client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
}

String read_web()
{
  String url = "/web.html";
  String line;
  String buff;
  String var="";
  WiFiClient client;
   const int httpPort = 80;
   if (!client.connect(host, httpPort)) 
   {
     Serial.println("connection failed");
     //return;
   }
  
  Serial.print("Requesting URL: ");
  Serial.print(host);
  Serial.println(url);
   
  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  
  unsigned long timeout = millis();
  while (client.available() == 0) 
  {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
    }
  }
  // Read all the lines of the reply from server and print them to Serial
  while(client.available())
  {
    buff = client.readStringUntil('\n');
    if(buff.startsWith("Variable1="))
    {
      //use subtring 10 because "Variable1=" has 10 character
      //                         12345678910
      line=buff.substring(10);
    }
  }
  return line;
}
