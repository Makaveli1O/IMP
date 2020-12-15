/* Demonštrácia využiťia Watchdog timera
 *  
 * IMP projekt 2020/2021 3BIT 
 * Samuel Líška(xliska20)
 */

/*
 * Použité knižnice
 */

/*knižnica pre WatchDogTimer*/
#include <avr/wdt.h> 
/*knižnica pre nevolatívnu pamäť*/
#include <EEPROM.h> 

/*
 * inicializácia konštánt pre WDT
 */
#define RESET 1
#define INTERRUPT 2
#define INTERRUPT_RESET 3
#define WD_OFF 4

/*
 * inicializácia LED pinov
 */
int cervenaLED = 4;
int modraLED = 3;
int zltaLED = 5;

/*
 * inicializácia SINGLE LED displeja(priložený obrázok)
 */

int a = 8;
int b = 9;
int c = 12;
int d = 11;
int e = 10;
int f = 7;
int g = 6;
int dot = 13;

/*
 * Premenné pri práci s WDT
 * @var WD_count  - počítadlo WD resetov
 * @var interrupt - flag ktorý indikuje prerušenie
 */
 
int WD_count;
bool interrupt= false;

void setup() {
   wdt_disable(); //zakazanie WDT podľa dokumentácie
   /*inicializácia LED svetiel
    * 
    * červená - signalizácia behu hlavného programu
    * modrá   - signalizícia Watchdog resetu aplikácie 
    * žltá    - signalizácia obsluhy prerušenia
    */
   pinMode(modraLED, OUTPUT);
   pinMode(cervenaLED, OUTPUT); //LED na doske 
   pinMode(zltaLED, OUTPUT);
   pinMode(2,INPUT); //Pin 2 ako vstupný

  /*
   * inicializácia SIGLE LED DIGIT displeja
   *       A
   *     _____
   * F  |     |  B
   *    |_____|
   * E  |     |  C
   *    |_____|
   *       D
   */
   pinMode(a ,OUTPUT);
   pinMode(b, OUTPUT);
   pinMode(c, OUTPUT);
   pinMode(d, OUTPUT);
   pinMode(e, OUTPUT);
   pinMode(f, OUTPUT);
   pinMode(g, OUTPUT);
   pinMode(dot, OUTPUT);

  /* po 3 po sebe idúcich resetoch od WD prepni na prerušenie */
   if(EEPROM.get(0, WD_count)){
      if(WD_count >= 3){
        watchdogSetup(INTERRUPT);
        return;
      }else{
        EEPROM.get(0, WD_count);
        WD_count++;
        EEPROM.put(0, WD_count);
      }
    }else{
      WD_count = 1;
      EEPROM.put(0,WD_count);
    }

    if(EEPROM.get(10,interrupt)){
      if(interrupt == true){
        interrupt = false;
        EEPROM.put(10,interrupt);
        interruptVerify();
      }

    }
   
    /*Sériová komunikácia*/
    Serial.begin(9600);
    digitalWrite(modraLED, HIGH); 
    delay(200);
    digitalWrite(modraLED, LOW); 
    displayNumber(WD_count,false);
    //Serial.println(WD_count);
    //Serial.println(interrupt);
    Serial.end();
    /*nastavenie Watchdog modulu*/
    watchdogSetup(RESET);
    Serial.end();

}
/*
 * Hlavná smyčka, simuluje beh vlaku
 */
 
void loop() {
     if(digitalRead(2) == HIGH) { //čítanie tlačidla resetujúceho WDT
        resetCounter();
        wdt_reset();              
      }
       runTrain();
}

/*
 * Funkcia ktorá nastavuje watchdog podľa potreby
 */

void watchdogSetup(int mode){
  cli(); // vypnutie prerušení
  /*
   WDTCSR configuration:
   WDIE = Interrupt Enable
   WDE  = Reset Enable
   WDP3 = For 2000ms Time-out
   WDP2 = For 2000ms Time-out
   WDP1 = For 2000ms Time-out
   WDP0 = For 2000ms Time-out
   Obrázok podrobnejšie
  */
  // vstup do konfiguračného módu WDOGu
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  /* nastavenie WDOG registra
   * 1) reset
   * 2) interruput
   * 3) interrupt + reset
   * 4) turn off
   */
  switch(mode){
    case 1:
      WDTCSR = (0<<WDIE) | (1<<WDE) | (0<<WDP3) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0);

    break;
    case 2:  
      WDTCSR = (1<<WDIE) | (0<<WDE) | (0<<WDP3) | (0<<WDP2) | (0<<WDP1) | (1<<WDP0);

    break;
    case 3:
      WDTCSR = (1<<WDIE) | (1<<WDE) | (1<<WDP3) | (0<<WDP2) | (0<<WDP1) | (0<<WDP0);

    break;
    case 4:
      WDTCSR = (0<<WDIE) | (0<<WDE) | (0<<WDP3) | (0<<WDP2) | (0<<WDP1) | (0<<WDP0);

     break;
  }
  sei(); //opätovné zapnutie prerušení
}

