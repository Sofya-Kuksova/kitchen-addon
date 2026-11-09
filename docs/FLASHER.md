# Windows-прошиватор Kitchen Timers

## Текущий пакет HW V1.3 / ESP32-P4 rev 1.x — 09.11.2026

ZIP: `local-flasher/Kitchen_Timers_HW1.3_ESP32P4_rev1x_2026-11-09.zip`.
Рядом оставлена распакованная папка с тем же именем. Пакет включает текущие
warning/DONE, mixer, инверсный прогресс, отрицательное время с компактным минусом,
стабильный размер цифр и правки нажатий/pressed кнопок.

Источник всех BIN: `build-compat-hw1.3-p4-rev1x`, готовые артефакты владельца.
Статически сверены BOARD_V13/LCD_V13/SILICON_LEGACY, ветка чипа <3.0,
CPU 360 MHz, PSRAM 200 MHz, ESP-IDF v6.0.2. App descriptor сохраняет
историческую строку версии `0.1.0-baseline`. Agent build не выполнялся.
Объекты 20 исходников проекта не старше исходников; BIN новее текущих
исходников/заголовков/config/assets. SHA-256 ELF совпадает с app descriptor BIN;
оба WAV и preview обнаружены в BIN побайтно. Это офлайн-проверка артефактов,
не подтверждение Windows/device runtime.

| Адрес | Файл | Bytes |
| --- | --- | --- |
| 0x2000 | bootloader.bin | 23152 |
| 0x8000 | partition-table.bin | 3072 |
| 0x10000 | kitchen-addon.bin | 1041936 |

App помещается в factory 0x300000 (3145728 bytes). Flash DIO / 4MB / 40m.
Manifest скопирован из этого build; изменены только относительные пути BIN.
EXE и 1029 файлов `_internal` побайтно перенесены из предыдущего LayoutB
Windows-пакета без запуска/изменений. Предыдущая firmware не копировалась.
PACKAGE.json содержит hashes BIN/ELF/runtime/исходников/config и происхождение.
ZIP CRC и побайтное совпадение всех записей с папкой проверены.
ZIP SHA-256: `6da41221380f58081ac07bb0e8ea4f45994cae9aa1b1c532e1d140c752acd8e0`.

На Windows распаковать целиком, запустить `CrowPanel_P4_fast_flasher.exe`,
Refresh → COM-порт → Flash panel. Clean install оставить выключенным для
сохранения настроек. README_RU.md внутри содержит инструкцию и ручную приёмку.
Пакет предназначен для HW1.3 / ESP32-P4 rev1.x, не rev3.x. LCD HW1.3 сохраняет
статус требующего проверки на панели. Запуск Windows, flash/monitor и аппаратная
проверка этого пакета агентом не выполнялись. `local-flasher/` игнорируется Git.

## Предыдущий пакет HW1.2 (история)

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
