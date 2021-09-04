#include <arduino-timer.h>
#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>
#include <stdint.h>
#include "TouchScreen.h"
#include <ArduinoQueue.h>

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
  Contador copia_contador;
  Estado estado;
  bool suenaAlarma;
};

Maquina maquinas[CANT_MAQUINAS];
int pines_maquinas[3] = {23,25,27};
MCUFRIEND_kbv tft;
Timer<CANT_MAQUINAS+2> tareas;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 322);
unsigned int desp_x[2] = {0, 250};
Boton menos, mas, empezar_pausar, detener, alarma;
bool pulsacion = false;
ArduinoQueue<unsigned int> alarmas(CANT_MAQUINAS);
int cant_sonadas[CANT_MAQUINAS] = {4,10};
int intervalo[CANT_MAQUINAS] = {30,10};
int sonadasRestantes, intervaloRestante;
bool alarmaEnUso = false, sonando = false, esperando = false;

bool sonarAlarma(){
  if(!alarmas.isEmpty()){
    if(!alarmaEnUso){
      digitalWrite(PIN_ALARMA, LOW);
      sonadasRestantes = cant_sonadas[alarmas.getHead()];
      intervaloRestante = intervalo[alarmas.getHead()];
      alarmaEnUso = true;
      sonando = true;
    }
    if(sonando && --intervaloRestante == 0){
      digitalWrite(PIN_ALARMA, HIGH);
      intervaloRestante = intervalo[alarmas.getHead()];
      sonando = false;
      if(--sonadasRestantes == 0){
        alarmaEnUso = false;
        alarmas.dequeue();
      }
    }
    else if(!sonando && --intervaloRestante == 0){
      digitalWrite(PIN_ALARMA, LOW);
      intervaloRestante = intervalo[alarmas.getHead()];
      sonando = true;
    }
  }
  return true;
}

bool restarMinuto(unsigned int i){
  if(maquinas[i].contador.minutos == 0) return false;
  maquinas[i].contador.minutos--;
  return true;
}

bool restarSegundo(unsigned int i){
  if(maquinas[i].contador.segundos == 0){
    bool quedanMinutos = restarMinuto(i);
    if (quedanMinutos) maquinas[i].contador.segundos = 59;
    return quedanMinutos;
  }
  maquinas[i].contador.segundos--;
  return true;
}

