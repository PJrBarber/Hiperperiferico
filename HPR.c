// main.c - Projeto HiperperifÃ©rico ALFA para BitDogLab SE com Pico W
// Autor: Paulo Ricardo Oliveira dos Santos Junior
// DescriÃ§Ã£o:
//   - Emula um dispositivo USB HID (mouse) utilizando o joystick para movimentar o cursor.
//   - O botÃ£o do joystick executa cliques: curto (<1s) = clique esquerdo; longo (>=1s) = clique direito.
//   - BotÃ£o A (GPIO 5): Exibe "Transcrevendo tela" e toca som (tom de voz simulado) no buzzer (GPIO 12) por 5s.
//   - BotÃ£o B (GPIO 6): LÃª o microfone (ADC canal 2 â€“ GP28); se o nÃ­vel for maior que 2048 exibe "Ouvindo", senÃ£o "Pronto pra ouvir".
//   - O display OLED (SSD1306 via I2C, pinos 14/15) exibe "Em uso" quando o joystick estiver ativo e "Aguardando" caso contrÃ¡rio.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "pico/time.h"
#include "tusb.h"  // TinyUSB para USB HID

// =====================
// DefiniÃ§Ãµes de Pinos e ParÃ¢metros
// =====================

// Display OLED (assumindo SSD1306)
#define I2C_PORT            i2c0
#define PIN_SDA             14
#define PIN_SCL             15
#define SSD1306_ADDR         0x3C

// BotÃµes externos
#define BUTTON_A_PIN        5   // BotÃ£o A
#define BUTTON_B_PIN        6   // BotÃ£o B

// BotÃ£o do joystick
#define JOY_BUTTON_PIN      22

// Joystick ADC (movimento)
#define JOY_X_ADC_CHANNEL   0   // GP26
#define JOY_Y_ADC_CHANNEL   1   // GP27

// Microfone ADC (para leitura de Ã¡udio)
#define MIC_ADC_CHANNEL     2   // GP28

// Buzzer (usando PWM)
#define BUZZER_PIN          12

// Display OLED: dimensÃµes
#define SSD1306_WIDTH       128
#define SSD1306_HEIGHT      64
#define SSD1306_BUFFER_SIZE (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

// =====================
// FunÃ§Ãµes para o Display OLED (ImplementaÃ§Ã£o bÃ¡sica do SSD1306 via I2C)
// =====================

uint8_t ssd1306_buffer[SSD1306_BUFFER_SIZE];

#define FONT_WIDTH  5
#define FONT_HEIGHT 7
#define CHAR_SPACING 1

// Para simplificaÃ§Ã£o, uma tabela mÃ­nima de fonte (apenas espaÃ§o e "!" definidos; expanda conforme necessÃ¡rio)
const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // ' ' (32)
    {0x00,0x00,0x5F,0x00,0x00}, // '!' (33)
    // ... adicione mais caracteres conforme necessÃ¡rio
};

void ssd1306_command(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking(I2C_PORT, SSD1306_ADDR, buf, 2, false);
}

void ssd1306_init(void) {
    sleep_ms(100);
    ssd1306_command(0xAE); // Display off
    ssd1306_command(0x20); // Set Memory Addressing Mode
    ssd1306_command(0x00); // Horizontal addressing
    ssd1306_command(0xB0); // Page start address
    ssd1306_command(0xC8); // COM output scan direction remapped
    ssd1306_command(0x00); // Low column address
    ssd1306_command(0x10); // High column address
    ssd1306_command(0x40); // Start line address
    ssd1306_command(0x81); // Set contrast
    ssd1306_command(0xFF);
    ssd1306_command(0xA1); // Segment re-map
    ssd1306_command(0xA6); // Normal display
    ssd1306_command(0xA8); // Multiplex ratio
    ssd1306_command(0x3F);
    ssd1306_command(0xA4); // Output follows RAM content
    ssd1306_command(0xD3); // Display offset
    ssd1306_command(0x00);
    ssd1306_command(0xD5); // Display clock divide ratio/oscillator frequency
    ssd1306_command(0xF0);
    ssd1306_command(0xD9); // Pre-charge period
    ssd1306_command(0x22);
    ssd1306_command(0xDA); // COM pins hardware configuration
    ssd1306_command(0x12);
    ssd1306_command(0xDB); // VCOMH deselect level
    ssd1306_command(0x20);
    ssd1306_command(0x8D); // Charge pump
    ssd1306_command(0x14);
    ssd1306_command(0xAF); // Display ON

    memset(ssd1306_buffer, 0, SSD1306_BUFFER_SIZE);
    // Atualiza display para mostrar tela limpa
    ssd1306_command(0x21); // Column address
    ssd1306_command(0); 
    ssd1306_command(SSD1306_WIDTH - 1);
    ssd1306_command(0x22); // Page address
    ssd1306_command(0);
    ssd1306_command((SSD1306_HEIGHT/8) - 1);
}

