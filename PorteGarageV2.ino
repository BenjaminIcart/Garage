#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>



WiFiClient client; 

char* ssid = "YOUR_SSID"; 
char* password = "YOUR_PASSWORD"; 
const char* host = "maker.ifttt.com";
MDNSResponder mdns; 
long port = 3000;
ESP8266WebServer server(port); 
int timer = 30;

const int led = 2;
int relayPin = D8;
int relayPin2 = D7;

String Etat = "jsp";

const int trigPin = 5;
const int echoPin = 4;
const int tempsOuverturePorteEnMs=20000;

#define SOUND_VELOCITY 0.034

long duration;
float distanceCm;
unsigned long chronoDemarragePorte;
unsigned long chronoRafraichirEtat;
bool porteSOuvre;
bool porteSeFerme;

// sécurisation
const char* www_username = "USERNAME_WEBPAGE";
const char* www_password = "PASSWORD_WEBPAGE";

void handle_root() 
{
    autentification();
    
    String page = "<!DOCTYPE html>";

    page += "<html lang='fr'>";

    page += "<head>";
    page += "    <title>Serveur ESP32</title>";
    page += "    <meta http-equiv='refresh' content='60' name='viewport' content='width=device-width, initial-scale=1' charset='UTF-8' />";
    page += "    <link rel='stylesheet' href='https://www.w3schools.com/w3css/4/w3.css'>";
    page += "</head>";

    page += "<body>";
    page += "    <div class='w3-card w3-blue w3-padding-small w3-jumbo w3-center'>";

    page += "        <p>La porte de garage est " + Etat + "</p>";
    page += "    </div>";

    page += "    <div class='w3-bar'>";
    if (!porteSOuvre && !porteSeFerme)
    {
      page += "        <a href='/on?pass="+server.arg("pass")+"' class='w3-bar-item w3-button w3-border w3-jumbo' style='width:50%; height:50%;'>Ouvrir</a>";
      page += "        <a href='/off?pass="+server.arg("pass")+"' class='w3-bar-item w3-button w3-border w3-jumbo' style='width:50%; height:50%;'>Fermer</a>";
    }

    page += "        <a href='/etat' </a>";

    page += "    </div>";

    page += "    <div class='w3-center w3-padding-16'>";
    page += "    </div>";

    page += "</body>";

    page += "</html>";

    server.setContentLength(page.length());
    server.send(200, "text/html", page);
}


void handleEtat()
{
    server.send(200, "text/plain", Etat);
}

void handleOn()
{
    autentification();

    digitalWrite(relayPin, HIGH);
    delay(1000);
    digitalWrite(relayPin, LOW);  
    chronoDemarragePorte = millis();
    porteSOuvre = true;
    server.sendHeader("Location","/?pass="+server.arg("pass"));
    server.send(303);
}

void handleOff()
{
    autentification();
  
    digitalWrite(relayPin2, HIGH);
    delay(1000);
    digitalWrite(relayPin2, LOW);
    chronoDemarragePorte = millis();    
    porteSeFerme = true;   
    server.sendHeader("Location","/?pass="+server.arg("pass"));
    server.send(303);
}


void connect(char *_SSID, char* _PWD) 
{
  Serial.println("");
  Serial.print("Connecting ");
  Serial.print(_SSID);
  Serial.print(" password = ");
  Serial.print( _PWD);

  WiFi.begin(_SSID, _PWD);
  Serial.println("");

  int h = 0;

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");

    if (h++ > 40) 
    { 
      Serial.println();
      Serial.println("Failed to connect");
      return;
    }
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

}


void setup() 
{
  Serial.begin(9600); 
  pinMode(trigPin, OUTPUT); 
  pinMode(echoPin, INPUT); 
  digitalWrite(led, LOW);
  pinMode(relayPin,OUTPUT);
  pinMode(relayPin2,OUTPUT);
  server.on("/", handle_root);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/etat", handleEtat);

  connect(ssid, password); 

  if (mdns.begin("esp8266", WiFi.localIP())) 
  {
    Serial.println("MDNS responder started");
  }
  server.on("/", handle_root);
  server.begin(); // demarrage du serveur
  Serial.println("HTTP server started");
}

 
void loop() 
{
  server.handleClient(); 

  if (millis()-chronoRafraichirEtat>1000)
  {
    chronoRafraichirEtat=millis();
    
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    distanceCm = duration * SOUND_VELOCITY/2;
    Serial.print("Distance (cm): ");
    Serial.println(distanceCm);
  }

  if (porteSOuvre)
  {
    int tempsRestant = tempsOuverturePorteEnMs - (millis()-chronoDemarragePorte);
    if (tempsRestant<=0)
    {
      porteSOuvre=false;
      tempsRestant=0;
    }
    Etat="en train de s'ouvrir ["+String(tempsRestant/1000)+"s]...";
  }
  else
  if (porteSeFerme)
  {
    int tempsRestant = tempsOuverturePorteEnMs - (millis()-chronoDemarragePorte);
    if (tempsRestant<=0)
    {
      porteSeFerme=false;
      tempsRestant=0;
    }
    Etat="en train de se fermer ["+String(tempsRestant/1000)+"s]...";
  }
  else
  {
    if(distanceCm>=200)
    {
      Etat="Ouverte";
    }
    else
    {
      Etat="Ferm&eacute;e";
    }
  }  
}
 
void autentification()
{
    if (server.arg("pass")== www_password) return;
    
    if (!server.authenticate(www_username, www_password)) 
    {
      delay(5000);
      return server.requestAuthentication();
    }
}