bool actualizar(unsigned int MAQUINA_INDEX){
  if(maquinas[MAQUINA_INDEX].estado != EN_USO) return true;
  bool quedanSegundos = restarSegundo(MAQUINA_INDEX);
  if(!quedanSegundos){
    digitalWrite(pines_maquinas[MAQUINA_INDEX], HIGH);
    dibujarContador(MAQUINA_INDEX);
    esconderBotonDetener(MAQUINA_INDEX);
    if(maquinas[MAQUINA_INDEX].suenaAlarma) alarmas.enqueue(MAQUINA_INDEX);
    maquinas[MAQUINA_INDEX].estado = DETENIDA;
    maquinas[MAQUINA_INDEX].contador.minutos = maquinas[MAQUINA_INDEX].copia_contador.minutos;
    maquinas[MAQUINA_INDEX].contador.segundos = maquinas[MAQUINA_INDEX].copia_contador.segundos; 
    dibujarBotonEmpezarPausar(MAQUINA_INDEX);
    esconderBotonAlarma(MAQUINA_INDEX);
    dibujarBotonMas(MAQUINA_INDEX);
    dibujarBotonMenos(MAQUINA_INDEX);
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

void esconderBotonMenos(unsigned int MAQUINA_INDEX){
  tft.fillRect(menos.x+desp_x[MAQUINA_INDEX], menos.y, menos.w, menos.h, BLACK);
}

void dibujarBotonMas(unsigned int MAQUINA_INDEX){
  tft.setTextColor(WHITE);
  tft.setTextSize(mas.font_size);
  tft.fillRect(mas.x+desp_x[MAQUINA_INDEX], mas.y, mas.w, mas.h, BLUE);
  tft.setCursor(mas.x+desp_x[MAQUINA_INDEX]+mas.pd_left, mas.y+mas.pd_top);
  tft.print("+");
}

void esconderBotonMas(unsigned int MAQUINA_INDEX){
  tft.fillRect(mas.x+desp_x[MAQUINA_INDEX], mas.y, mas.w, mas.h, BLACK);
}

void dibujarBotonAlarma(unsigned int MAQUINA_INDEX){
  tft.setTextColor(WHITE);
  tft.setTextSize(mas.font_size);
  tft.fillRect(alarma.x+desp_x[MAQUINA_INDEX], alarma.y, alarma.w, alarma.h, BLUE);
  tft.setCursor(alarma.x+desp_x[MAQUINA_INDEX]+alarma.pd_left, alarma.y+alarma.pd_top);
  if (maquinas[MAQUINA_INDEX].suenaAlarma) tft.print("Alarm ON");
  else tft.print("Alarm OFF");
}

void esconderBotonAlarma(unsigned int MAQUINA_INDEX){
  tft.fillRect(alarma.x+desp_x[MAQUINA_INDEX], alarma.y, alarma.w, alarma.h, BLACK);
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
  alarma = {10, 40, 200, 50, 30, 15, 3};
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
    maquinas[i].contador.segundos = 0;
    maquinas[i].suenaAlarma = true;
  }
}

bool enRango(int x1, int y1, int x2, int y2, int w, int h){
  return x2 <= x1 && x1 <= x2+w && y2 <= y1 && y1 <= y2+h;
}

void restar(unsigned int MAQUINA_INDEX){
  if (maquinas[MAQUINA_INDEX].contador.minutos <= 5) maquinas[MAQUINA_INDEX].contador.minutos = 1;
  else if (maquinas[MAQUINA_INDEX].contador.minutos == 99) maquinas[MAQUINA_INDEX].contador.minutos = 95;
  else maquinas[MAQUINA_INDEX].contador.minutos -= 5;
  dibujarContador(MAQUINA_INDEX);
}

void sumar(unsigned int MAQUINA_INDEX){
  if (maquinas[MAQUINA_INDEX].contador.minutos >= 95) maquinas[MAQUINA_INDEX].contador.minutos = 99;
  else if (maquinas[MAQUINA_INDEX].contador.minutos == 1) maquinas[MAQUINA_INDEX].contador.minutos = 5;
  else maquinas[MAQUINA_INDEX].contador.minutos += 5;
  dibujarContador(MAQUINA_INDEX);
}

void cambiarEstadoAlarma(unsigned int MAQUINA_INDEX){
  maquinas[MAQUINA_INDEX].suenaAlarma = !maquinas[MAQUINA_INDEX].suenaAlarma;
  dibujarBotonAlarma(MAQUINA_INDEX);
}

void empezarPausar(unsigned int MAQUINA_INDEX){
  if (maquinas[MAQUINA_INDEX].estado == EN_USO){
    digitalWrite(pines_maquinas[MAQUINA_INDEX], HIGH);
    maquinas[MAQUINA_INDEX].estado = PAUSADA;
  }
  else{
    if(maquinas[MAQUINA_INDEX].estado == DETENIDA){
      maquinas[MAQUINA_INDEX].copia_contador.minutos = maquinas[MAQUINA_INDEX].contador.minutos;
      maquinas[MAQUINA_INDEX].copia_contador.segundos = maquinas[MAQUINA_INDEX].contador.segundos;
      esconderBotonMas(MAQUINA_INDEX);
      esconderBotonMenos(MAQUINA_INDEX);
      dibujarBotonAlarma(MAQUINA_INDEX);
    }
    digitalWrite(pines_maquinas[MAQUINA_INDEX], LOW);
    maquinas[MAQUINA_INDEX].estado = EN_USO;
    dibujarBotonDetener(MAQUINA_INDEX);
  }
  dibujarBotonEmpezarPausar(MAQUINA_INDEX);
}

void detenerMaquina(unsigned int MAQUINA_INDEX){
  digitalWrite(pines_maquinas[MAQUINA_INDEX], HIGH);
  maquinas[MAQUINA_INDEX].estado = DETENIDA;
  maquinas[MAQUINA_INDEX].contador.minutos = maquinas[MAQUINA_INDEX].copia_contador.minutos;
  maquinas[MAQUINA_INDEX].contador.segundos = maquinas[MAQUINA_INDEX].copia_contador.segundos; 
  dibujarContador(MAQUINA_INDEX);
  dibujarBotonEmpezarPausar(MAQUINA_INDEX);
  esconderBotonDetener(MAQUINA_INDEX);
  esconderBotonAlarma(MAQUINA_INDEX);
  dibujarBotonMas(MAQUINA_INDEX);
  dibujarBotonMenos(MAQUINA_INDEX);
  
}

void leerTactil(){
  TSPoint p = ts.getPoint();
  
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  
  if (p.z > ts.pressureThreshhold && !pulsacion) {
    pulsacion = true;
    int X = map(p.x, 80, 940, 0, tft.width());
    int Y = map(p.y, 120, 900, 0, tft.height());

    for(int i = 0; i < CANT_MAQUINAS; i++){
      if (maquinas[i].estado == DETENIDA && enRango(X,Y,menos.x+desp_x[i],menos.y, menos.w, menos.h)) restar(i);
      if (maquinas[i].estado == DETENIDA && enRango(X,Y,mas.x+desp_x[i],mas.y, mas.w, mas.h)) sumar(i);
      if ((maquinas[i].estado == EN_USO || maquinas[i].estado == PAUSADA) && enRango(X,Y,alarma.x+desp_x[i],alarma.y, alarma.w, alarma.h)) cambiarEstadoAlarma(i);
      if (enRango(X,Y,empezar_pausar.x+desp_x[i],empezar_pausar.y, empezar_pausar.w, empezar_pausar.h)) empezarPausar(i);
      if (enRango(X,Y,detener.x+desp_x[i],detener.y, detener.w, detener.h)) detenerMaquina(i);
    }
  }
  else if (p.z <= ts.pressureThreshhold) {
    pulsacion = false;
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

  tareas.every(50, leerTactil);
  pinMode(PIN_ALARMA, OUTPUT);
  digitalWrite(PIN_ALARMA, HIGH);

  tareas.every(100, sonarAlarma);
}

void loop() {
  tareas.tick();
}