void ssd1306_update(void) {
    ssd1306_command(0x21);
    ssd1306_command(0); 
    ssd1306_command(SSD1306_WIDTH - 1);
    ssd1306_command(0x22);
    ssd1306_command(0);
    ssd1306_command((SSD1306_HEIGHT/8) - 1);

    const int CHUNK_SIZE = 16;
    for (int i = 0; i < SSD1306_BUFFER_SIZE; i += CHUNK_SIZE) {
        uint8_t data[CHUNK_SIZE + 1];
        data[0] = 0x40;
        int chunk = CHUNK_SIZE;
        if (i + chunk > SSD1306_BUFFER_SIZE)
            chunk = SSD1306_BUFFER_SIZE - i;
        memcpy(&data[1], &ssd1306_buffer[i], chunk);
        i2c_write_blocking(I2C_PORT, SSD1306_ADDR, data, chunk + 1, false);
    }
}

void ssd1306_clear(void) {
    memset(ssd1306_buffer, 0, SSD1306_BUFFER_SIZE);
}

void ssd1306_set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT)
        return;
    int page = y / 8;
    int index = x + page * SSD1306_WIDTH;
    uint8_t mask = 1 << (y % 8);
    if (on)
        ssd1306_buffer[index] |= mask;
    else
        ssd1306_buffer[index] &= ~mask;
}

void ssd1306_draw_char(char c, int x, int y) {
    if (c < 32 || c > 127) return;
    int index = c - 32;
    for (int col = 0; col < FONT_WIDTH; col++) {
        uint8_t line = font5x7[index][col];
        for (int row = 0; row < FONT_HEIGHT; row++) {
            if (line & (1 << row))
                ssd1306_set_pixel(x + col, y + row, true);
        }
    }
    // EspaÃ§o extra entre caracteres
    for (int row = 0; row < FONT_HEIGHT; row++) {
        ssd1306_set_pixel(x + FONT_WIDTH, y + row, false);
    }
}

void ssd1306_draw_string(const char *str, int x, int y) {
    while (*str) {
        ssd1306_draw_char(*str, x, y);
        x += FONT_WIDTH + CHAR_SPACING;
        str++;
    }
    ssd1306_update();
}

// =====================
// USB HID (Mouse) via TinyUSB
// =====================
void enviar_mouse_report(uint8_t buttons, int8_t dx, int8_t dy, int8_t wheel) {
    if (tud_hid_ready()) {
        tud_hid_mouse_report(0, buttons, dx, dy, wheel);
    }
}

// =====================
// Buzzer via PWM
// =====================
static uint slice_num;

void buzzer_init() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_wrap(slice_num, 2500);  // Define wrap para o cÃ¡lculo da frequÃªncia (base: 125MHz)
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);
    pwm_set_enabled(slice_num, true);
}

void buzzer_set_frequency(uint32_t freq) {
    float divider = (float)125000000 / (2500 * freq);
    pwm_set_clkdiv(slice_num, divider);
}

void buzzer_on() {
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 1250); // 50% duty cycle
}

void buzzer_off() {
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);
}

// Executa som "tom de voz" â€“ varia a frequÃªncia levemente â€“ por 5 segundos
void executar_som_buzzer_5s() {
    uint32_t start = time_us_32();
    while ((time_us_32() - start) < 5000000UL) { // 5 segundos
        // FrequÃªncia variando para simular um tom de voz: entre 680 e 720 Hz
        uint32_t freq = 680 + (rand() % 41);
        buzzer_set_frequency(freq);
        buzzer_on();
        sleep_ms(100);
        buzzer_off();
        sleep_ms(50);
    }
}

