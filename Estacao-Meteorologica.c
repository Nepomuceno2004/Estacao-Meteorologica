#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "aht20.h"
#include "bmp280.h"
#include "ssd1306.h"
#include "font.h"
#include <math.h>

#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include <string.h>

#define I2C_PORT i2c0               // i2c0 pinos 0 e 1, i2c1 pinos 2 e 3
#define I2C_SDA 0                   // 0 ou 2
#define I2C_SCL 1                   // 1 ou 3
#define SEA_LEVEL_PRESSURE 101325.0 // Pressão ao nível do mar em Pa
// Display na I2C
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define endereco 0x3C
#define botaoA 5

#define ledRed 13

#define WIFI_SSID "HILARIO_OI FIBRA 2.4"
#define WIFI_PASS "nepomuceno"

double g_pressure = 0;
double g_temperature = 0;
double g_altitude = 0;
double g_aht_temperature = 0;
double g_aht_humidity = 0;

float temp_min = 0.0, temp_max = 50.0;
float hum_min = 0.0, hum_max = 100.0;

uint8_t tela = 0;       // Variável para controlar a tela do display
uint32_t last_time = 0; // Variável para controlar a tela do display

const char HTML_BODY[] =
    "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Estação</title>"
    "<style>"
    "body{font-family:sans-serif;text-align:center;margin:0;background:#222;color:#eee}"
    "h1{color:#0cf;margin-top:20px}"
    ".d{max-width:400px;margin:10px auto;background:#333;border-radius:8px;padding:15px}"
    ".l{font-weight:600;margin:8px 0;font-size:14px;color:#ddd}"
    ".g{display:flex;flex-direction:column;align-items:center;gap:20px;margin:20px 0}"
    ".c{width:90%;max-width:600px;background:#fff;border-radius:8px;padding:10px}"
    "canvas{width:100%;height:240px}"
    "</style>"
    "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>"
    "<script>"
    "let tD=[],hD=[],bD=[],l=[],tC,hC,bC;"
    "function sC(){"
    "tC=new Chart(document.getElementById('t').getContext('2d'),{type:'line',data:{labels:l,datasets:[{label:'Temp AHT',data:tD,borderColor:'red',backgroundColor:'rgba(255,0,0,0.1)',fill:true}]},options:{responsive:true,animation:false}});"
    "hC=new Chart(document.getElementById('h').getContext('2d'),{type:'line',data:{labels:l,datasets:[{label:'Umidade AHT',data:hD,borderColor:'blue',backgroundColor:'rgba(0,0,255,0.1)',fill:true}]},options:{responsive:true,animation:false}});"
    "bC=new Chart(document.getElementById('b').getContext('2d'),{type:'line',data:{labels:l,datasets:[{label:'Temp BMP',data:bD,borderColor:'limegreen',backgroundColor:'rgba(50,205,50,0.1)',fill:true}]},options:{responsive:true,animation:false}});"
    "}"
    "function u(){"
    "fetch('/estado').then(r=>r.json()).then(d=>{"
    "document.getElementById('at').innerText=d.aht_temperature.toFixed(2)+' °C';"
    "document.getElementById('ah').innerText=d.aht_humidity.toFixed(2)+' %';"
    "document.getElementById('pr').innerText=d.pressure.toFixed(3)+' kPa';"
    "document.getElementById('te').innerText=d.temperature.toFixed(2)+' °C';"
    "document.getElementById('al').innerText=d.altitude.toFixed(2)+' m';"
    "let n=new Date().toLocaleTimeString();"
    "if(l.length>=20){l.shift();tD.shift();hD.shift();bD.shift();}"
    "l.push(n);tD.push(d.aht_temperature);hD.push(d.aht_humidity);bD.push(d.temperature);"
    "tC.update();hC.update();bC.update();"
    "}).catch(_=>{"
    "document.getElementById('pr').innerText='--';"
    "document.getElementById('te').innerText='--';"
    "document.getElementById('al').innerText='--';"
    "document.getElementById('at').innerText='--';"
    "document.getElementById('ah').innerText='--';"
    "});"
    "}"
    "window.onload=function(){sC();setInterval(u,1000);};"
    "</script></head><body>"
    "<h1>Estação Meteorológica</h1>"
    "<div class='d'>"
    "<p class='l'>Temp AHT20: <span id='at'>--</span></p>"
    "<p class='l'>Umidade AHT20: <span id='ah'>--</span></p>"
    "<p class='l'>Temp BMP280: <span id='te'>--</span></p>"
    "<p class='l'>Pressão: <span id='pr'>--</span></p>"
    "<p class='l'>Altitude: <span id='al'>--</span></p>"
    "</div>"
    "<div class='d'>"
    "<form action='/setlimits' method='get'>"
    "<p><input type='number' step='0.1' name='tmin' placeholder='Temp Mín'></p>"
    "<p><input type='number' step='0.1' name='tmax' placeholder='Temp Máx'></p>"
    "<p><input type='number' step='0.1' name='hmin' placeholder='Umid Mín'></p>"
    "<p><input type='number' step='0.1' name='hmax' placeholder='Umid Máx'></p>"
    "<input type='submit' value='Salvar'>"
    "</form></div>"
    "<div class='g'>"
    "<div class='c'><canvas id='t'></canvas></div>"
    "<div class='c'><canvas id='h'></canvas></div>"
    "<div class='c'><canvas id='b'></canvas></div>"
    "</div></body></html>";

