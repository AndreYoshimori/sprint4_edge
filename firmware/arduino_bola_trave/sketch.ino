#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

Adafruit_MPU6050 mpu;

const float GRAVIDADE = 9.80665f;

float limiar_g        = 1.5f;   
float corte_medio_g   = 3.0f;   
float corte_forte_g   = 5.0f;  
unsigned int corte_medio_ms = 200;   
unsigned int corte_forte_ms = 400;

const uint16_t tempo_queda_ms    = 15;   
const uint16_t tempo_bloqueio_ms = 300;  

enum Estado { ESPERA, IMPACTO };
Estado estado = ESPERA;

unsigned long inicio_impacto_ms = 0;
unsigned long ultima_acima_ms   = 0;
unsigned long ultimo_evento_ms  = 0;
float pico_g = 0.0f;

unsigned long bolas_na_trave = 0;
unsigned long total_fracos   = 0;
unsigned long total_medios   = 0;
unsigned long total_fortes   = 0;

const char* classificar_chute(float pico_g_val, unsigned long duracao_ms) {
  if (pico_g_val >= corte_forte_g && duracao_ms >= corte_forte_ms) return "FORTE";
  if (pico_g_val >= corte_medio_g || duracao_ms >= corte_medio_ms) return "MEDIO";
  return "FRACO";
}

void setup() {
  Serial.begin(115200);
  Wire.begin(); 

  if (!mpu.begin()) {
    
    while (1) { delay(1000); }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_94_HZ);
  delay(100);

  Serial.println(F("MPU6050 OK. Aguardando impactos..."));
}

void loop() {
  sensors_event_t acel, giro, temp;
  mpu.getEvent(&acel, &giro, &temp);

  const float ax = acel.acceleration.x;
  const float ay = acel.acceleration.y;
  const float az = acel.acceleration.z;

  const float modulo_ms2 = sqrtf(ax*ax + ay*ay + az*az);
  const float modulo_g   = modulo_ms2 / GRAVIDADE;
  float alta_passagem_g  = modulo_g - 1.0f; 
  if (alta_passagem_g < 0) alta_passagem_g = 0;

  const unsigned long agora = millis();

  switch (estado) {
    case ESPERA:
      if ((agora - ultimo_evento_ms) > tempo_bloqueio_ms && alta_passagem_g >= limiar_g) {
        estado = IMPACTO;
        inicio_impacto_ms = agora;
        ultima_acima_ms   = agora;
        pico_g            = alta_passagem_g;
      }
      break;

    case IMPACTO:
      if (alta_passagem_g > pico_g) pico_g = alta_passagem_g;
      if (alta_passagem_g >= limiar_g) {
        ultima_acima_ms = agora;
      }

      if ((agora - ultima_acima_ms) >= tempo_queda_ms) {
        const unsigned long duracao_ms = agora - inicio_impacto_ms;
        ultimo_evento_ms = agora;
        estado = ESPERA;

        bolas_na_trave++;
        const char* categoria = classificar_chute(pico_g, duracao_ms);
        if (categoria[0] == 'F' && categoria[1] == 'O') total_fortes++;
        else if (categoria[0] == 'M')                   total_medios++;
        else                                            total_fracos++;

        Serial.print(F("[#")); Serial.print(bolas_na_trave);
        Serial.print(F("] categoria=")); Serial.print(categoria);
        Serial.print(F(" | g_pico=")); Serial.print(pico_g, 2); Serial.print(F(" g"));
        Serial.print(F(" | duracao=")); Serial.print(duracao_ms); Serial.print(F(" ms"));
        Serial.print(F(" | cont(fraco/medio/forte)="));
        Serial.print(total_fracos); Serial.print('/');
        Serial.print(total_medios); Serial.print('/');
        Serial.println(total_fortes);

        pico_g = 0.0f; 
      }
      break;
  }
}