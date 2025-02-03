#include "stm32f4xx.h" // Подключение заголовочного файла для STM32F4

#define MODULUS 2147483647
#define MULTIPLIER 48271
#define INCREMENT 0
#define SEED 123456789

static uint32_t lcg_random = SEED;

// Прототипы функций
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void UART_Transmit(uint8_t *data, uint16_t size);
void Generate_Frequency(uint32_t frequency);
uint32_t Get_Random_Frequency(void); // Функция для получения случайной частоты
uint32_t LCG(void); // Линейный конгруэнтный генератор

int main(void)
{
    HAL_Init(); // Инициализация библиотеки HAL
    SystemClock_Config(); // Настройка системного тактирования
    MX_GPIO_Init(); // Инициализация GPIO
    MX_USART2_UART_Init(); // Инициализация USART2

    while (1)
    {
        // Получение случайной частоты
        uint32_t frequency = Get_Random_Frequency();
        
        // Генерация частоты
        Generate_Frequency(frequency); 

        // Отправка значения частоты через UART
        UART_Transmit((uint8_t *)&frequency, sizeof(frequency));

        // Включаем светодиод
        GPIOA->ODR |= (1 << 5); // Включаем светодиод на PA5
        for (volatile int j = 0; j < 100000; j++); // Задержка

        // Выключаем светодиод
        GPIOA->ODR &= ~(1 << 5); // Выключаем светодиод на PA5
        for (volatile int j = 0; j < 100000; j++); // Задержка
    }
}

uint32_t LCG(void)
{
    // Линейный конгруэнтный генератор (LCG) для генерации случайных чисел
    lcg_random = (MULTIPLIER * lcg_random + INCREMENT) % MODULUS;
    return lcg_random;
}

uint32_t Get_Random_Frequency(void)
{
    // Получаем случайное число
    uint32_t random_number = LCG();

    // Масштабируем значение в диапазон от 300 МГц до 5.8 ГГц
    return 300000000 + (random_number % (5800000000 - 300000000));
}

void Generate_Frequency(uint32_t frequency)
{
    // Реализация генерации частоты
    // Внешний передатчик должен быть настроен для работы на заданной частоте
    // Примерно настраиваем таймер для генерации частоты
    TIM_TypeDef *TIM2 = (TIM_TypeDef *) TIM2_BASE;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Включаем тактирование таймера
    TIM2->PSC = 0; // Делитель частоты
    TIM2->ARR = (SystemCoreClock / frequency) - 1; // Установка периода
    TIM2->CR1 |= TIM_CR1_CEN; // Запуск таймера
}

void SystemClock_Config(void)
{
    // Настройка системного тактирования
    // Здесь мы настраиваем тактирование системы на 168 МГц 
    
    // Включаем кристалл HSE (High-Speed External)
    RCC->CR |= RCC_CR_HSEON; // Включаем HSE
    while (!(RCC->CR & RCC_CR_HSERDY)); // Ожидание, пока HSE станет готовым

    // Настройка делителей и множителей
    RCC->PLLCFGR = (RCC_PLLCFGR_PLLSRC_HSE | // Источник PLL - внешний кристалл HSE
                    (8 << RCC_PLLCFGR_PLLM_Pos) | // Делитель на 8
                    (336 << RCC_PLLCFGR_PLLN_Pos) | // Умножение на 336
                    (0 << RCC_PLLCFGR_PLLP_Pos) | // Делитель на 2 (PLLP)
                    (7 << RCC_PLLCFGR_PLLQ_Pos)); // Делитель на 7 (PLLCLOCK)

    RCC->CR |= RCC_CR_PLLON; // Включаем PLL
    while (!(RCC->CR & RCC_CR_PLLRDY)); // Ожидание, пока PLL станет готовым

    // Настройка системы на использование PLL как источника тактирования
    RCC->CFGR |= RCC_CFGR_SW_PLL; // Устанавливаем PLL как источник тактирования
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL); // Ожидание, пока PLL станет источником тактирования

    // Настройка тактирования AHB, APB1 и APB2
    RCC->CFGR |= (RCC_CFGR_HPRE_DIV1 | // AHB тактирование без деления
                   RCC_CFGR_PPRE1_DIV4 | // APB1 делитель на 4
                   RCC_CFGR_PPRE2_DIV2); // APB2 делитель на 2

    // Установка системной частоты
    SystemCoreClockUpdate(); // Обновляем значение частоты системы
}

