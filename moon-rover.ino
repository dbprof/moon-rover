/**************************************************************
moon-rover.ino
BlackBug Engineering
11.01.2018
https://github.com/dbprof/moon-rover
***************************************************************/

///////////////////////////ПИЩАЛКА///////////////////////////
#define BUZZER_PIN 6

///////////////////////////КЛАВИАТУРА///////////////////////////
// Библиотека клавиатуры
#include <Keypad.h>

// Переменная строк клавиатуры
const byte ROWS = 4;

// Переменная столбцов клавиатуры
const byte COLS = 4;

// Объявляем двумерный массив символов клавиатуры
// u-up, d-down, l-left, r-right, s-shot, g-go
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','u'},
  {'4','5','6','d'},
  {'7','8','9','l'},
  {'g','0','s','r'}
};

// Пины подключения строк клавиатуры
// на 1 пин подключит Arduino Uno не удалось, пришлось использовать 8
byte rowPins[ROWS] = {A0, A1, A2, A3};

//Пины подключения столбцов клавиатуры 
byte colPins[COLS] = {5, 4, 3, 2};

//Инициализация клавиатуры
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 


///////////////////////////ДИСПЛЕЙ///////////////////////////
// Подключаем библиотеку работы с 4-значным экраном
#include <TM1637.h>

// Переменная для вывода на экран
int8_t TEXT[] = {0, 0, 0, 0};

// Количество символов в переменной TubeTab библиотеки TM1637.cpp
int imax = 36;

// Переменная текущего номера символа
int icur = 0;

// Пины для подключения
#define CLK 8
#define DIO 7

// Создание объект класса
TM1637 tm1637(CLK, DIO);


///////////////////////////ШИЛД/МОТРЫ///////////////////////////
//Подключаем библиотеку для работы с I2C
#include <Wire.h>

//Подключаем библиотеку шилда моторов
#include <Adafruit_MotorShield.h>

//Подключаем библиотеку драйвера моторов Adafruit
#include "utility/Adafruit_MS_PWMServoDriver.h"

//Создаем объект шилда моторов с адресом I2C по умолчанию
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

//Создаем объекты правого и левого моторов
//Правый мотор подключен к выходу 3
Adafruit_DCMotor *RightMotor = AFMS.getMotor(3);
//Левый мотор подключен к выходу 4
Adafruit_DCMotor *LeftMotor = AFMS.getMotor(4);

//Создаем ограничение максимальной скорости для каждого мотора
int iMaxSpeedRM = 200; //50
int iMaxSpeedLM = 200; //50

///////////////////////////MAIN///////////////////////////
// Переменная признака чтения команд
bool isReading = true;
const unsigned int DIM = 16;
// Массив назначения команды
char CMD[DIM];
// Массив величины команды
int VAL[DIM];
// Счетчик команд
int iCurCmd;
// Переменная величины команды
String sVal;

// Функция очистки памяти команд
void clearAll(){
  for (int i = 0; i < DIM; i++) {
    CMD[i] = 0;
    VAL[i] = 0;
  }
  // Счетчик команд
  iCurCmd = 0;
  // Переменная величины команды
  sVal = "";
}

// Функция вывода на экран
void printAll(){
  // Не выводим двоеточие при чтении
  if (isReading) tm1637.point(false);
  else tm1637.point(true);
  
  // Выводим команду
  if (CMD[iCurCmd] == 'u') TEXT[0] = 33; // "Up"
  else if (CMD[iCurCmd] == 'd') TEXT[0] = 34; // "Down"
  else if (CMD[iCurCmd] == 'l') TEXT[0] = 35; // "Left"
  else if (CMD[iCurCmd] == 'r') TEXT[0] = 36; // "Right"
  else if (CMD[iCurCmd] == 's') TEXT[0] = 19; // "Shot" = "H"
  else TEXT[0] = 16; // "-"

  // Выводим значение
  TEXT[1] = int(int(VAL[iCurCmd]/100)%10);
  TEXT[2] = int(int(VAL[iCurCmd]/10)%10);
  TEXT[3] = int(VAL[iCurCmd]%10);
  
  tm1637.display(TEXT);
}

// Функции вывода звуков пищалки:
// Звук неверного действия
void playBad(){
  tone(BUZZER_PIN,100,500);
  delay(500);
}
// Звук выстрела
void playShot(){
  for (int i = 50; i > 30; i--){
    tone(BUZZER_PIN,i*100,100);         
    delay(100);
  }
  tone(BUZZER_PIN,100,300);
  delay(300);
  tone(BUZZER_PIN,90,300);
  delay(300);
  tone(BUZZER_PIN,100,300);
  delay(300);
}
// Звук подтверждения
void playApproove(){
  tone(BUZZER_PIN,2000,100);         
  delay(100);
}
// Звук начала движения
void playGo(){
  tone(BUZZER_PIN,3000,100);         
  delay(100);
  tone(BUZZER_PIN,4000,100);         
  delay(100);
  tone(BUZZER_PIN,5000,100);         
  delay(100);
}

