# Resource allocation

## Peripherals
ID | Name               | Used by
--- | --- | ---
2  | UART0              | cs_Serial
3  | SPI0 / TWI0        | ?
4  | SPI1/TW1/SPIS1     | ?
   |                    | 
6  | GPIOTE             | ?
7  | ADC                | cs_ADC
8  | TIMER0             | Softdevice - timeslot API
9  | TIMER1             | cs_ADC?
10 | TIMER2             | cs_PWM
11 | RTC0               | timer_control (scheduler?)
12 | TEMP               | Softdevice
13 | RNG                | Softdevice
14 | ECB                | Softdevice
15 | CCM                | Softdevice
15 | AAR                | Softdevice
16 | WDT                | ?
17 | RTC1               | app_timer (mesh)
18 | QDEC               | mesh
19 | LPCOMP             | cs_LPComp
20 | SWI0               | ?
21 | Radio Notification | Softdevice
22 | SoC Events         | Softdevice
23 | SWI3               | Softdevice
24 | SWI4               | Softdevice
25 | SWI5               | Softdevice
   |                    | 
30 | NVMC               | Softdevice
31 | PPI                | ?

## PPI
Channel | Used by
--- | ---
0  | pwm?
1  | pwm?
2  | pwm?
3  | pwm?
4  | pwm?
5  | pwm?
6  | ?
7  | ?
8  | mesh
9  | mesh
10 | mesh
11 | mesh
12 | mesh
13 | mesh?
14 | Softdevice
15 | Softdevice

## PPI groups
Group | Used by
--- | ---
0 | pwm?
1 | ?
2 | Softdevice
3 | Softdevice

## GPIOTE
Channel | Used by
--- | ---
0 | pwm?
1 | pwm?
2 | cs_ADC
3 | ?