/*
 * Interrupt service routine - funkcia ktorá obstaráva prerušenie(je automaticky zavolané ked nastane prerušenie
 */
ISR(WDT_vect){
  Serial.begin(9600);
  digitalWrite(zltaLED, HIGH);
  delay(200);
  interrupt = true;
  EEPROM.put(10,interrupt);
  resetCounter();
  watchdogSetup(RESET);
  wdt_enable(WDTO_15MS);
  digitalWrite(zltaLED, LOW);
  delay(200);
  Serial.end();
}

/*
 * Overovanie rešňovodiča po prerušení(kontrolná otázka na displeji a následný input do serial terminálu)
 */

void interruptVerify(){
  randomSeed(analogRead(0));
  int question = random(0,9);
  displayNumber(question, true);
  
  int i = 7, answer = 0;
  bool lateResponse = false; 
  
  Serial.begin(9600);
  digitalWrite(zltaLED, HIGH);
  delay(100);

  /* vygeneruje náhodné číslo od 1 do 9, vypíše na displej a čaká na potvrdenie od užívateľa do terminálu*/
  Serial.println("Zadajte číslo z displeja");

  /*program čaká 7 sekúnd na zadanie čísla z displeja, potom sa vyhodnotí ako nesprávna*/
  while(Serial.available()==0){
    Serial.print(i);Serial.print(" ");
    i--;
    delay(900);
    digitalWrite(13,HIGH);
    delay(100);
    digitalWrite(13,LOW);
    if(i == 0){
      Serial.println();
      lateResponse=true;
      break;
    }
  }
  Serial.println("");
  if(!lateResponse) answer = Serial.parseInt();
  else answer = -1;
  if(answer == question){>
    Serial.print(question);
    Serial.print(" : ");
    Serial.println(answer);
    Serial.println("Správna verifikácia vlak môže pokračovať!");
    digitalWrite(zltaLED, LOW);
    delay(100);
    wdt_enable(WDTO_15MS);
    resetCounter();
  }else{
    Serial.print(question);
    Serial.print(" : ");
    Serial.println(answer);
    Serial.println("Nesprávna verifikácia vlak zastavuje.");
    digitalWrite(cervenaLED, HIGH);
    digitalWrite(modraLED, HIGH);
    resetCounter();
    DIGIT_wrong();
    while(1){}
  }
  Serial.end();
}

/*
 * beh vlaku(blikanie červenej led
 *       A
 *     _____
 * F  |     |  B
 *    |_____|
 * E  |     |  C
 *    |_____|
 *       D
 */
void runTrain(){
  digitalWrite(cervenaLED, HIGH); //blikaj cervenu LED
  delay(100);
  digitalWrite(cervenaLED,LOW); 
  delay(100);
  return;  
}

/*
 * reštartovanie WD countera
 */
void resetCounter(){
  EEPROM.get(0,WD_count);
  WD_count=-1;
  EEPROM.put(0, WD_count);
  return;
}

/*LED DISPLAY FUNKCIE*/

/*
 * Vykreslenie čísel na single digit displej
 */
int displayNumber(int number, bool dot){
  switch(number){
    case 0:
      DIGIT_zero(dot);
      break;
    case 1:
      DIGIT_one(dot);
      break;  
    case 2:
      DIGIT_two(dot);
      break;  
    case 3:
      DIGIT_three(dot);
      break;  
    case 4:
      DIGIT_four(dot);
      break;  
    case 5:
      DIGIT_five(dot);
      break;
    case 6:
      DIGIT_six(dot);
      break;  
    case 7:
      DIGIT_seven(dot);
      break;  
    case 8:
      DIGIT_eight(dot);
      break;  
    case 9:
      DIGIT_nine(dot);
      break;   
    default:
      return -1; 
  }
  return 0;
}

/*
 * zobrazenie pomlčky na displeji
 */

void DIGIT_wrong(){
  digitalWrite(a,LOW);
  digitalWrite(b,LOW);
  digitalWrite(c,LOW);
  digitalWrite(d,LOW);
  digitalWrite(e,LOW);
  digitalWrite(f,LOW);
  digitalWrite(g,HIGH);
  digitalWrite(dot, LOW);
  return; 
}