// Функции движения:
// Движение вперед
void motorUp(){
  RightMotor->run(BACKWARD);
  LeftMotor->run(BACKWARD);
}
// Движение назад
void motorDown(){
  RightMotor->run(FORWARD);
  LeftMotor->run(FORWARD);
}
// Движение влево
void motorLeft(){
  RightMotor->run(FORWARD);
  LeftMotor->run(BACKWARD);
}
// Движение вправо
void motorRight(){
  RightMotor->run(BACKWARD);
  LeftMotor->run(FORWARD);
}
// Остановка движения
void motorStop(){
  RightMotor->run(RELEASE);
  LeftMotor->run(RELEASE);
}

void setup() {
  // Открываем последовательный порт (для отладки)
  // Serial.begin(9600);

  ///////////////////////////BUZZER///////////////////////////
  pinMode(BUZZER_PIN,OUTPUT);

  ///////////////////////////ШИЛД/МОТРЫ///////////////////////////
  //Создаем объект шилда мотор моторов на частоте по умолчанию 1.6KHz
  AFMS.begin();
  RightMotor->setSpeed(iMaxSpeedRM);
  LeftMotor->setSpeed(iMaxSpeedLM);

  ///////////////////////////ДИСПЛЕЙ///////////////////////////
  //Инициализация модуля
  tm1637.init();
  //Установка яркости 0~7
  tm1637.set(7); 

  ///////////////////////////КЛАВИАТУРА///////////////////////////
  clearAll();
}

void loop() 
{
  // Читаем 
  if (isReading){
    // Читаем введенный с клавиатуры символ
    char customKey = customKeypad.getKey();

    // Если считан символ нажатия кнопки
    if (customKey){
      // Если символ - команда
      // То запоминаем в массив назначение команды
      if (customKey == 'u' || customKey == 'd' || customKey == 'l' || customKey == 'r' || customKey == 's'){
        // Если величина массива превышает ограничение
        // То не изменяем счетчик и назначение команды
        if (iCurCmd <= DIM ){
          // Если в текущем цикле назначение команды записано и величина команды записана
          // То записываем следующую команду
          if (CMD[iCurCmd] != 0){
            if (VAL[iCurCmd] != 0){
              iCurCmd++;
              CMD[iCurCmd] = customKey;
              playApproove();
            }
            else if (VAL[iCurCmd] == 0){
              CMD[iCurCmd] = customKey;
              playApproove();
            }
            else playBad();
          }
          // Если в текущем цикле назначение команды не записано и величина команды не записана
          // То записываем следующую команду
          else if (CMD[iCurCmd] == 0){
            CMD[iCurCmd] = customKey;
            playApproove();
          }
          // Если в текущем цикле 
          else playBad();
        }  
        else{
          playBad();
        }
      }
      // Если символ - применить
      // То изменяем признак чтения
      else if (customKey == 'g'){
        isReading = false;
      }
      // Если символ - остальные знаки (цифры)
      // То записываем их в переменную и в массив
      else if (customKey == '0' || customKey == '1' || customKey == '2' || customKey == '3' || 
                                   customKey == '4' || customKey == '5' || customKey == '6' || 
                                   customKey == '7' || customKey == '8' || customKey == '9'){
        // Не записываем, если ранее не было назначения команды и вне диапазона
        if (CMD[iCurCmd] == 0) playBad();
        else{
          int iVal = (String(VAL[iCurCmd]) + String(customKey)).toInt();
          if (iVal > 0 && iVal <= 360){
            VAL[iCurCmd] = iVal;
            playApproove();
          }
        }
      }
      else playBad();
    }
    printAll();
  }
  
  // Выполняем считанные команды 
  else{
    playGo();
    
    int iCurLim = iCurCmd;
    for (iCurCmd = 0; iCurCmd <= iCurLim; iCurCmd++) {
      printAll();
      delay(1000);
      if (CMD[iCurCmd] == 'u'){
        motorUp();
        delay(VAL[iCurCmd]*100);
        motorStop();
        playApproove();
      }
      else if (CMD[iCurCmd] == 'd'){
        motorDown();
        delay(VAL[iCurCmd]*100);
        motorStop();
        playApproove();
      }
      else if (CMD[iCurCmd] == 'l'){
        motorLeft();
        delay(VAL[iCurCmd]*100);
        motorStop();
        playApproove();
      }
      else if (CMD[iCurCmd] == 'r'){
        motorRight();
        delay(VAL[iCurCmd]*100);
        motorStop();
        playApproove();
      }
      else if (CMD[iCurCmd] == 's'){
        for (int i = 0; i < VAL[iCurCmd]; i++){
          playShot();
        }
      }
      else {
        delay(1000);
      }
      delay(1000);
    }
    clearAll();
    isReading = true;
  }
}

