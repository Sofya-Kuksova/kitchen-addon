# Аппаратные профили standalone

Эти настройки не входят в embedded kitchen. Встроенное приложение целиком использует
выбор PCB/silicon и BSP хоста; GPIO/DSI/PSRAM здесь им не навязываются.
Целевой standalone SDK — IDF 6.0.2, LVGL 8.4.0 и esp_lvgl_port 2.8.0~1.

## Три независимые оси

Board defaults выбирают PCB; LCD defaults задают модель таймингов; silicon defaults
выбирают несовместимую ветку SDK и параметры CPU/памяти. Номер PCB не определяет
chip revision. Требуются все три выбора; молчаливого default PCB нет.

| PCB | LCD defaults | SDIO D0/D1/D2/D3 | Слот A/B | Статус исходных сведений |
| --- | --- | --- | --- | --- |
| V1.0 | lcd-legacy | 14/15/16/17 | 53/54 | WarehouseFlow + Elecrow |
| V1.1 | lcd-legacy | 17/16/15/14 | 53/54 | LCD/touch/BL/SDIO: рабочий Leader; слот: Elecrow |
| V1.2 | lcd-legacy | 17/16/15/14 | 27/28 | Leader; слот: WarehouseFlow + Elecrow |
| V1.3 | lcd-v1.3 | NC/NC/NC/NC | 27/28, candidate | LCD: проект коллеги; SDIO неизвестен |

Для V1.0–V1.2 SDIO CLK18/CMD19, C6 reset32. Для V1.3 все три также NC до подтверждения.
Эти интерфейсы не инициализируются; нет ESP-Hosted и API их включения.
`sdio_known=false` и `wireless_confirmed=false` явно сохраняют неопределённость V1.3.
Поля pin mapping не означают доступность функции в baseline.

[Elecrow Product version number](https://github.com/Elecrow-RD/CrowPanel-Advanced-7inch-ESP32-P4-HMI-AI-Display-1024x600-IPS-Touch-Screen#2-product-version-number)
описывает перестановки SDIO с V1.1 и беспроводного слота с V1.2. Нижняя GPIO-таблица
README содержит старые значения; приоритет отдан описанию изменений и рабочим
локальным источникам. Источник относится к 7″, не квалифицирует V1.3/другие диагонали.

Пользователь подтвердил работу настроек Leader на V1.1/V1.2. Дополнительная схема
не требуется для переноса этих параметров. Новая комбинация SDK/драйверов остаётся
отдельным предметом проверки. Точные файлы, commits и SHA-256: [SOURCES](SOURCES.md).

## LCD, питание, touch

| Параметр | Legacy V1.0/V1.1/V1.2 | Candidate V1.3 9″/10.1″ |
| --- | --- | --- |
| Controller / format | EK79007 / RGB565 | EK79007 / RGB565 |
| Размер | 1024×600 | 1024×600 |
| DSI | 2 lanes × 1000 Mbps | 2 lanes × 900 Mbps |
| DPI clock | 51 MHz | 51 MHz |
| HBP/HPW/HFP | 160/70/160 | 90/70/160 |
| VBP/VPW/VFP | 23/10/12 | 13/10/21 |
| LCD reset | NC, software reset драйвера | GPIO41, active low |
| Backlight | GPIO31 | GPIO31, candidate |
| I2C SDA/SCL | 45/46 | 45/46, candidate |
| GT911 reset/INT | 40/42 | 40/42, candidate |
| PHY LDO | channel3, 2500 mV | channel3, 2500 mV, candidate |

Legacy LCD/touch/BL проверены по WarehouseFlow/Leader; V1.1/V1.2 подтверждены
пользователем на Leader. LDO взят из BSP-примеров. V1.3 timing/reset — из
`bsp_display.c/.h` коллеги, в том числе HBP=160−70 и VBP=23−10.
Происхождение проекта коллеги не доказывает точное соответствие работающему бинарнику.

I2C0, 400 кГц, internal pullups, GT911 без swap/mirror. Адрес по умолчанию 0x5D;
Kconfig `KITCHEN_TOUCH_ADDRESS_14` позволяет 0x14. После изменения нужен новый
конфигурационный вариант. Драйвер 1.2.1 сам формирует reset/INT sequence: INT low
для 0x5D, high для 0x14, затем INT переводится во вход. Произвольного перебора GPIO
или автоматического переключения адресов после ошибки нет.

PWM: 30 кГц, LEDC low-speed timer1/channel0, 10 бит, активный высокий уровень,
`duty=floor(1023×percent/100)`. 0% = duty0, 100% = duty1023. Начальные 70% включаются
после первого UI refresh, 0% восстанавливаются до 70% через 5 секунд.
Полярность/диапазон для V1.3 остаются кандидатами до device test.

## Silicon и память

| Defaults | IDF branch | Диапазон SDK | CPU / PSRAM | PHY reference |
| --- | --- | --- | --- | --- |
| silicon-legacy | SELECTS_REV_LESS_V3=y | min0.0…max1.99 | 360 / 200 MHz | DEFAULT_LEGACY |
| silicon-v3 | SELECTS_REV_LESS_V3=n | min3.0…max3.99 | 400 / 250 MHz | DEFAULT |

0.x не исключены автоматически. Границы отражают Kconfig IDF, а не испытанные
ревизии; диапазон 2.x этими ветками не поддерживается. Рабочих логов нового baseline
нет. Имена PHY constants сверены с IDF6: старый alias `MIPI_DSI_PHY_CLK_SRC_DEFAULT`
всегда обозначает legacy, поэтому использовать его для v3 нельзя.

В обеих ветках PSRAM HEX 16-line, LDO2=1.8V; flash LDO1=3.3V, DIO 80 МГц,
image header 4 МиБ. HEX/напряжения основаны на примерах и должны соответствовать
физическому модулю. 250 МГц включены только в v3 по выбранному пользователем варианту;
это не подтверждение устойчивости любой платы. PSRAM memtest включён, XiP выключен.

Два framebuffer занимают 2 457 600 байт PSRAM; отдельно нужны объекты UI, heap,
драйверные структуры и внутренний GUI stack 8192 байта. Отдельный partial draw buffer больше не выделяется. Ошибка выделения останавливает
запуск; дополнительные framebuffer или увеличение stack автоматически не применяются.


## Изменение графического режима после host correction

Прежний LVGL9 adapter baseline снят с цели. Текущий standalone использует
LVGL8 esp_lvgl_port: direct_mode=true, avoid_tearing=true, double_buffer=true,
buffer_size=1024×600 пикселей. Порт заимствует два кадра BSP (2 457 600 байт суммарно),
не выделяя отдельные draw buffers. Full refresh/rotation/PPA/LCD DMA2D выключены.

Этот вариант заменяет исходный partial-режим для сравнения плавности и синхронизации.
Его build/device приёмка пока открыта. CPU/PSRAM, LCD timing и GUI scheduling не менялись.
Аппаратный порт живёт до reset; open/close циклы относятся к kitchen instance/UI.
Ограничения vendor error/teardown paths описаны в SOURCES/ARCHITECTURE.
