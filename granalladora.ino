#include <arduino-timer.h>
#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>
#include <stdint.h>
#include "TouchScreen.h"

#define CANT_MAQUINAS 3
#define CANT_HORAS_INICIAL 4
#define ANCHO_CONTADOR 240
#define ALTO_CONTADOR 40
#define ANCHO_BOTON 245
#define ALTO_BOTON 60
#define COORD_X_CONTADOR 50
#define COORD_X_BOTON 45
#define MARGEN_BOTON_X 20
#define MARGEN_BOTON_Y 10

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin
#define LCD_CS A3   // Chip Select goes to Analog 3
#define LCD_CD A2  // Command/Data goes to Analog 2
#define LCD_WR A1  // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

// Assign human-readable names to some common 16-bit color values:
#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 8   // can be a digital pin
#define XP 9   // can be a digital pin

struct Contador{
  unsigned int horas;
  unsigned int minutos;
  unsigned int segundos;
};

struct Maquina{
  unsigned int pin;
  Contador contador;
  Timer<> timer;
  bool enUso;
};

Maquina maquinas[CANT_MAQUINAS];
MCUFRIEND_kbv tft;
auto lectura_tactil = timer_create_default();
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 322);
int coord_y_contador[3] = {30, 180, 330};

bool restarHora(unsigned int i){
  if(maquinas[i].contador.horas == 0) return false;
  maquinas[i].contador.horas--;
  return true;
}

bool restarMinuto(unsigned int i){
  if(maquinas[i].contador.minutos == 0){
    maquinas[i].contador.minutos = 59;
    return restarHora(i);
  }
  maquinas[i].contador.minutos--;
  return true;
}

bool restarSegundo(unsigned int i){
  if(maquinas[i].contador.segundos == 0){
    maquinas[i].contador.segundos = 59;
    return restarMinuto(i);
  }
  maquinas[i].contador.segundos--;
  return true;
}

bool actualizar(unsigned int MAQUINA_INDEX){
  bool quedanSegundos = restarSegundo(MAQUINA_INDEX);
  if(!quedanSegundos){
    // Apagar Maquina
  }
  dibujarContador(MAQUINA_INDEX);
  return quedanSegundos;
}

void dibujarContador(unsigned int i){
      tft.setTextColor(WHITE);
      tft.setTextSize(5);
      
      tft.fillRect(COORD_X_CONTADOR, coord_y_contador[i], ANCHO_CONTADOR, ALTO_CONTADOR, BLACK);
      tft.setCursor(COORD_X_CONTADOR,coord_y_contador[i]);
      tft.print(toString(maquinas[i].contador));
}

String toString(Contador c){
  String horas, minutos, segundos;

  horas = c.horas < 10 ? "0" + String(c.horas) : String(c.horas);
  minutos = c.minutos < 10 ? "0" + String(c.minutos) : String(c.minutos);
  segundos = c.segundos < 10 ? "0" + String(c.segundos) : String(c.segundos);

  return horas + ":" + minutos + ":" + segundos;
  
}

void inicializarPantalla(){
  tft.reset();
  tft.begin(0x9486);
  tft.fillScreen(BLACK);
  
  for(int i = 0; i < CANT_MAQUINAS; i++){
      tft.setTextColor(WHITE);
      tft.setTextSize(5);

      tft.setCursor(COORD_X_CONTADOR, coord_y_contador[i]);
      tft.print(toString(maquinas[i].contador));

      tft.setCursor(COORD_X_BOTON + MARGEN_BOTON_X, coord_y_contador[i] + ALTO_CONTADOR + MARGEN_BOTON_Y);
      tft.fillRect(COORD_X_BOTON, coord_y_contador[i]+ALTO_CONTADOR, ANCHO_BOTON, ALTO_BOTON, GREEN);
      tft.print("Empezar");
  }
  lectura_tactil.every(100, leerTactil);
}

void inicializarMaquinas(){
  for(int i = 0; i < CANT_MAQUINAS; i++){
    maquinas[i].enUso = false;
    maquinas[i].contador.horas = CANT_HORAS_INICIAL;
    maquinas[i].contador.minutos = 0;
    maquinas[i].contador.segundos = 0;
    maquinas[i].timer.every(1000, actualizar, (void*)i);
  }
}

void leerTactil(){
  TSPoint p = ts.getPoint();
  
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  
  if (p.z > ts.pressureThreshhold) {
    Serial.println(p.y);
    int Y = map(p.x, 80, 940, 0, tft.height());
    int X = map(p.y, 140, 800, 0, tft.width());

    for(int i = 0; i < CANT_MAQUINAS; i++){
      if(
        (COORD_X_BOTON <= X && X <= COORD_X_BOTON + ANCHO_BOTON) 
        && 
        (coord_y_contador[i]+ALTO_CONTADOR <= Y && Y <= coord_y_contador[i]+ ALTO_CONTADOR + ALTO_BOTON)
      ){
        maquinas[i].enUso = !maquinas[i].enUso;
        if(maquinas[i].enUso){
          tft.setCursor(COORD_X_BOTON + MARGEN_BOTON_X, coord_y_contador[i] + ALTO_CONTADOR + MARGEN_BOTON_Y);
          tft.fillRect(COORD_X_BOTON, coord_y_contador[i]+ALTO_CONTADOR, ANCHO_BOTON, ALTO_BOTON, RED);
          tft.print("Detener");
        }
        else{
          maquinas[i].contador.horas = CANT_HORAS_INICIAL;
          maquinas[i].contador.minutos = 0;
          maquinas[i].contador.segundos = 0;

          dibujarContador(i);
          
          tft.setCursor(COORD_X_BOTON + MARGEN_BOTON_X, coord_y_contador[i] + ALTO_CONTADOR + MARGEN_BOTON_Y);
          tft.fillRect(COORD_X_BOTON, coord_y_contador[i]+ALTO_CONTADOR, ANCHO_BOTON, ALTO_BOTON, GREEN);
          tft.print("Empezar");
        }
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  inicializarMaquinas();
  inicializarPantalla();
}

void loop() {
  for(int i = 0; i < CANT_MAQUINAS; i++){
    if(maquinas[i].enUso) maquinas[i].timer.tick();
  }
  lectura_tactil.tick();
}