struct http_state
{
    char response[4096];
    size_t len;
    size_t sent;
};

static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    struct http_state *hs = (struct http_state *)arg;
    hs->sent += len;
    if (hs->sent >= hs->len)
    {
        tcp_close(tpcb);
        free(hs);
    }
    return ERR_OK;
}

static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *req = (char *)p->payload;
    struct http_state *hs = malloc(sizeof(struct http_state));
    if (!hs)
    {
        pbuf_free(p);
        tcp_close(tpcb);
        return ERR_MEM;
    }
    hs->sent = 0;

    if (strstr(req, "GET /estado"))
    {
        char json_payload[256];
        int json_len = snprintf(json_payload, sizeof(json_payload),
                                "{\"pressure\":%.3f,\"temperature\":%.2f,\"altitude\":%.2f,"
                                "\"aht_temperature\":%.2f,\"aht_humidity\":%.2f}\r\n",
                                g_pressure, g_temperature, g_altitude, g_aht_temperature, g_aht_humidity);

        hs->len = snprintf(hs->response, sizeof(hs->response),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/json\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           json_len, json_payload);
    }
    else if (strstr(req, "GET /setlimits?"))
    {
        // Ponteiros para os valores na URL
        char *tmin_str = strstr(req, "tmin=");
        char *tmax_str = strstr(req, "tmax=");
        char *hmin_str = strstr(req, "hmin=");
        char *hmax_str = strstr(req, "hmax=");

        if (tmin_str && tmax_str && hmin_str && hmax_str)
        {
            // Pegando os valores numéricos
            float tmin = atof(tmin_str + 5); // pula "tmin="
            float tmax = atof(tmax_str + 5);
            float hmin = atof(hmin_str + 5);
            float hmax = atof(hmax_str + 5);

            // Atualiza variáveis globais
            temp_min = tmin;
            temp_max = tmax;
            hum_min = hmin;
            hum_max = hmax;

            printf("Limites atualizados:\nTemp: %.1f - %.1f\nUmidade: %.1f - %.1f\n", temp_min, temp_max, hum_min, hum_max);
        }

        hs->len = snprintf(hs->response, sizeof(hs->response),
                           "HTTP/1.1 302 Found\r\n"
                           "Location: /\r\n"
                           "Connection: close\r\n\r\n");
    }

    else
    {
        hs->len = snprintf(hs->response, sizeof(hs->response),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           (int)strlen(HTML_BODY), HTML_BODY);
    }

    pbuf_free(p);
    tcp_arg(tpcb, hs);
    tcp_sent(tpcb, http_sent);
    tcp_write(tpcb, hs->response, hs->len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}

static void start_http_server(void)
{
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb)
    {
        printf("Erro ao criar PCB TCP\n");
        return;
    }
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
    printf("Servidor HTTP rodando na porta 80...\n");
}

// Função para calcular a altitude a partir da pressão atmosférica
double calculate_altitude(double pressure)
{
    return 44330.0 * (1.0 - pow(pressure / SEA_LEVEL_PRESSURE, 0.1903));
}

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    if (current_time - last_time > 200)
    {
        if (gpio == botaoB)
        {
            reset_usb_boot(0, 0);
        }
        else
        {
            tela++;
            if (tela > 1)
            {
                tela = 0;
            }
        }
        last_time = current_time; // Atualiza o tempo do último evento
    }
}

