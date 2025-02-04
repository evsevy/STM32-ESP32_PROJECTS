// Copyright (c) 2025 Syn_Soft

#include "transmitter.h"
#include <math.h>
#include <string.h>

const uint8_t PN_Sequence[PN_SEQUENCE_LENGTH] = {1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0};

UART_HandleTypeDef huart2;
uint8_t uart_buffer[UART_BUFFER_SIZE];

void init_uart(void) {
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3; // TX и RX
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

void log_message(const char* message) {
    HAL_UART_Transmit(&huart2, (uint8_t*)message, strlen(message), HAL_MAX_DELAY);
}

void transmit_signal(float *input_signal) {
    HAL_UART_Transmit(&huart2, &SYNC_BYTE, sizeof(SYNC_BYTE), HAL_MAX_DELAY);
    
    for (int i = 0; i < SIGNAL_LENGTH; i++) {
        float modulated_signal = input_signal[i] * PN_Sequence[i % PN_SEQUENCE_LENGTH];
        memcpy(uart_buffer + (i * sizeof(float)), &modulated_signal, sizeof(modulated_signal));
    }
    
    HAL_UART_Transmit(&huart2, uart_buffer, SIGNAL_LENGTH * sizeof(float), HAL_MAX_DELAY);
    log_message("Signal transmitted.\n");

    uint8_t ack_byte;
    HAL_UART_Receive(&huart2, &ack_byte, sizeof(ack_byte), 1000);
    if (ack_byte == ACK_BYTE) {
        log_message("Acknowledgment received.\n");
    } else {
        log_message("No acknowledgment. Resending signal.\n");
        transmit_signal(input_signal);
    }
}

int main(void) {
    HAL_Init();
    init_uart();

    float input_signal[SIGNAL_LENGTH];
    for (int i = 0; i < SIGNAL_LENGTH; i++) {
        input_signal[i] = sin(2 * M_PI * 0.1 * i);
    }

    transmit_signal(input_signal);
    while (1) {
        // Основной цикл
    }
}