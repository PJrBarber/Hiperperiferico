// Includes necessários
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "bsp/board.h"
#include "tusb.h"
#include <string.h>
#include <stdio.h>

// Definições dos pinos
#define OLED_SDA 14
#define OLED_SCL 15
#define BUTTON_A 5
#define BUTTON_B 6
#define JOYSTICK_X 26
#define JOYSTICK_Y 27
#define JOYSTICK_BTN 22

// Constantes do display OLED
#define OLED_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// Estados do sistema
typedef enum {
    STATE_INIT,
    STATE_READY,
    STATE_LISTENING,
    STATE_SPEAKING,
    STATE_IN_USE
} SystemState;

// Estrutura para controle do cursor
typedef struct {
    int8_t x;
    int8_t y;
    uint8_t buttons;
} MouseReport;

// Variáveis globais
SystemState currentState = STATE_INIT;
MouseReport mouseReport = {0, 0, 0};
bool mouse_mounted = false;

// Protótipos das funções
void init_i2c(void);
void init_gpio(void);
void init_adc(void);
bool init_system(void);
void update_display(const char* message);
void process_joystick(void);
void handle_buttons(void);
void send_hid_report(void);

// Função para inicialização do USB HID
void init_usb_hid(void) {
    board_init();
    tusb_init();
}

// Função de inicialização do I2C
void init_i2c(void) {
    i2c_init(i2c1, 400000);
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);
}

// Função de inicialização dos GPIOs
void init_gpio(void) {
    gpio_init(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_init(JOYSTICK_BTN);
    
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    
    gpio_pull_up(BUTTON_A);
    gpio_pull_up(BUTTON_B);
    gpio_pull_up(JOYSTICK_BTN);
}

// Função de inicialização do ADC
void init_adc(void) {
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
}

// Função de inicialização do sistema
bool init_system(void) {
    stdio_init_all();
    init_i2c();
    init_gpio();
    init_adc();
    init_usb_hid();
    
    bool system_ok = true;
    
    if (system_ok) {
        update_display("Ola! Vamos iniciar.");
        currentState = STATE_READY;
        return true;
    } else {
        update_display("Erro no sistema!");
        return false;
    }
}

// Função para atualizar o display (simulada)
void update_display(const char* message) {
    // Aqui seria implementada a lógica real do display OLED
    printf("Display: %s\n", message);
}

// Função para processar entradas do joystick
void process_joystick(void) {
    adc_select_input(0);
    uint16_t x = adc_read();
    adc_select_input(1);
    uint16_t y = adc_read();
    
    // Converte leituras ADC para movimentos do cursor
    mouseReport.x = (x > 2048) ? 5 : (x < 1024) ? -5 : 0;
    mouseReport.y = (y > 2048) ? 5 : (y < 1024) ? -5 : 0;
}

// Função para processar botões
void handle_buttons(void) {
    // Botão esquerdo do mouse
    if (!gpio_get(BUTTON_A)) {
        mouseReport.buttons |= MOUSE_BUTTON_LEFT;
    } else {
        mouseReport.buttons &= ~MOUSE_BUTTON_LEFT;
    }
    
    // Botão direito do mouse
    if (!gpio_get(BUTTON_B)) {
        mouseReport.buttons |= MOUSE_BUTTON_RIGHT;
    } else {
        mouseReport.buttons &= ~MOUSE_BUTTON_RIGHT;
    }
}

// Função para enviar relatório HID
void send_hid_report(void) {
    if (tud_mounted() && tud_hid_ready()) {
        tud_hid_mouse_report(REPORT_ID_MOUSE, 
                            mouseReport.buttons,
                            mouseReport.x,
                            mouseReport.y,
                            0,  // wheel
                            0); // pan
    }
}

// Função principal
int main(void) {
    if (!init_system()) {
        return -1;
    }
    
    update_display("Pronto pra uso");
    
    while (true) {
        tud_task(); // Tarefa USB
        
        if (tud_mounted()) {
            if (!mouse_mounted) {
                mouse_mounted = true;
                update_display("Mouse conectado");
            }
            
            process_joystick();
            handle_buttons();
            send_hid_report();
        } else {
            if (mouse_mounted) {
                mouse_mounted = false;
                update_display("Mouse desconectado");
            }
        }
        
        sleep_ms(10); // Pequeno delay para estabilidade
    }
    
    return 0;
}

// Callbacks necessários para USB HID
void tud_mount_cb(void) {
    mouse_mounted = true;
}

void tud_umount_cb(void) {
    mouse_mounted = false;
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
}

void tud_resume_cb(void) {
}
