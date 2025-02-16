/************************************************************************************************************
*           Тестовая программа написана для работы с микроконтроллером, серии ESP32,                        *
*        с использованием библиотек Wire для I2C и WiFi для подключения к беспроводной сети                 *
* Основная задача программы — считывать данные с TF-Luna датчика расстояния, с расчетом поправки погрешности*
*              с устройства по I2C и отправлять эти данные по wi-fi на удалённый сервер.                    *
*           Программист - разработчик Меркулов Е. В. Syn_Soft Ltd. © 2025                                   *
*************************************************************************************************************

-------------------------------------------------------------------------------------------------------------
                                                                                                   ,----, 
                                    ,--.                             ,----..                     ,/   .`| 
  .--.--.                         ,--.'|              .--.--.       /   /   \       ,---,.     ,`   .'  : 
 /  /    '.          ,---,    ,--,:  : |             /  /    '.    /   .     :    ,'  .' |   ;    ;     / 
|  :  /`. /         /_ ./| ,`--.'`|  ' :     ,---,. |  :  /`. /   .   /   ;.  \ ,---.'   | .'___,/    ,'  
;  |  |--`    ,---, |  ' : |   :  :  | |   ,'  .' | ;  |  |--`   .   ;   /  ` ; |   |   .' |    :     |   
|  :  ;_     /___/ \.  : | :   |   \ | : ,---.'   , |  :  ;_     ;   |  ; \ ; | :   :  :   ;    |.';  ;   
 \  \    `.   .  \  \ ,' ' |   : '  '; | |   |    |  \  \    `.  |   :  | ; | ' :   |  |-, `----'  |  |   
  `----.   \   \  ;  `  ,' '   ' ;.    ; :   :  .'    `----.   \ .   |  ' ' ' : |   :  ;/|     '   :  ;   
  __ \  \  |    \  \    '  |   | | \   | :   |.'      __ \  \  | '   ;  \; /  | |   |   .'     |   |  '   
 /  /`--'  /     '  \   |  '   : |  ; .' `---'       /  /`--'  /  \   \  ',  /  '   :  '       '   :  |   
'--'.     /       \  ;  ;  |   | '`--'              '--'.     /    ;   :    /   |   |  |       ;   |.'    
  `--'---'         :  \  \ '   : |                    `--'---'      \   \ .'    |   :  \       '---'      
                    \  ' ; ;   |.'                                   `---`      |   | ,'                  
                     `--`  '---'                                                `----'                    
                                                           
------------------------------------------------------------------------------------------------------------
*/

#include <Wire.h>         // Подключаем библиотеку для I2C
#include <WiFi.h>         // Подключаем библиотеку для ESP32
#include <WebServer.h>    // Подключаем библиотеку для веб-сервера

#define TF_LUNA_ADDRESS 0x10 // Адрес I2C датчика TF Luna

// Настройки Wi-Fi
const char* ssid = "Redmi";        // Замените на ваше имя Wi-Fi
const char* password = "557effb42762"; // Замените на ваш пароль Wi-Fi

WebServer server(80); // Создаем веб-сервер на порту 80

String lastResponse = ""; // Хранит последний ответ (расстояние или ошибка)

const String htmlPageTemplate = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>TOF-MEASURE</title>
  <style>
    body {
      background-color: #add8e6; /* Голубой фон */
      text-align: center; /* Центрирование текста */
      font-family: Arial, sans-serif; /* Шрифт */
      font-size: 24px; /* Размер шрифта */
    }
    h1 {
      font-size: 90px; /* Увеличенный шрифт заголовка втрое */
      color: red; /* Красный цвет заголовка */
    }
    p {
      font-size: 66px; /* Увеличенный шрифт абзаца */
      color: blue; /* Синий цвет абзаца */
    }
    #distance {
      font-size: 46px; /* Размер шрифта для расстояния */
      color: blue; /* Синий цвет для расстояния */
      border: 4px solid green; /* Зеленая рамка вокруг расстояния */
      display: inline-block; /* Чтобы рамка обрамляла текст */
      padding: 20px; /* Отступ внутри рамки */
    }
  </style>
  <script>
    // Обновление данных каждый 3 секунды
    setInterval(function() {
      fetch('/update').then(response => response.text()).then(data => {
        document.getElementById('distance').innerHTML = data;
      });
    }, 3000);
  </script>
</head>
<body>
  <h1>TOF-MEASURE</h1> <!-- Заголовок страницы -->  
  <p>Real distance:</p>
  <p id="distance">{{distance}}</p>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200); // Инициализация последовательного порта
    Wire.begin(); // Инициализация I2C

    // Подключаемся к Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(300);
        Serial.println("Подключение к Wi-Fi...");
    }
    Serial.println("Подключено к Wi-Fi");
    Serial.print("IP-адрес: ");
    Serial.println(WiFi.localIP());

    // Обработчик главной страницы
    server.on("/", HTTP_GET, handleRoot);
    // Обработчик обновления данных
    server.on("/update", HTTP_GET, handleUpdate);

    server.begin(); // Запуск веб-сервера
}

void loop() {
    // Обновляем данные о расстоянии каждые 3 секунды
    static unsigned long lastUpdateTime = 0;
    if (millis() - lastUpdateTime > 2000) {
        updateDistance();
        lastUpdateTime = millis();
    }

    server.handleClient(); // Обработка клиентских запросов
}

void handleRoot() {
    // Отправляем HTML страницу
    String htmlContent = htmlPageTemplate;
    htmlContent.replace("{{distance}}", lastResponse); // Вставляем актуальное расстояние или ошибку
    server.send(200, "text/html", htmlContent);
}

void handleUpdate() {
    // Обновляем данные и отправляем их в ответ
    updateDistance();
    server.send(200, "text/plain", lastResponse); // Отправляем обновленное расстояние
}

void updateDistance() {
    int distanceCm = readDistance(); // Считываем расстояние в сантиметрах
    if (distanceCm >= 0) {
        float distanceM = distanceCm / 100.0; // Переводим сантиметры в метры
        lastResponse = String(distanceM, 2) + " м"; // Обновляем только расстояние и используем метры с 2 знаками после запятой
        Serial.println("Расстояние: " + lastResponse); // Выводим расстояние в Serial Monitor
    } else {
        lastResponse = "Ошибка: Не удалось получить данные с датчика";
        Serial.println(lastResponse); // Выводим ошибку в Serial Monitor
    }
}

int readDistance() {
    Wire.beginTransmission(TF_LUNA_ADDRESS); // Начало передачи
    Wire.write(0x00); // Команда для получения расстояния
    Wire.endTransmission(); // Завершение передачи

    Wire.requestFrom(TF_LUNA_ADDRESS, 2); // Запрос 2 байта данных

    if (Wire.available() == 2) { // Проверка, получены ли 2 байта
        uint8_t lowByte = Wire.read(); // Чтение младшего байта
        uint8_t highByte = Wire.read(); // Чтение старшего байта

        int distance = (highByte << 8) | lowByte; // Формирование 16-битного значения расстояния
        return distance; // Возврат расстояния
    } else {
        return -1; // Возврат ошибки, если не удалось получить данные
    }
}