void DIGIT_zero(bool display_dot){
  if (display_dot){
    digitalWrite(dot,HIGH);  
  }else{
    digitalWrite(dot,LOW); 
  }
  digitalWrite(a,HIGH);
  digitalWrite(b,HIGH);
  digitalWrite(c,HIGH);
  digitalWrite(d,HIGH);
  digitalWrite(e,HIGH);
  digitalWrite(f,HIGH);
  digitalWrite(g,LOW);
  return; 
}

void DIGIT_one(bool display_dot){
  if (display_dot){
    digitalWrite(dot,HIGH);  
  }else{
    digitalWrite(dot,LOW); 
  }
  digitalWrite(a,LOW);
  digitalWrite(b,HIGH);
  digitalWrite(c,HIGH);
  digitalWrite(d,LOW);
  digitalWrite(e,LOW);
  digitalWrite(f,LOW);
  digitalWrite(g,LOW);
  return;
}

void DIGIT_two(bool display_dot){
  if (display_dot){
    digitalWrite(dot,HIGH);  
  }else{
    digitalWrite(dot,LOW); 
  }
  digitalWrite(a,HIGH);
  digitalWrite(b,HIGH);
  digitalWrite(c,LOW);
  digitalWrite(d,HIGH);
  digitalWrite(e,HIGH);
  digitalWrite(f,LOW);
  digitalWrite(g,HIGH);
  return;
}

void DIGIT_three(bool display_dot){
  if (display_dot){
    digitalWrite(dot,HIGH);  
  }else{
    digitalWrite(dot,LOW); 
  }
  digitalWrite(a,HIGH);
  digitalWrite(b,HIGH);
  digitalWrite(c,HIGH);
  digitalWrite(d,HIGH);
  digitalWrite(e,LOW);
  digitalWrite(f,LOW);
  digitalWrite(g,HIGH);
  return;
}
void DIGIT_four(int display_dot){
  if (display_dot){
    digitalWrite(dot,HIGH);  
  }else{
    digitalWrite(dot,LOW); 
  }
  digitalWrite(a,LOW);
  digitalWrite(b,HIGH);
  digitalWrite(c,HIGH);
  digitalWrite(d,LOW);
  digitalWrite(e,LOW);
  digitalWrite(f,HIGH);
  digitalWrite(g,HIGH);
  return;
}

void DIGIT_five(int display_dot){
  if (display_dot){
    digitalWrite(dot,HIGH);  
  }else{
    digitalWrite(dot,LOW); 
  }
  digitalWrite(a,HIGH);
  digitalWrite(b,LOW);
  digitalWrite(c,HIGH);
  digitalWrite(d,HIGH);
  digitalWrite(e,LOW);
  digitalWrite(f,HIGH);
  digitalWrite(g,HIGH);
  return;
}
void DIGIT_six(bool display_dot){
  if (display_dot){
    digitalWrite(dot,HIGH);  
  }else{
    digitalWrite(dot,LOW); 
  }
  digitalWrite(a,HIGH);
  digitalWrite(b,LOW);
  digitalWrite(c,HIGH);
  digitalWrite(d,HIGH);
  digitalWrite(e,HIGH);
  digitalWrite(f,HIGH);
  digitalWrite(g,HIGH);
  return;
}

void DIGIT_seven(bool display_dot){
  if (display_dot){
    digitalWrite(dot,HIGH);  
  }else{
    digitalWrite(dot,LOW); 
  }
  digitalWrite(a,HIGH);
  digitalWrite(b,HIGH);
  digitalWrite(c,HIGH);
  digitalWrite(d,LOW);
  digitalWrite(e,LOW);
  digitalWrite(f,LOW);
  digitalWrite(g,LOW);
  return;
}

void DIGIT_eight(bool display_dot){
  if (display_dot){
    digitalWrite(dot,HIGH);  
  }else{
    digitalWrite(dot,LOW); 
  }
  digitalWrite(a,HIGH);
  digitalWrite(b,HIGH);
  digitalWrite(c,HIGH);
  digitalWrite(d,HIGH);
  digitalWrite(e,HIGH);
  digitalWrite(f,HIGH);
  digitalWrite(g,HIGH);
  return;
}

void DIGIT_nine(bool display_dot){
  if (display_dot){
    digitalWrite(dot,HIGH);  
  }else{
    digitalWrite(dot,LOW); 
  }
  digitalWrite(a,HIGH);
  digitalWrite(b,HIGH);
  digitalWrite(c,HIGH);
  digitalWrite(d,HIGH);
  digitalWrite(e,LOW);
  digitalWrite(f,HIGH);
  digitalWrite(g,HIGH);
  return;
}
