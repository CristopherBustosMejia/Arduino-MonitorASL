//Librerias
#include <DHT.h>
#include <FirebaseESP32.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_NeoPixel.h>

//Sensor de infrarrojo
#define InfraSensor 18

//Foto resistencia
#define FotoRes 4

//Sensor de temperatura
#define DhtType DHT11
#define DhtPin 15
DHT dht (DhtPin, DhtType);

//Detector de corriente
#define Corriente 14

//Iman puerta
#define Iman 5

//LED PROCESO
#define Led 27

//Luces Led
#define BLEDs 13
#define NumPixeles 75
Adafruit_NeoPixel pixeles(NumPixeles, BLEDs, NEO_GRB + NEO_KHZ800);

//Firebase
#define FIREBASE_HOST "https://aslogicmonitor-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "bl7QoK78VkYJw8KB5b01pbkJtKuPoOH6bwJAidow"

//Variables
float Temp, Hume;
uint32_t TiempoAux = 0, TiempoLuz = 0, TiempoEnergia = 0, TiempoLed = 0, retardo = 2000;
bool BanderaEnergia = true, BanderaRack = true;
//WiFi
const char* ssid = "Redmi8";
const char* pass = "Eder123#";
FirebaseData firebaseData;
String site = "/ASLOGIC/Site1/Rack2/", NumeroRack = "2", NumeroSite = "1";
//Notificacion

HTTPClient http;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(InfraSensor, INPUT);
  pinMode(FotoRes, INPUT);
  pinMode(Corriente, INPUT);
  pinMode(Iman, INPUT_PULLUP);
  pinMode(Led, OUTPUT);
  dht.begin();
  WiFi.begin(ssid, pass);
  Serial.print("Se está conectado a la red WiFi denominada: ");
  Serial.print(ssid);
  Serial.println();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.print("IP address:");
  Serial.println(WiFi.localIP());
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Serial.println("Conectado");
}

void loop()
{
  // put your main code here, to run repeatedly:
  digitalWrite (Led, HIGH);
  if (millis() - TiempoLed >= 500)
  {
    TiempoLed = millis();
    digitalWrite (Led, LOW);
  }
  if (digitalRead(Corriente) == LOW)
  {
    if (millis() - TiempoEnergia >= retardo)
    {
      TiempoEnergia = millis();
      Firebase.setString(firebaseData, site + "Energia", "false");
      if(BanderaEnergia){
        Notificacion("No hay energia");
        BanderaEnergia = false;
      }
    }
  }
  else
  {
    if(!BanderaEnergia){
      Notificacion("Regreso la energia");
      BanderaEnergia = true;
    }
    if (millis() - TiempoEnergia >= retardo)
    {
      TiempoEnergia = millis();
      Firebase.setString(firebaseData, site + "Energia", "true");
    }
  }
  if (millis() - TiempoAux >= 5000)
  {
    TiempoAux = millis();
    RegistrarTH();
    Firebase.setFloat(firebaseData, site + "Temperatura", Temp);
    Firebase.setFloat(firebaseData, site + "Humedad", Hume);
    if(Temp >= 26){
       Notificacion("La temperatura ha sobrepasado el rango de seguridad " + (String)Temp + "°C");
    }
  }

  if (digitalRead(FotoRes) == LOW)
  {
    if (millis() - TiempoLuz >= retardo)
    {
      TiempoLuz = millis();
      Firebase.setString(firebaseData, site + "Luz", "true");
    }
    ControlLuces();
  }
  else
  {
    if (millis() - TiempoLuz >= retardo)
    {
      TiempoLuz = millis();
      Firebase.setString(firebaseData, site + "Luz", "false");
    }
    LucesLed(0, 0, 0, true);
  }

}

void RegistrarTH() {
  Temp = dht.readTemperature();
  Hume = dht.readHumidity();
  if (isnan(Temp) || isnan(Hume)) {
    RegistrarTH();
  }
}

void ControlLuces() {
  if (digitalRead(Iman) == HIGH) {
    Firebase.setString(firebaseData, site + "Puertas", "true");
    if(BanderaRack){
      Notificacion("La puerta fue abierta");
      BanderaRack = false;
    }
    LucesLed(0, 255, 0, true);
  } else {
    if(!BanderaRack){
      Notificacion("Se cerro la puerta");
      BanderaRack = true;
    }
    Firebase.setString(firebaseData, site + "Puertas", "false");
    if (digitalRead(InfraSensor) == LOW) {
      LucesLed(255, 0, 0, true);
    } else {
      LucesLed(0, 0, 255, false);
    }

  }
}
void Notificacion(String Mensaje){
  http.begin("https://fcm.googleapis.com/fcm/send");
      String data = "{";
      data = data + "\"to\":\"/topics/ASLOGIC\",";
      data = data + "\"notification\": {";
      data = data + "\"body\": \"SITE " + NumeroSite + ": " + Mensaje + " en el rack " + NumeroRack + "\",";
      data = data + "\"title\":\"ASLOGIC\"";
      data = data + "} }";
      http.addHeader("Authorization", "key=AAAAKn2RV54:APA91bEHRxts4k3zDkL4bZ51LcQhk2uIK-7GxdjsABOop_R-RSMBVhAsQbqrv1fGAYkdI36O6_z1Ie64V-M89lXpUAn3KVrmHBwHv2Yst4_5SHebxdbXLeKL_twgIOE8Fnzau85dueFT");
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Content-Length", (String)data.length());
      int httpResponseCode = http.POST(data);
}
void LucesLed(int R, int G, int B, bool Sentido) {
  if (Sentido) {
    for (int i = 0; i < NumPixeles; i++) {
      pixeles.setPixelColor(i, pixeles.Color(R, G, B));
      pixeles.show();
    }
  } else {
    for (int i = NumPixeles; i >= 0; i--) {
      pixeles.setPixelColor(i, pixeles.Color(R, G, B));
      pixeles.show();
    }
  }
}
