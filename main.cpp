#include <Arduino.h>

//Adaptação de CT4BB para o medidor de Radiações Gama em Sieverts (Sv) com escala de perigosidade. 19/03/2021
#include <SPI.h>
#include <LiquidCrystal.h>
#define LOG_PERIOD 15000     //Periodo de Logging em millisegundos Vlor entre 15000-60000.
#define MAX_PERIOD 60000    //Maximo periodo de logging
const byte interruptPin = 2;// Pino do Arduino definido pelo fabricante para interrupts
unsigned long counts;             //Buffer para armazenagem das contagens do BPW34
unsigned long cpm;                 //Buffer para as contagens por minuto CPM
float cpm2SvHora ;                 // Buffer para inserir o cálculo de cpm (to) Sv/hora 
float cpm2SvAnual;                 //Buffer para inserir o cálculo de cpm (to) Sv/Ano
unsigned int multiplier;           //Buffer para inserir o cálculo do multiplicador uma vez que se lê de 15 em 15 segundos. Neste caso é 4 15seg é 1/4 do minuto!
unsigned int Correc ; // Factor de correcção para a calibração do BPW34. Para já está colocado em 1
unsigned long previousMillis;      //para armazenamento do Tempo pévio das medidas
LiquidCrystal lcd(12,11,5,4,3,7);// Neste projecto não se pode usar o pino 2 do Arduino, porque será utilizado para os INTERRUPTs(Norma do fabricante para o Arduino Due)
//Usa-se em alternativa um outro pino qualquer. Usei o Pino 7 do Arduino para o Data7 do LCD


 void BPW34_impulso()// Função que executará as contagens no pino 2 do Arduino
{              
 counts++;
}

void setup()// Configuração do programa
{
pinMode(2, INPUT);   // Pino 2 do Arduino como INT0 para captura dos pulsos do BPW34
digitalWrite(2, HIGH);// Resistencias interna de PullUp
lcd.begin(16,2);//Iniciar o LCD de 16 letras X 2 linhas tipo LCD1602
lcd.clear();
lcd.home();
lcd.write("CounTer_4BB 2021");
lcd.setCursor(0,1);
lcd.write("ContadorImpulsos");
counts = 0;
cpm = 0;
multiplier = MAX_PERIOD / LOG_PERIOD;      //Calculo do multiplicador que depend do periodo de log. Neste caso o log (Janela) para captação de impulsos é de 15 segundos
attachInterrupt(digitalPinToInterrupt(2), BPW34_impulso, FALLING);  //define como ler os interrupts de BPW34 ( Pino 2, Impulsos_BPW34, na pendente de descida do impulso)
Serial.begin(9600);   // Inicia.se a comunicação com a consola de monição do projecto do Software  
}

void loop()
{                                               //Ciclo principal (main)
unsigned long currentMillis = millis();
if(currentMillis - previousMillis > LOG_PERIOD)
{
Correc = 1;// Este será o factor de correcção a inserir aquando da calibração com um medidor de radiações de referencia. A radiação normal na terra é entre 1mSv/ano a 3 mSv/ano
// O Contador pode ser calibrado ajustando este factor por forma a que ele normalmente no exterior meça entre 1 e 3 mSv/ano, ou 58 pulsos / minuto. Na janela de 15segundos são 58/4=14 impulsos
previousMillis = currentMillis;
cpm = counts * multiplier * Correc;

if ( (cpm>=0) & (cpm <= 9000000))// Imposição do limite de leitura do Arduino que é de 250KHz na porta interrupt equivalente a 250x60 = 15 milhões de impulsos/minuto. Considerei 10 milhões
{
lcd.clear();
Serial.print(cpm);
cpm2SvHora= (cpm*60*8.77)/90000;// NOTA: 1uSv/h = 171 CPM ou  8,7 mSv/ano
cpm2SvAnual= (cpm2SvHora*24*365)/1000;
Serial.write("  ");
lcd.clear();
lcd.home();
//lcd.write("CPM=");Reirei esta indicação que ocupa espaço no LCD O 1º número a aparecer será sempre a contagem de pulsos/minuto (cpm)
lcd.print(cpm);
//Depois de calculado o CPM classifiquei os vários tipos de perigo em função do nível das radiações

//NORMAL até 3mSv/ano
if((cpm >=0) & (cpm <=60))
{
   lcd.print(" ");
   lcd.print(cpm2SvHora);
   lcd.print("uSv/h");
   lcd.setCursor(0,1);
   lcd.print(cpm2SvAnual);
   lcd.print("mSv/a Normal");
   
}
//ANORMAL mas aceitável. As norma de trabalho definem o limite de 16 mSv/ano como aceitável e seguro
if ((cpm >60) &( cpm <=420))
{
  lcd.print(" ");
  lcd.print(cpm2SvHora);
  lcd.print("uSv/h");
  lcd.setCursor(0,1);
  lcd.print(cpm2SvAnual);
  lcd.print("mSv/a Anormal");
 }

//Risco de poder originar doenças cancerigenas ao longo de alguna anos com esta exposição
if((cpm >420) & (cpm <=1020))
{
  lcd.print(" ");
  lcd.print(cpm2SvHora);
  lcd.print("uSv/h");
  lcd.setCursor(0,1);
  lcd.print(cpm2SvAnual);
  lcd.print("mSv/a Risco");
}

//Perigo Radiação no espaço não permanecer mais de um ano

if((cpm >1020) & (cpm <=5220))
{
   lcd.print(" ");
   lcd.print(cpm2SvHora);
   lcd.print("uSv/h");
   lcd.setCursor(0,1);
   lcd.print(cpm2SvAnual);
   lcd.print("mSv/a Perigo");
   
}
// 
//GRAVE Tipo radiação de Fukushima e Chernobyl
if((cpm >5220) & (cpm <=171000))
{
   cpm2SvHora=cpm2SvHora/1000;
   cpm2SvAnual=cpm2SvAnual/1000;
   lcd.print(" ");
   lcd.print(cpm2SvHora);
   lcd.print("mSv/h");
   lcd.setCursor(0,1);
   lcd.print(cpm2SvAnual);
   lcd.print("Sv/a GRAVE!");
   
}
//MORTE a curto prazo queimaduras e problemas de saúde

if ((cpm >171000) & (cpm <=9000000))
{
   cpm2SvHora=cpm2SvHora/1000;
   cpm2SvAnual=cpm2SvAnual/1000;
   lcd.print(" ");
   lcd.print(cpm2SvHora);
   lcd.print("mSv/h");
   lcd.setCursor(0,1);
   lcd.print(cpm2SvAnual);
   lcd.print("Sv/a MORTE!!");
   
}

cpm=0;
counts=0;
delay(15000);                              
lcd.clear(); 

}// Fim da limitação de impulsos do Arduino

else // No caso de se exceder a capacidade de leitura do ARDUINO no INT0 do Pin 2 de 250 KHz de pulsos envia-se a informação seguinte...
{
  lcd.clear();
  lcd.home();
  lcd.print("Fora do limite"); 
  lcd.setCursor(0,1);
  lcd.print("maximo= 500 Sv/a");
  delay (15000);
  counts=0;
}//Fim do else


}//Fim do Arranque das leituras
}// Fim do Loop