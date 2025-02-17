#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "ssd1306.h"

// Definindo os pinos
#define BUTTON_A_PIN  15
#define BUTTON_B_PIN  16
#define JOYSTICK_X_PIN  26
#define JOYSTICK_Y_PIN  27
#define JOYSTICK_BTN_PIN  22
#define UART_TX_PIN  0
#define UART_RX_PIN  1
#define SDA_PIN  4
#define SCL_PIN  5

// Declarações das variáveis globais
int joystick_x_pos = 0;
int joystick_y_pos = 0;

// Função para inicializar I2C para comunicação com o display OLED
void init_i2c_display() {
    i2c_init(i2c0, 100000);  // Inicializa o I2C com a frequência de 100 kHz
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);  // Ativa o pull-up para o SDA
    gpio_pull_up(SCL_PIN);  // Ativa o pull-up para o SCL
}

// Função para inicializar UART para comunicação Bluetooth
void init_uart_bluetooth() {
    uart_init(uart0, 9600);  // Inicializa UART com uma taxa de 9600 bauds
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);  // Configura o pino TX para UART
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);  // Configura o pino RX para UART
}

// Função para inicializar os pinos dos botões
void init_buttons() {
    gpio_set_function(BUTTON_A_PIN, GPIO_FUNC_INPUT);
    gpio_set_function(BUTTON_B_PIN, GPIO_FUNC_INPUT);
    gpio_pull_up(BUTTON_A_PIN);  // Configura o pull-up para o botão A
    gpio_pull_up(BUTTON_B_PIN);  // Configura o pull-up para o botão B
}

// Função para ler a posição X e Y do joystick
void read_joystick(int &x_pos, int &y_pos) {
    x_pos = gpio_get(JOYSTICK_X_PIN);  // Leitura do eixo X
    y_pos = gpio_get(JOYSTICK_Y_PIN);  // Leitura do eixo Y
}

// Função para verificar se o botão do joystick foi pressionado
bool is_joystick_pressed() {
    return (gpio_get(JOYSTICK_BTN_PIN) == 0);  // Retorna verdadeiro se o botão for pressionado
}

// Função para verificar se um botão foi pressionado com debounce
bool is_button_pressed(int button_pin) {
    bool state = gpio_get(button_pin) == 0;  // Verifica se o botão está pressionado
    if (state) {
        sleep_ms(50);  // Pequeno delay para debounce
    }
    return state;
}

// Função para atualizar o display com uma mensagem
void update_display(const char* message, int x = 0, int y = 0) {
    ssd1306_clear_screen();  // Limpa a tela
    ssd1306_draw_string(x, y, message);  // Desenha a string na tela
    ssd1306_display();  // Atualiza o display com as novas informações
}

// Função para enviar dados via Bluetooth (UART)
void send_bluetooth_data(const char* data) {
    uart_puts(uart0, data);  // Envia a string via Bluetooth
}

// Função para receber dados via Bluetooth (UART)
void receive_bluetooth_data(char* buffer, int max_size) {
    int i = 0;
    while (uart_is_readable(uart0) && i < max_size - 1) {
        buffer[i++] = uart_getc(uart0);  // Lê um caractere
    }
    buffer[i] = '\0';  // Adiciona o caractere nulo ao final da string
}

// Função para mover o cursor com base na posição do joystick
void move_cursor_with_joystick(int x_pos, int y_pos) {
    // Aqui você pode implementar a lógica para mover o cursor ou realizar alguma ação
    // De acordo com a posição dos eixos X e Y do joystick
}

// Função para inicializar o display OLED
void init_display() {
    ssd1306_init();  // Inicializa o display OLED
    update_display("Inicializando...");  // Exibe uma mensagem inicial
}

int main() {
    // Inicializações
    init_i2c_display();  // Inicializa o display OLED via I2C
    init_uart_bluetooth();  // Inicializa a UART para Bluetooth
    init_buttons();  // Inicializa os botões
    init_display();  // Inicializa o display OLED

    // Variáveis locais
    char bt_buffer[100];  // Buffer para dados do Bluetooth
    int x_pos, y_pos;

    // Loop principal
    while (true) {
        // Lê a posição do joystick
        read_joystick(x_pos, y_pos);

        // Movimenta o cursor com base no joystick
        move_cursor_with_joystick(x_pos, y_pos);

        // Verifica se o botão A foi pressionado para simular clique do mouse
        if (is_button_pressed(BUTTON_A_PIN)) {
            // Ação para clicar
            update_display("Ação de Clique", 0, 10);
        }

        // Verifica se o botão B foi pressionado para comunicar com o smartphone
        if (is_button_pressed(BUTTON_B_PIN)) {
            // Ação para comunicação com smartphone via Bluetooth
            send_bluetooth_data("Mensagem para smartphone");
        }

        // Verifica se o joystick foi pressionado
        if (is_joystick_pressed()) {
            // Ação para pressionamento do joystick
            update_display("Joystick pressionado", 0, 20);
        }

        // Recebe dados do Bluetooth (se houver)
        receive_bluetooth_data(bt_buffer, sizeof(bt_buffer));
        if (bt_buffer[0] != '\0') {
            update_display(bt_buffer);  // Exibe a mensagem recebida do Bluetooth
        }

        sleep_ms(10);  // Pequeno delay para evitar sobrecarga do loop
    }

    return 0;
}