int main()
{
    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    // Fim do trecho para modo BOOTSEL com botão B

    gpio_init(botaoA);
    gpio_set_dir(botaoA, GPIO_IN);
    gpio_pull_up(botaoA);
    gpio_set_irq_enabled_with_callback(botaoA, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_init(ledRed);
    gpio_set_dir(ledRed, GPIO_OUT);
    gpio_put(ledRed, 0); // Desliga o LED vermelho

    stdio_init_all();

    // I2C do Display funcionando em 400Khz.
    i2c_init(I2C_PORT_DISP, 400 * 1000);

    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA_DISP);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL_DISP);                                        // Pull up the clock line
    ssd1306_t ssd;                                                     // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT_DISP); // Inicializa o display
    ssd1306_config(&ssd);                                              // Configura o display
    ssd1306_send_data(&ssd);                                           // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Inicializa o I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o BMP280
    bmp280_init(I2C_PORT);
    struct bmp280_calib_param params;
    bmp280_get_calib_params(I2C_PORT, &params);

    // Inicializa o AHT20
    aht20_reset(I2C_PORT);
    aht20_init(I2C_PORT);

    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Iniciando Wi-Fi", 0, 0);
    ssd1306_draw_string(&ssd, "Aguarde...", 0, 30);
    ssd1306_send_data(&ssd);

    if (cyw43_arch_init())
    {
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "WiFi => FALHA", 0, 0);
        ssd1306_send_data(&ssd);
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000))
    {
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "WiFi => ERRO", 0, 0);
        ssd1306_send_data(&ssd);
        return 1;
    }

    uint8_t *ip = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
    char ip_str[24];
    snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "WiFi => OK", 0, 0);
    ssd1306_draw_string(&ssd, ip_str, 0, 10);
    ssd1306_send_data(&ssd);

    start_http_server();

    // Estrutura para armazenar os dados do sensor
    AHT20_Data data;
    int32_t raw_temp_bmp;
    int32_t raw_pressure;

    char str_tmp1[5]; // Buffer para armazenar a string
    char str_alt[5];  // Buffer para armazenar a string
    char str_tmp2[5]; // Buffer para armazenar a string
    char str_umi[5];  // Buffer para armazenar a string

    bool cor = true;
    while (1)
    {
        cyw43_arch_poll();

        // Leitura do BMP280
        bmp280_read_raw(I2C_PORT, &raw_temp_bmp, &raw_pressure);
        int32_t temperature = bmp280_convert_temp(raw_temp_bmp, &params);
        int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temp_bmp, &params);

        // Cálculo da altitude
        double altitude = calculate_altitude(pressure);

        printf("Pressao = %.3f kPa\n", pressure / 1000.0);
        printf("Temperatura BMP: = %.2f C\n", temperature / 100.0);
        printf("Altitude estimada: %.2f m\n", altitude);

        // Leitura do AHT20
        if (aht20_read(I2C_PORT, &data))
        {
            printf("Temperatura AHT: %.2f C\n", data.temperature);
            printf("Umidade: %.2f %%\n\n\n", data.humidity);
        }
        else
        {
            printf("Erro na leitura do AHT10!\n\n\n");
        }

        sprintf(str_tmp1, "%.1fC", temperature / 100.0); // Converte o inteiro em string
        sprintf(str_alt, "%.0fm", altitude);             // Converte o inteiro em string
        sprintf(str_tmp2, "%.1fC", data.temperature);    // Converte o inteiro em string
        sprintf(str_umi, "%.1f%%", data.humidity);       // Converte o inteiro em string

        //  Atualiza o conteúdo do display com animações
        if (tela == 0)
        {
            ssd1306_fill(&ssd, !cor);                     // Limpa o display
            ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor); // Desenha um retângulo
            ssd1306_line(&ssd, 3, 18, 123, 18, cor);      // Desenha uma linha
            ssd1306_draw_string(&ssd, ip_str, 9, 7);      // Desenha uma string
            ssd1306_draw_string(&ssd, "AHT20", 10, 23);   // Desenha uma string
            // ssd1306_line(&ssd, 63, 32, 63, 60, cor);      // Desenha uma linha vertical
            ssd1306_draw_string(&ssd, "Temp.", 14, 38);  // Desenha uma string
            ssd1306_draw_string(&ssd, "Humi.", 14, 51);  // Desenha uma string
            ssd1306_draw_string(&ssd, str_tmp2, 73, 38); // Desenha uma string
            ssd1306_draw_string(&ssd, str_umi, 73, 51);  // Desenha uma string
            ssd1306_send_data(&ssd);                     // Atualiza o display
        }
        else if (tela == 1)
        {
            ssd1306_fill(&ssd, !cor);                     // Limpa o display
            ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor); // Desenha um retângulo
            ssd1306_line(&ssd, 3, 18, 123, 18, cor);      // Desenha uma linha
            ssd1306_draw_string(&ssd, ip_str, 9, 7);      // Desenha uma string
            ssd1306_draw_string(&ssd, "BMP280", 10, 23);  // Desenha uma string
            // ssd1306_line(&ssd, 63, 32, 63, 60, cor);      // Desenha uma linha vertical
            ssd1306_draw_string(&ssd, "Temp.", 14, 38);  // Desenha uma string
            ssd1306_draw_string(&ssd, "Alt.", 14, 51);   // Desenha uma string
            ssd1306_draw_string(&ssd, str_tmp1, 73, 38); // Desenha uma string
            ssd1306_draw_string(&ssd, str_alt, 73, 51);  // Desenha uma string
            ssd1306_send_data(&ssd);                     // Atualiza o display
        }

        g_pressure = pressure / 1000.0;       // kPa
        g_temperature = temperature / 100.0;  // °C
        g_altitude = altitude;                // m
        g_aht_temperature = data.temperature; // °C
        g_aht_humidity = data.humidity;       // %

        if(temperature > temp_max || data.temperature > temp_max || temperature < temp_min || data.temperature < temp_min){
            gpio_put(ledRed, 1); // Liga o LED vermelho se a temperatura estiver acima do máximo
        }
        else{
            gpio_put(ledRed, 0); // Desliga o LED vermelho se a temperatura estiver dentro do limite
        }

        sleep_ms(300);
    }

    cyw43_arch_deinit();
    return 0;
}