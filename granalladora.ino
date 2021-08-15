#include <arduino-timer.h>
#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>
#include <stdint.h>
#include "TouchScreen.h"

#define CANT_MAQUINAS 2
#define CANT_MINUTOS_INICIAL 40

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin
#define LCD_CS A3   // Chip Select goes to Analog 3
#define LCD_CD A2  // Command/Data goes to Analog 2
#define LCD_WR A1  // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
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

#define PIN_ALARMA 29
#define INTERVALO_ALARMA 5000

struct Contador{
  unsigned int minutos;
  unsigned int segundos;
};

enum Estado{
  PAUSADA,
  DETENIDA,
  EN_USO
};

struct Boton{
  unsigned int x;
  unsigned int y;
  unsigned int w;
  unsigned int h;
  unsigned int pd_left;
  unsigned int pd_top;
  unsigned int font_size;
};

struct Maquina{
  unsigned int pin;
  Contador contador;
  Estado estado;
};

Maquina maquinas[CANT_MAQUINAS];
int pines_maquinas[3] = {23,25,27};
MCUFRIEND_kbv tft;
Timer<CANT_MAQUINAS+1> tareas;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 322);
unsigned int desp_x[2] = {0, 250};
Boton menos, mas, empezar_pausar, detener;

void sonarAlarma(unsigned int i){
  for(int cant = 0; cant < 10; cant++){
    digitalWrite(PIN_ALARMA, LOW);
    delay(500);
    digitalWrite(PIN_ALARMA, HIGH);   
  }
}

bool restarMinuto(unsigned int i){
  if(maquinas[i].contador.minutos == 0) return false;
  maquinas[i].contador.minutos--;
  return true;
}

bool restarSegundo(unsigned int i){
  if(maquinas[i].contador.segundos == 0 && restarMinuto(i)){
    maquinas[i].contador.segundos = 59;
    return true;
  }
  else if (maquinas[i].contador.segundos == 0){
    return false;
  }
  else{
    maquinas[i].contador.segundos--;
    return maquinas[i].contador.segundos > 0;  
  }
}

bool actualizar(unsigned int MAQUINA_INDEX){
  if(maquinas[MAQUINA_INDEX].estado != EN_USO) return true;
  bool quedanSegundos = restarSegundo(MAQUINA_INDEX);
  if(!quedanSegundos){
    digitalWrite(pines_maquinas[MAQUINA_INDEX], HIGH);
    dibujarContador(MAQUINA_INDEX);
    esconderBotonEmpezarPausar(MAQUINA_INDEX);
    esconderBotonDetener(MAQUINA_INDEX);
    sonarAlarma(MAQUINA_INDEX);
    maquinas[MAQUINA_INDEX].estado = DETENIDA;
    maquinas[MAQUINA_INDEX].contador.minutos = CANT_MINUTOS_INICIAL;
    maquinas[MAQUINA_INDEX].contador.segundos = 0;
    dibujarBotonEmpezarPausar(MAQUINA_INDEX);
  }
  dibujarContador(MAQUINA_INDEX);
  return true;
}

void dibujarTitulo(unsigned int MAQUINA_INDEX){
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(100+desp_x[MAQUINA_INDEX],10);
  tft.print(MAQUINA_INDEX+1);
}

void dibujarContador(unsigned int MAQUINA_INDEX){
  if(maquinas[MAQUINA_INDEX].contador.minutos == 0) tft.setTextColor(RED);
  else tft.setTextColor(WHITE);
  tft.setTextSize(5);
  tft.fillRect(30+desp_x[MAQUINA_INDEX], 120, 240, 40, BLACK);
  tft.setCursor(30+desp_x[MAQUINA_INDEX],120);
  tft.print(toString(maquinas[MAQUINA_INDEX].contador));
}

