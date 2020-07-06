#include <DHT.h>
#include <ESP8266WiFi.h>
//#include <ThingSpeak.h>


const char *ssid = "nome do WIFI";
const char *pass = "senha do WIFI";
const char* server = "api.thingspeak.com";
unsigned long chID = 1234;
const char * apiKey = "Sua-chave";
String thingtweetAPIKey = "thingtweetAPIKey";
unsigned long tbID = 1234;
const char * tbAPIKey = "TALKBACK KEY";

int modoNoturno = 0;

WiFiClient client;

#define DHTPIN 5
DHT dht(DHTPIN, DHT11);


void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);  pinMode(15, OUTPUT);  delay(10);

  dht.begin();
  wiFiConnection();
  //ThingSpeak.begin(client);
}


/*
          >>>>>>>>>>>>>> LOOP <<<<<<<<<<<<<<<
*/
void loop() {

  wiFiConnection();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  TinhSpeakPOST(t, h);


  piscaLed(15, 2500, 10);
  piscaLed(15, 500, 5);

}
/*
          >>>>>>>>>>>>>> LOOP <<<<<<<<<<<<<<<
*/
void TinhSpeakPOST(float t, float h) {

  String postMessage =  String("field1=") + String(t) +
                        String("&field2=") + String(h) +
                        String("&api_key=") + String(apiKey) +
                        String("&talkback_key=") + String(tbAPIKey);

  // Make a string for any commands in the queue
  String newCommand = String();

  // Make the POST to ThingSpeak
  int x = httpPOST(postMessage, newCommand);
  client.stop();

  if (x == 200) {
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" degrees Celcius, Humidity: ");
    Serial.print(h);
    Serial.println("%. Enviado com sucesso ao Thingspeak.");
    Serial.println("Verificando Comandos...");
    // check for a command returned from TalkBack
    if (newCommand.length() != 0) {
      Serial.print("Ultimos comandos encontrados: ");
      Serial.println(newCommand);
      if (newCommand == "LIGAR_AGUA") {
        Serial.println("Ligando AGUA");
        //Acao não implementada
      }
      if (newCommand == "DESLIGAR_AGUA") {
        Serial.println("Desligando AGUA");
        //Acao não implementada
      }
      if (newCommand == "PISCAR_PINO13") {
        Serial.println("Piscando PINO13");
        piscaLed(13, 125, 10);
        digitalWrite(13, HIGH);
      }
      if (newCommand == "MODO_NOTURNO") {
        Serial.println("Alternando Modo Noturno");
        if (modoNoturno == 0) {
          modoNoturno = 1;
        } else {
          modoNoturno = 0;
        }
      }
    } else {
      Serial.println("  Nothing new.");
    }
  } else {
    Serial.println("Erro na escrita do canal. HTTP error code " + String(x));
  }
}

void wiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
      digitalWrite(13, HIGH);//Em sequencia do proximo comando ao Pino 13 faz piscar um led durante a conexão
      piscaLed(13, 250, 0);
      Serial.print(".");

    }
    Serial.println("");
    Serial.println("WiFi connected");
  }
  if (modoNoturno == 1) {
    digitalWrite(13, LOW); // mantem acesso o led caso a conexão seja estabelecida
  } else {
    digitalWrite(13, HIGH); // mantem acesso o led caso a conexão seja estabelecida
  }

}

void piscaLed(int pino, int tempo, int repetir) {
  if (modoNoturno == 0) {
    if (repetir == 0) {
      digitalWrite(pino, HIGH);
      delay(tempo);
      digitalWrite(pino, LOW);
      delay(tempo);
    } else {
      for (int i = 0; i < repetir; i++) {
        //Serial.println("Ciclo do Pisca:"+String(i));
        digitalWrite(pino, HIGH);
        delay(tempo);
        digitalWrite(pino, LOW);
        delay(tempo);
      }
    }
  } else {
    Serial.println("Modo Noturno ativado!!!  \n Todas as Luzes Desligadas");
    if (repetir == 0) {
      delay(tempo);
      delay(tempo);
    } else {
      for (int i = 0; i < repetir; i++) {
        delay(tempo);
        delay(tempo);
      }
    }
  }
}

void updateTwitterStatus(String tsData) {
  if (client.connect(server, 80))
  {
    // Create HTTP POST Data
    tsData = "api_key=" + thingtweetAPIKey + "&status=" + tsData;
    client.print("POST /apps/thingtweet/1/statuses/update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(tsData.length());
    client.print("\n\n");
    client.print(tsData);
  }
}
// General function to POST to ThingSpeak
int httpPOST(String postMessage, String &response) {

  bool connectSuccess = false;
  connectSuccess = client.connect("api.thingspeak.com", 80);

  if (!connectSuccess) {
    return -301;
  }

  postMessage += "&headers=false";

  String Headers =  String("POST /update HTTP/1.1\r\n") +
                    String("Host: api.thingspeak.com\r\n") +
                    String("Content-Type: application/x-www-form-urlencoded\r\n") +
                    String("Connection: close\r\n") +
                    String("Content-Length: ") + String(postMessage.length()) +
                    String("\r\n\r\n");

  client.print(Headers);
  client.print(postMessage);

  long startWaitForResponseAt = millis();
  while (client.available() == 0 && millis() - startWaitForResponseAt < 5000) {
    delay(100);
  }

  if (client.available() == 0) {
    return -304; // Didn't get server response in time
  }

  if (!client.find(const_cast<char *>("HTTP/1.1"))) {
    return -303; // Couldn't parse response (didn't find HTTP/1.1)
  }

  int status = client.parseInt();
  if (status != 200) {
    return status;
  }

  if (!client.find(const_cast<char *>("\n\r\n"))) {
    return -303;
  }


  String tempString = String(client.readString());

  if (tempString == "LIGAR_AGUA") {
    Serial.println("Imprimindo o ReadString:" + tempString);
  } else {
    Serial.println("Imprimindo o ReadString so para ver o que veio:" + tempString);
    tempString.trim();
    int o = tempString.indexOf('<');
    tempString.remove(0, (o + 1));
    o = tempString.indexOf('>');
    tempString.remove(o);
    //tempString.equals("0");
    Serial.println("Resultado do Remove:" + tempString);
    if (tempString == "PINO_15") {
      Serial.println("é um 15");
    }
    o = tempString.toInt();
    Serial.println("Resultado do toInt:" + String(o));
  }
  response = tempString;


  return status;

}
