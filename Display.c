# Hiperperiferico
código necessário pra transformar uma BitDogLab em um hiperperiférico
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR   0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void exibirMensagem(const char *mensagem) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println(mensagem);
    display.display();
}

int verificarSistema() {
    // Adicione as verificações aqui
    return 0; // Retorne um código de erro caso necessário
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Wire.begin(14, 15);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("Erro ao inicializar o display!");
    }
    display.clearDisplay();
    display.display();

    int status = verificarSistema();
    if (status == 0) {
        exibirMensagem("Sistema OK");
    } else {
        char msg[40];
        sprintf(msg, "Erro: %d", status);
        exibirMensagem(msg);
    }
}

void loop() {
    // Adicione chamadas às funções conforme necessário
}