void dibujarBotonMenos(unsigned int MAQUINA_INDEX){
  tft.setTextColor(WHITE);
  tft.setTextSize(menos.font_size);
  tft.fillRect(menos.x+desp_x[MAQUINA_INDEX], menos.y, menos.w, menos.h, BLUE);
  tft.setCursor(menos.x+desp_x[MAQUINA_INDEX]+menos.pd_left, menos.y+menos.pd_top);
  tft.print("-");
}

void dibujarBotonMas(unsigned int MAQUINA_INDEX){
  tft.setTextColor(WHITE);
  tft.setTextSize(mas.font_size);
  tft.fillRect(mas.x+desp_x[MAQUINA_INDEX], mas.y, mas.w, mas.h, BLUE);
  tft.setCursor(mas.x+desp_x[MAQUINA_INDEX]+mas.pd_left, mas.y+mas.pd_top);
  tft.print("+");
}

void dibujarBotonEmpezarPausar(unsigned int MAQUINA_INDEX){
  if(maquinas[MAQUINA_INDEX].estado == EN_USO){
    tft.setTextColor(BLACK);
    tft.setTextSize(empezar_pausar.font_size);
    tft.fillRect(empezar_pausar.x+desp_x[MAQUINA_INDEX], empezar_pausar.y, empezar_pausar.w, empezar_pausar.h, WHITE);
    tft.setCursor(empezar_pausar.x+desp_x[MAQUINA_INDEX] + empezar_pausar.pd_left - 10, empezar_pausar.y + empezar_pausar.pd_top);
    tft.print("||");
  }
  else{
    tft.setTextColor(WHITE);
    tft.setTextSize(empezar_pausar.font_size);
    tft.fillRect(empezar_pausar.x+desp_x[MAQUINA_INDEX], empezar_pausar.y, empezar_pausar.w, empezar_pausar.h, GREEN);
    tft.setCursor(empezar_pausar.x+desp_x[MAQUINA_INDEX] + empezar_pausar.pd_left, empezar_pausar.y + empezar_pausar.pd_top);
    tft.print(">");
  }
}

void esconderBotonEmpezarPausar(unsigned int MAQUINA_INDEX){
  tft.fillRect(empezar_pausar.x+desp_x[MAQUINA_INDEX], empezar_pausar.y, empezar_pausar.w, empezar_pausar.h, BLACK);
}

void dibujarBotonDetener(unsigned int MAQUINA_INDEX){
  tft.setTextColor(WHITE);
  tft.setTextSize(detener.font_size);
  tft.fillRect(detener.x+desp_x[MAQUINA_INDEX], detener.y, detener.w, detener.h, RED);
  tft.setCursor(detener.x+desp_x[MAQUINA_INDEX]+detener.pd_left, detener.y+detener.pd_top);
  tft.print("X");
}

void esconderBotonDetener(unsigned int MAQUINA_INDEX){
  tft.fillRect(detener.x+desp_x[MAQUINA_INDEX], detener.y, detener.w, detener.h, BLACK);
}

void mostrarMaquina(unsigned int MAQUINA_INDEX){
  dibujarTitulo(MAQUINA_INDEX);
  dibujarContador(MAQUINA_INDEX);
  dibujarBotonMenos(MAQUINA_INDEX);
  dibujarBotonMas(MAQUINA_INDEX);
  dibujarBotonEmpezarPausar(MAQUINA_INDEX);
}

String toString(Contador c){
  String minutos, segundos;
  
  minutos = c.minutos < 10 ? "0" + String(c.minutos) : String(c.minutos);
  segundos = c.segundos < 10 ? "0" + String(c.segundos) : String(c.segundos);

  return minutos + ":" + segundos;
}

void inicializarBotones(){
  menos = {10, 40, 80, 50, 30, 15, 3};
  mas = {120, 40, 80, 50, 30, 15, 3};
  empezar_pausar = {10, 180, 200, 50, 90, 10, 4};
  detener = {10, 250, 200, 50, 90, 10, 4};
}

