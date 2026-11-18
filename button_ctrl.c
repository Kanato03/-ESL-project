#include "button_ctrl.h"
#include "app_board.h"
#include "app_utils.h"
#include "led_ctrl.h"
#include "timer_ctrl.h"
#include "nrf_gpio.h"
#include "nrfx_gpiote.h"

static volatile uint32_t m_last_press_ms = 0;
static volatile uint32_t m_first_click_ms = 0;
static volatile bool     m_waiting_for_second = false;

static void button_event_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    // Проверяем, что кнопка действительно нажата (активный уровень 0)
    if (nrf_gpio_pin_read(BUTTON_PIN) != BUTTON_ACTIVE_LEVEL) return;

    uint32_t now = timer_ctrl_ms();

    // 1. Антидребезг (игнорируем отскоки контактов)
    if ((uint32_t)(now - m_last_press_ms) < DEBOUNCE_MS) return;
    m_last_press_ms = now;

    // 2. Логика двойного клика
    if (m_waiting_for_second && (uint32_t)(now - m_first_click_ms) <= DOUBLE_CLICK_MS)
    {
        // Мы ждали второй клик, и он пришел вовремя!
        led_ctrl_toggle();
        m_waiting_for_second = false; // Сбрасываем ожидание
    }
    else
    {
        // Это первый клик (или прошло больше 500 мс, и это новый первый клик)
        m_waiting_for_second = true;
        m_first_click_ms = now;
    }
}

void button_ctrl_init(void)
{
    app_error_check(nrfx_gpiote_init());

    // Прерывание только по нажатию (High-to-Low)
    nrfx_gpiote_in_config_t config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    config.pull = NRF_GPIO_PIN_PULLUP;

    app_error_check(nrfx_gpiote_in_init(BUTTON_PIN, &config, button_event_handler));
    nrfx_gpiote_in_event_enable(BUTTON_PIN, true);

    // ВАЖНО: Инициализируем время последнего нажатия, чтобы первый клик после включения не игнорировался
    m_last_press_ms = timer_ctrl_ms();
}