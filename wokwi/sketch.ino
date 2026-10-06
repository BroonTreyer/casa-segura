/*
  Casa Segura — Central de alarme no ESP32 (Wokwi)
  --------------------------------------------------
  Reproduz a lógica do app Casa Segura em hardware simulado:
    - Sensores de PORTA e JANELA (reed/botão: pressionado = aberto)
    - Sensor de PRESENÇA (PIR)
    - Detector de FUMAÇA e sensor de ÁGUA (botões) — vigiados 24h
    - SIRENE (buzzer) + LED verde (sistema ok/armado) e LED vermelho (alarme)
    - TECLADO 4x4 para senha e troca de modo
    - LCD 16x2 (I2C) mostra o estado

  Teclas:
    A = modo "Em casa"   (vigia porta/janela)
    B = modo "Dormindo"  (porta/janela + presença)  -> atraso de saída
    C = modo "Viagem"    (tudo + presença)           -> atraso de saída
    D = iniciar DESARME (abre o campo de senha)
    *  = limpar o que foi digitado
    #  = confirmar a senha

  Senhas:  1234 = desarma   |   9999 = coação (desarma e manda socorro silencioso)
  Fumaça e água disparam o alarme MESMO desarmado (vigilância 24h).
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ----------------- pinos -----------------
const int PIN_PORTA  = 15;   // D15  - reed porta  (LOW = aberta)
const int PIN_JANELA = 4;    // D4   - reed janela (LOW = aberta)
const int PIN_FUMACA = 5;    // D5   - detector de fumaça (LOW = fumaça)
const int PIN_AGUA   = 18;   // D18  - sensor de água     (LOW = vazamento)
const int PIN_PIR    = 19;   // D19  - presença (HIGH = movimento)
const int PIN_BUZZER = 23;   // D23  - sirene
const int PIN_LED_OK = 17;   // TX2  - LED verde
const int PIN_LED_AL = 16;   // RX2  - LED vermelho

// teclado 4x4
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27};   // D13 D12 D14 D27
byte colPins[COLS] = {26, 25, 33, 32};   // D26 D25 D33 D32
Keypad keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ----------------- estado -----------------
enum Modo { DESARMADO, EM_CASA, DORMINDO, VIAGEM };
Modo modo = DESARMADO;
const char* nomeModo[] = {"Desarmado", "Em casa", "Dormindo", "Viagem"};

bool alarme  = false;
bool armando = false;   // atraso de saída (ligando)
bool entrada = false;   // atraso de entrada (pedindo senha)
unsigned long tMarca = 0;
const unsigned long SAIDA_MS   = 10000;  // 10 s para sair
const unsigned long ENTRADA_MS = 10000;  // 10 s para digitar a senha

const String SENHA  = "1234";
const String COACAO = "9999";
String digitado = "";

// ----------------- helpers -----------------
bool aberta(int pin)  { return digitalRead(pin) == LOW; }
bool fumaca()         { return digitalRead(PIN_FUMACA) == LOW; }
bool agua()           { return digitalRead(PIN_AGUA) == LOW; }
bool presenca()       { return digitalRead(PIN_PIR) == HIGH; }
bool vigiaPerimetro() { return modo != DESARMADO; }
bool vigiaPresenca()  { return modo == DORMINDO || modo == VIAGEM; }

void mostra(const char* l1, const char* l2) {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(l1);
  lcd.setCursor(0, 1); lcd.print(l2);
}

void atualizaLCD() {
  char l1[17], l2[17];
  if (alarme)       { snprintf(l1, 17, "** ALARME **");   snprintf(l2, 17, "Senha p/ parar"); }
  else if (entrada) { snprintf(l1, 17, "Entrada...");      snprintf(l2, 17, "Senha: %s", digitado.c_str()); }
  else if (armando) { snprintf(l1, 17, "Ligando %s", nomeModo[modo]); snprintf(l2, 17, "Saia... %lus", (SAIDA_MS - (millis() - tMarca)) / 1000 + 1); }
  else              { snprintf(l1, 17, "Casa Segura");     snprintf(l2, 17, "%s", nomeModo[modo]); }
  mostra(l1, l2);
}

void dispara(const char* motivo) {
  if (alarme) return;
  alarme = true; armando = false; entrada = false;
  digitalWrite(PIN_LED_AL, HIGH);
  Serial.print("ALARME: "); Serial.println(motivo);
}

void desarma(bool coacao) {
  alarme = false; armando = false; entrada = false; modo = DESARMADO; digitado = "";
  noTone(PIN_BUZZER);
  digitalWrite(PIN_LED_AL, LOW);
  digitalWrite(PIN_LED_OK, LOW);
  if (coacao) Serial.println(">> SENHA DE COACAO: socorro silencioso enviado a familia <<");
  else        Serial.println("Desarmado.");
}

void armaPara(Modo m) {
  modo = m; entrada = false; alarme = false; digitado = "";
  digitalWrite(PIN_LED_AL, LOW);
  if (m == DORMINDO || m == VIAGEM) {   // modos com atraso de saída
    armando = true; tMarca = millis();
    Serial.print("Ligando "); Serial.println(nomeModo[m]);
  } else {                               // "Em casa" arma na hora
    armando = false; digitalWrite(PIN_LED_OK, HIGH);
    Serial.print("Armado: "); Serial.println(nomeModo[m]);
  }
}

void tentaSenha() {
  if (digitado == SENHA)       desarma(false);
  else if (digitado == COACAO) desarma(true);
  else                         digitado = "";
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PORTA,  INPUT_PULLUP);
  pinMode(PIN_JANELA, INPUT_PULLUP);
  pinMode(PIN_FUMACA, INPUT_PULLUP);
  pinMode(PIN_AGUA,   INPUT_PULLUP);
  pinMode(PIN_PIR,    INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_OK, OUTPUT);
  pinMode(PIN_LED_AL, OUTPUT);
  Wire.begin(21, 22);
  lcd.init(); lcd.backlight();
  mostra("Casa Segura", "A/B/C arma D sai");
  Serial.println("Casa Segura ESP32 pronta. Teclas: A/B/C modos, D desarmar, # confirma.");
  delay(1200);
}

void loop() {
  unsigned long now = millis();

  // ---------- teclado ----------
  char k = keypad.getKey();
  if (k) {
    tone(PIN_BUZZER, 2000, 35);   // click
    if      (k == 'A') armaPara(EM_CASA);
    else if (k == 'B') armaPara(DORMINDO);
    else if (k == 'C') armaPara(VIAGEM);
    else if (k == 'D') { if (modo != DESARMADO || alarme) { entrada = true; digitado = ""; tMarca = now; } }
    else if (k == '*') { digitado = ""; }
    else if (k == '#') { tentaSenha(); }
    else {                         // dígito
      if (entrada || alarme) {
        digitado += k;
        if (digitado.length() > 4) digitado = digitado.substring(1);
        if (digitado == SENHA || digitado == COACAO) tentaSenha();
      }
    }
  }

  // ---------- fim do atraso de saída: arma de fato ----------
  if (armando && now - tMarca >= SAIDA_MS) {
    armando = false; digitalWrite(PIN_LED_OK, HIGH);
    Serial.print("Armado: "); Serial.println(nomeModo[modo]);
  }

  // ---------- vigilância 24h: fumaça e água ----------
  if (fumaca()) dispara("Fumaca detectada");
  if (agua())   dispara("Vazamento de agua");

  // ---------- perímetro / presença quando armado ----------
  if (!armando && !entrada && !alarme) {
    if (vigiaPerimetro() && (aberta(PIN_PORTA) || aberta(PIN_JANELA))) {
      entrada = true; digitado = ""; tMarca = now;
      Serial.println("Porta/Janela aberta - atraso de entrada");
    } else if (vigiaPresenca() && presenca()) {
      entrada = true; digitado = ""; tMarca = now;
      Serial.println("Presenca detectada - atraso de entrada");
    }
  }

  // ---------- atraso de entrada estourou -> dispara ----------
  if (entrada && now - tMarca >= ENTRADA_MS) dispara("Ninguem digitou a senha");

  // ---------- sirene / bips ----------
  if (alarme) {
    tone(PIN_BUZZER, ((now / 400) % 2 == 0) ? 1500 : 1000);   // sirene pulsante
  } else if (entrada) {
    if ((now / 700) % 2 == 0) tone(PIN_BUZZER, 2200); else noTone(PIN_BUZZER);  // bip de aviso
  } else if (armando) {
    if ((now / 900) % 2 == 0) tone(PIN_BUZZER, 2000, 80);     // bip lento ao sair
  } else {
    noTone(PIN_BUZZER);
  }

  // LED verde pisca durante o atraso de saída
  if (armando) digitalWrite(PIN_LED_OK, (now / 300) % 2);

  // ---------- LCD ----------
  static unsigned long lastLCD = 0;
  if (now - lastLCD > 250) { lastLCD = now; atualizaLCD(); }
}
