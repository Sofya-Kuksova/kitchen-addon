# Локальный Windows-прошиватор Kitchen HW1.2

Готовый локальный пакет: `local-flasher/Kitchen_Timers_HW1.2_P4legacy/`.
Весь `local-flasher/` исключён `.gitignore`: EXE/runtime и бинарные прошивки не
попадают в репозиторий. Полная инструкция находится в `README_RU.md` пакета.

Источник прошиватора: `/Users/sonyakuks/Downloads/CrowPanelv1.2_Demo_for_P4_v1.1`.
Создана отдельная копия; исходник не изменён. EXE и `_internal` побайтно сохранены.
Встроенный Python 3.11, esptool 5.3.1; приложение читает `firmware/flasher_args.json`.
Статически просмотрены строки/константы встроенных app, firmware_package и flasher:
динамические относительные пути, обязательные bootloader/partition-table/app,
проверка esp32p4 и доступных BIN, выбор COM, необязательный Clean install.
Исполняемый код прошиватора и esptool не запускались.

## Упакованная сборка

Источник — `build-compat-hw1.2-p4legacy`, успешная сборка подтверждена пользовательским
логом. Проверен sdkconfig: BOARD_V12, LCD_LEGACY, SILICON_LEGACY. Приложение 884480 bytes,
factory partition 3145728 bytes. Строка версии CMake остаётся `0.1.0-baseline`.
Этот пакет для PCB 1.2 / P4 legacy 0.x/1.x; не для P4 rev 3.x.

| Адрес | Файл firmware | Размер |
| --- | --- | --- |
| 0x2000 | bootloader.bin | 23152 bytes |
| 0x8000 | partition-table.bin | 3072 bytes |
| 0x10000 | kitchen-addon.bin | 884480 bytes |

Manifest взят из этой сборки, изменены только пути на плоские относительные имена.
Flash settings — DIO / 4MB / 40m. Все BIN сверены SHA-256, роли/адреса manifest
сопоставлены. `PACKAGE.json` пакета содержит полные hashes и сведения о происхождении.
Исходная demo app и её manifest не входят в firmware копии.

## Ручная приёмка

На Windows перенести всю папку целиком, запустить `CrowPanel_P4_fast_flasher.exe`,
выбрать COM через Refresh и вручную нажать Flash panel. Clean install не включён
по умолчанию; он стирает всю flash, включая прежние данные. После Firmware installed
successfully проверить старт Kitchen, touch, звук, volume preview и несколько
пульсирующих таймеров на панели. Сам пакет на Windows/устройстве ещё не испытан.

Для обновления пакета все три BIN, manifest и PACKAGE.json менять совместно из
одного нового build того же HW/silicon профиля. Нельзя смешивать bootloader/table/app
разных каталогов. Прошивка standalone не является образом реального HA-хоста.