static void MX_GPIO_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // Включение тактирования порта A

    // Настройка PA5 как выход для светодиода
    GPIOA->MODER |= (1 << (5 * 2)); // Установка режима вывода
    GPIOA->OTYPER &= ~(1 << 5); // Установка типа выходного напряжения (push-pull)
    GPIOA->OSPEEDR |= (3 << (5 * 2)); // Установка скорости (high speed)
    GPIOA->PUPDR &= ~(3 << (5 * 2)); // Отключение подтягивающих резисторов

    // Настройка PA2 как альтернативная функция для TX USART2
    GPIOA->MODER &= ~(3 << (2 * 2)); // Сброс режима
    GPIOA->MODER |= (2 << (2 * 2)); // Установка режима альтернативной функции
    GPIOA->AFR[0] |= (7 << (2 * 4)); // Установка альтернативной функции для PA2 (USART2_TX)
    
    // Настройка PA3 как альтернативная функция для RX USART2
    GPIOA->MODER &= ~(3 << (3 * 2)); // Сброс режима
    GPIOA->MODER |= (2 << (3 * 2)); // Установка режима альтернативной функции
    GPIOA->AFR[0] |= (7 << (3 * 4)); // Установка альтернативной функции для PA3 (USART2_RX)
}

static void MX_USART2_UART_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN; // Включение тактирования USART2
    USART2->BRR = 0x1A1; // Установка скорости передачи (например, 9600, 115200)
    USART2->CR1 |= USART_CR1_TE; // Включение передатчика
    USART2->CR1 |= USART_CR1_UE; // Включение USART
}

void UART_Transmit(uint8_t *data, uint16_t size)
{
    for (uint16_t i = 0; i < size; i++)
    {
        while (!(USART2->SR & USART_SR_TXE)); // Ожидание, пока передатчик будет готов
        USART2->DR = data[i]; // Отправка байта
    }
}


/*----------------------------------------------------------------------------------------------------------------------------------------------------------
Для реализации псевдослучайной перестройки частоты (ППРЧ) на микроконтроллере STM32 в диапазоне от 300 МГц до 5.8 ГГц с использованием внешнего передатчика.
Применен линейный конгруэнтный генератор (LCG) для генерации случайных частот и USART для передачи информации.
Она инициализирует необходимые аппаратные модули, включая GPIO и USART для передачи данных, а также настраивает таймер для генерации сигналов заданной частоты. 
В бесконечном цикле программа последовательно получает случайную частоту, 
генерирует соответствующий сигнал, передает значение частоты через интерфейс USART и управляет светодиодом, который мигает в синхронизации с процессом генерации.
Для работы с частотами выше 72 МГц на микроконтроллерах STM32F4 необходимо использовать внешние синтезаторы частоты или вспомогательные модули, 
такие как PLL (Phase-Locked Loop) или VCO (Voltage-Controlled Oscillator). В коде программы PLL настраивается для получения тактовой частоты системы 168 МГц. 
Это достигается путем настройки делителей и множителей в конфигурации PLL, где внешний кристалл (HSE) служит источником тактирования. 
С использованием PLL можно значительно увеличить тактовую частоту, 
что позволяет улучшить производительность системы и обеспечить работу различных компонентов микроконтроллера на оптимальных частотах.
Меркулов Е. В. 2025 Ⓒ
------------------------------------------------------------------------------------------------------------------------------------------------------------*/
