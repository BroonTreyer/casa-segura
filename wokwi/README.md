# Casa Segura — Simulação no Wokwi (ESP32)

Central de alarme residencial rodando num **ESP32 WROOM-32** simulado, com a mesma
lógica do app Casa Segura: modos de alarme, atraso de saída/entrada, senha, senha de
coação, e vigilância 24h de fumaça e água.

## Componentes
- ESP32 DevKit C v4
- LCD 16x2 (I2C, endereço 0x27)
- Teclado de membrana 4x4
- Sensor de presença PIR
- Buzzer (sirene)
- 4 botões = sensores: **Porta**, **Janela**, **Fumaça**, **Água**
- 2 LEDs: verde (sistema ok/armado) e vermelho (alarme) + 2 resistores 220Ω

## Como abrir no Wokwi (gera o link pra compartilhar)
1. Entre em **https://wokwi.com** e faça login (conta grátis).
2. Clique em **New Project → ESP32**.
3. Abra a aba **`diagram.json`**, apague o conteúdo e cole o `diagram.json` desta pasta.
4. Abra a aba **`sketch.ino`**, apague e cole o `sketch.ino` desta pasta.
5. Adicione as bibliotecas: menu **Library Manager** (ícone de biblioteca) →
   instale **"LiquidCrystal I2C"** e **"Keypad"** (já listadas em `libraries.txt`).
6. Clique em **▶ Start** para rodar.
7. **Save** (Ctrl+S) → o Wokwi gera a URL do projeto. Esse é o **link do Wokwi** pra enviar.

> Dica: o Wokwi também importa direto do GitHub. Com os arquivos nesta pasta
> (`diagram.json`, `sketch.ino`, `libraries.txt`), dá pra usar a extensão Wokwi
> do VS Code ou o import por repositório.

## Como usar na simulação
| Tecla | Ação |
|-------|------|
| `A` | Modo **Em casa** (vigia porta/janela) |
| `B` | Modo **Dormindo** (porta/janela + presença) — com atraso de saída |
| `C` | Modo **Viagem** (tudo + presença) — com atraso de saída |
| `D` | Iniciar **desarme** (abre o campo de senha) |
| `*` | Limpar o que foi digitado |
| `#` | Confirmar a senha |

- **Senha:** `1234` desarma · `9999` = **coação** (desarma e manda "socorro silencioso" no monitor serial).
- **Botões Porta/Janela/Presença:** com o sistema armado, acioná-los começa o
  **atraso de entrada** (bip) — se ninguém digitar a senha a tempo, a **sirene** toca.
- **Fumaça e Água:** disparam o alarme **mesmo desarmado** (vigilância 24h).
- Acompanhe os eventos pelo **Serial Monitor** (115200 baud) e o estado no **LCD**.