// =====================
// Leitura do Microfone (ADC)
// =====================
uint16_t ler_microfone() {
    adc_select_input(MIC_ADC_CHANNEL);
    return adc_read();
}

// =====================
// Processamento do Joystick (movimento do mouse)
// =====================
void processar_joystick() {
    adc_select_input(JOY_X_ADC_CHANNEL);
    uint16_t adc_x = adc_read();
    adc_select_input(JOY_Y_ADC_CHANNEL);
    uint16_t adc_y = adc_read();

    int x_offset = (int)adc_x - 2048;
    int y_offset = (int)adc_y - 2048;
    if (abs(x_offset) < 100) x_offset = 0;
    if (abs(y_offset) < 100) y_offset = 0;

    int8_t dx = (int8_t)(x_offset / 128);
    int8_t dy = (int8_t)(y_offset / 128);

    if (dx != 0 || dy != 0) {
        ssd1306_clear();
        ssd1306_draw_string("Em uso", 0, 0);
        enviar_mouse_report(0, dx, dy, 0);
    } else {
        ssd1306_clear();
        ssd1306_draw_string("Aguardando", 0, 0);
    }
}

// =====================
// Processamento do BotÃ£o do Joystick para cliques
// =====================
void processar_botao_joystick() {
    if (gpio_get(JOY_BUTTON_PIN) == 0) {  // Ativo em nÃ­vel baixo
        uint32_t inicio = to_ms_since_boot(get_absolute_time());
        while (gpio_get(JOY_BUTTON_PIN) == 0) {
            sleep_ms(10);
        }
        uint32_t duracao = to_ms_since_boot(get_absolute_time()) - inicio;
        if (duracao < 1000) {
            // Clique curto: clique esquerdo
            enviar_mouse_report(0x01, 0, 0, 0);
            sleep_ms(50);
            enviar_mouse_report(0, 0, 0, 0);
        } else {
            // Clique longo: clique direito
            enviar_mouse_report(0x02, 0, 0, 0);
            sleep_ms(50);
            enviar_mouse_report(0, 0, 0, 0);
        }
    }
}

// =====================
// Processamento do BotÃ£o A: "Transcrevendo tela" e som no buzzer
// =====================
void processar_botao_A() {
    if (gpio_get(BUTTON_A_PIN) == 0) {  // Ativo em nÃ­vel baixo
        ssd1306_clear();
        ssd1306_draw_string("Transcrevendo tela", 0, 0);
        executar_som_buzzer_5s();
        // Aguarda liberaÃ§Ã£o do botÃ£o
        while (gpio_get(BUTTON_A_PIN) == 0) { sleep_ms(10); }
    }
}

// =====================
// Processamento do BotÃ£o B: Microfone para "Ouvindo" ou "Pronto pra ouvir"
// =====================
void processar_botao_B() {
    if (gpio_get(BUTTON_B_PIN) == 0) {  // Ativo em nÃ­vel baixo
        sleep_ms(100); // debounce
        uint16_t mic_val = ler_microfone();
        ssd1306_clear();
        if (mic_val > 2048) {
            ssd1306_draw_string("Ouvindo", 0, 0);
        } else {
            ssd1306_draw_string("Pronto pra ouvir", 0, 0);
        }
        // Aguarda liberaÃ§Ã£o do botÃ£o
        while (gpio_get(BUTTON_B_PIN) == 0) { sleep_ms(10); }
    }
}

// =====================
// main
// =====================
int main() {
    stdio_init_all();
    tusb_init();

    // Configura os botÃµes com pull-up
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);
    
    gpio_init(JOY_BUTTON_PIN);
    gpio_set_dir(JOY_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(JOY_BUTTON_PIN);

    // Inicializa ADC (joystick e microfone)
    adc_init();

    // Inicializa I2C para o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

    // Inicializa o display OLED
    ssd1306_init();

    // Inicializa o buzzer
    buzzer_init();

    while (true) {
        tud_task();  // Processa as tarefas USB do TinyUSB
        processar_joystick();
        processar_botao_joystick();
        processar_botao_A();
        processar_botao_B();
        sleep_ms(50);
    }
    return 0;
}