void inicializarPantalla(){
  tft.reset();
  tft.begin(0x9486);
  tft.fillScreen(BLACK);
  tft.setRotation(1);
  mostrarMaquina(0);
  mostrarMaquina(1);
}

void inicializarMaquinas(){
  for(int i = 0; i < CANT_MAQUINAS; i++){
    maquinas[i].estado = DETENIDA;
    maquinas[i].contador.minutos = CANT_MINUTOS_INICIAL;
  }
}

bool enRango(int x1, int y1, int x2, int y2, int w, int h){
  return x2 <= x1 && x1 <= x2+w && y2 <= y1 && y1 <= y2+h;
}

void restar(unsigned int MAQUINA_INDEX){
  if (maquinas[MAQUINA_INDEX].contador.minutos == 5) maquinas[MAQUINA_INDEX].contador.minutos = 1;
  else maquinas[MAQUINA_INDEX].contador.minutos -= 5;
  dibujarContador(MAQUINA_INDEX);
}

void sumar(unsigned int MAQUINA_INDEX){
  if (maquinas[MAQUINA_INDEX].contador.minutos == 95) maquinas[MAQUINA_INDEX].contador.minutos = 99;
  else maquinas[MAQUINA_INDEX].contador.minutos += 5;
  dibujarContador(MAQUINA_INDEX);
}

void empezarPausar(unsigned int MAQUINA_INDEX){
  if (maquinas[MAQUINA_INDEX].estado == EN_USO){
    digitalWrite(pines_maquinas[MAQUINA_INDEX], HIGH);
    maquinas[MAQUINA_INDEX].estado = PAUSADA;
  }
  else{
    digitalWrite(pines_maquinas[MAQUINA_INDEX], LOW);
    maquinas[MAQUINA_INDEX].estado = EN_USO;
    dibujarBotonDetener(MAQUINA_INDEX);
  }
  dibujarBotonEmpezarPausar(MAQUINA_INDEX);
}

void detenerMaquina(unsigned int MAQUINA_INDEX){
  digitalWrite(pines_maquinas[MAQUINA_INDEX], HIGH);
  maquinas[MAQUINA_INDEX].estado = DETENIDA;
  maquinas[MAQUINA_INDEX].contador.minutos = CANT_MINUTOS_INICIAL;
  maquinas[MAQUINA_INDEX].contador.segundos = 0; 
  dibujarContador(MAQUINA_INDEX);
  dibujarBotonEmpezarPausar(MAQUINA_INDEX);
  esconderBotonDetener(MAQUINA_INDEX);
}

void leerTactil(){
  TSPoint p = ts.getPoint();
  
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  
  if (p.z > ts.pressureThreshhold) {
    
    int X = map(p.x, 80, 940, 0, tft.width());
    int Y = map(p.y, 120, 900, 0, tft.height());

    for(int i = 0; i < CANT_MAQUINAS; i++){
      if (enRango(X,Y,menos.x+desp_x[i],menos.y, menos.w, menos.h)) restar(i);
      if (enRango(X,Y,mas.x+desp_x[i],mas.y, mas.w, mas.h)) sumar(i);
      if (enRango(X,Y,empezar_pausar.x+desp_x[i],empezar_pausar.y, empezar_pausar.w, empezar_pausar.h)) empezarPausar(i);
      if (enRango(X,Y,detener.x+desp_x[i],detener.y, detener.w, detener.h)) detenerMaquina(i);
    }
  }
}

void setup() {
  inicializarBotones();
  inicializarMaquinas();
  inicializarPantalla();

  for(int i = 0; i < CANT_MAQUINAS; i++){
    tareas.every(1000, actualizar, i);
    pinMode(pines_maquinas[i], OUTPUT);
    digitalWrite(pines_maquinas[i], HIGH);
  }

  tareas.every(100, leerTactil);
  pinMode(PIN_ALARMA, OUTPUT);
  digitalWrite(PIN_ALARMA, HIGH);
}

void loop() {
  tareas.tick();
}
