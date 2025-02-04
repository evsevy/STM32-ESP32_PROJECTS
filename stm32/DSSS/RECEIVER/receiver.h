// Copyright (c) 2025 Syn_Soft

#ifndef RECEIVER_H
#define RECEIVER_H

#include "stm32f4xx_hal.h"

#define PN_SEQUENCE_LENGTH 11  // Длина последовательности псевдослучайных чисел
#define SAMPLE_RATE 1000       // Частота выборки
#define SIGNAL_LENGTH 100      // Длина сигнала
#define UART_BUFFER_SIZE 256   // Размер буфера для UART
#define SYNC_BYTE 0xAA         // Сигнал синхронизации
#define ACK_BYTE 0x55          // Сигнал подтверждения

// Функция инициализации UART
void init_uart(void);

// Функция для логирования сообщений
void log_message(const char* message);

// Функция для приема сигнала
void receive_signal(void);

#endif // RECEIVER_H