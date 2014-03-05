#include "nrf_pwm.h"
#include "nrf.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"
#if(USE_WITH_SOFTDEVICE == 1)
#include "nrf_sdm.h"
#endif

static uint32_t pwm_max_value, pwm_next_value[PWM_MAX_CHANNELS], pwm_io_ch[PWM_MAX_CHANNELS], pwm_running[PWM_MAX_CHANNELS];
static uint8_t pwm_gpiote_channel[3];
static uint32_t pwm_num_channels;

int32_t ppi_enable_channel(uint32_t ch_num, volatile uint32_t *event_ptr, volatile uint32_t *task_ptr)
{
    if(ch_num >= 16)
    {
        return -1;
    }
    else
    {
#if(USE_WITH_SOFTDEVICE == 1)
        sd_ppi_channel_assign(ch_num, event_ptr, task_ptr);
        sd_ppi_channel_enable_set(1 << ch_num);
#else
        // Otherwise we configure the channel and return the channel number
        NRF_PPI->CH[ch_num].EEP = (uint32_t)event_ptr;
        NRF_PPI->CH[ch_num].TEP = (uint32_t)task_ptr;    
        NRF_PPI->CHENSET = (1 << ch_num);   
#endif
        return ch_num;
    }
}

uint32_t nrf_pwm_init(nrf_pwm_config_t *config)
{
    if(config->num_channels < 1 || config->num_channels > 3) return 0xFFFFFFFF;
    
    switch(config->mode)
    {
        case PWM_MODE_LED_100:   // 0-100 resolution, 156 Hz PWM frequency, 32 kHz timer frequency (prescaler 9)
            PWM_TIMER->PRESCALER = 9; /* Prescaler 4 results in 1 tick == 1 microsecond */
            pwm_max_value = 100;
            break;
        case PWM_MODE_LED_255:   // 8-bit resolution, 122 Hz PWM frequency, 65 kHz timer frequency (prescaler 8)
            PWM_TIMER->PRESCALER = 8;
            pwm_max_value = 255;        
            break;
        case PWM_MODE_LED_1000:  // 0-1000 resolution, 250 Hz PWM frequency, 500 kHz timer frequency (prescaler 5)
            PWM_TIMER->PRESCALER = 5;
            pwm_max_value = 1000;
            break;
        case PWM_MODE_MTR_100:   // 0-100 resolution, 20 kHz PWM frequency, 4MHz timer frequency (prescaler 2)
            PWM_TIMER->PRESCALER = 2;
            pwm_max_value = 100;
            break;
        case PWM_MODE_MTR_255:    // 8-bit resolution, 31 kHz PWM frequency, 16MHz timer frequency (prescaler 0)	
            PWM_TIMER->PRESCALER = 0;
            pwm_max_value = 255;
            break;
        default:
            return 0xFFFFFFFF;
    }
    pwm_num_channels = config->num_channels;
    for(int i = 0; i < pwm_num_channels; i++)
    {
        pwm_io_ch[i] = (uint32_t)config->gpio_num[i];
        nrf_gpio_cfg_output(pwm_io_ch[i]);
        pwm_running[i] = 0;       
        pwm_gpiote_channel[i] = config->gpiote_channel[i];        
    }
    PWM_TIMER->TASKS_CLEAR = 1;
	PWM_TIMER->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
    PWM_TIMER->CC[3] = pwm_max_value*2;
	PWM_TIMER->MODE = TIMER_MODE_MODE_Timer;
    PWM_TIMER->SHORTS = TIMER_SHORTS_COMPARE3_CLEAR_Msk;
    PWM_TIMER->EVENTS_COMPARE[0] = PWM_TIMER->EVENTS_COMPARE[1] = PWM_TIMER->EVENTS_COMPARE[2] = PWM_TIMER->EVENTS_COMPARE[3] = 0;     

    for(int i = 0; i < pwm_num_channels; i++)
    {
        ppi_enable_channel(config->ppi_channel[i*2],  &PWM_TIMER->EVENTS_COMPARE[i], &NRF_GPIOTE->TASKS_OUT[pwm_gpiote_channel[i]]);
        ppi_enable_channel(config->ppi_channel[i*2+1],&PWM_TIMER->EVENTS_COMPARE[3], &NRF_GPIOTE->TASKS_OUT[pwm_gpiote_channel[i]]);        
    }
#if(USE_WITH_SOFTDEVICE == 1)
    sd_nvic_SetPriority(PWM_IRQn, 3);
    sd_nvic_EnableIRQ(PWM_IRQn);
#else
    NVIC_SetPriority(PWM_IRQn, 3);
    NVIC_EnableIRQ(PWM_IRQn);
#endif
    PWM_TIMER->TASKS_START = 1;
    return 0;
}

void nrf_pwm_set_value(uint32_t pwm_channel, uint32_t pwm_value)
{
    pwm_next_value[pwm_channel] = pwm_value;
    PWM_TIMER->EVENTS_COMPARE[3] = 0;
    PWM_TIMER->SHORTS = TIMER_SHORTS_COMPARE3_CLEAR_Msk | TIMER_SHORTS_COMPARE3_STOP_Msk;
    if((PWM_TIMER->INTENSET & TIMER_INTENSET_COMPARE3_Msk) == 0)
    {
        PWM_TIMER->TASKS_STOP = 1;
        PWM_TIMER->INTENSET = TIMER_INTENSET_COMPARE3_Msk;  
    }
    PWM_TIMER->TASKS_START = 1;
}

void PWM_IRQHandler(void)
{
    static uint32_t i;
    PWM_TIMER->EVENTS_COMPARE[3] = 0;
    PWM_TIMER->INTENCLR = 0xFFFFFFFF;
    for(i = 0; i < pwm_num_channels; i++)
    {
        if(pwm_next_value[i] == 0)
        {
            nrf_gpiote_unconfig(pwm_gpiote_channel[i]);
            nrf_gpio_pin_write(pwm_io_ch[i], 0);
            pwm_running[i] = 0;
        }
        else if (pwm_next_value[i] >= pwm_max_value)
        {
            nrf_gpiote_unconfig(pwm_gpiote_channel[i]);
            nrf_gpio_pin_write(pwm_io_ch[i], 1); 
            pwm_running[i] = 0;
        }
        else
        {
            PWM_TIMER->CC[i] = pwm_next_value[i] * 2;
            if(!pwm_running[i])
            {
                nrf_gpiote_task_config(pwm_gpiote_channel[i], pwm_io_ch[i], NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);  
                pwm_running[i] = 1;
            }
        }
    }
    PWM_TIMER->SHORTS = TIMER_SHORTS_COMPARE3_CLEAR_Msk;
    PWM_TIMER->TASKS_START = 1;
}
