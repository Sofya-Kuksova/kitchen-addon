# kitchen-addon

Подготовленный кухонный компонент для существующего crow-ha-fw и отдельный
аппаратный standalone-стенд. Совместимость с хостом описана в
[BASELINE_HOST_COMPAT_CORRECTION](docs/BASELINE_HOST_COMPAT_CORRECTION.md),
актуальное продуктовое поведение — в [TIMERS](docs/TIMERS.md).

Целевой snapshot: `feature-panel-microphone@c3675d87075` (распакованный каталог,
не независимо подтверждённый полный Git commit). Хост не изменялся.

| Standalone-стек, точно закреплённый по lock хоста | Версия |
| --- | --- |
| ESP-IDF / FreeRTOS | 6.0.2 / из SDK |
| LVGL | 8.4.0 |
| esp_lvgl_port | 2.8.0~1 |
| esp_lcd_ek79007 | 1.0.4 |
| esp_lcd_touch_gt911 | 1.2.0~3 |

Для встраивания нужны только `kitchen_core`, `kitchen_platform`, `kitchen_ui`,
`kitchen_app`. Core не зависит от LVGL/SDK/железа, UI создаётся в borrowed parent,
app предоставляет `get_kitchen_app_desc()` по реальному app_manager ABI. Один общий
UI на LVGL8: пять постоянных карточек раскладки B, линейный прогресс и единая
развёрнутая карточка с редакторами времени, имени и значка. Полноэкранное представление без widget/background callbacks;
suspend сохраняет модель, скрывает UI и сохраняет ход отсчёта по дедлайнам.
Standalone сохраняет длительности, имена, значки и громкость в NVS; после reboot
настроенные слоты READY, текущие сеансы не восстанавливаются. Standalone воспроизводит один общий chime при каждом
завершении, с общей громкостью 0–100%; DONE и elapsed остаются до подтверждения.
Продуктовое поведение и ограничения: [TIMERS](docs/TIMERS.md).

В embedded-режиме звук подключается необязательными неблокирующими callbacks;
без них работает визуальный режим. Optional storage backend предоставляет хост;
без него приложение работает в RAM с уведомлением об отсутствии сохранения. Хост единолично владеет display/touch/lock/brightness/sleep,
сетью, NVS, HA sessions, audio/microphone, OTA и разметкой. Kitchen не инициализирует
эти ресурсы. HA entities/service API и голосовые функции в этот этап не входят.

Standalone сохраняет HW V1.0–V1.3, независимые silicon-ветки и полезный BSP.
Тестовые яркость/диагностика находятся вне embedded closure. Стенд использует
LVGL8 direct mode + avoid_tearing: два заимствованных LCD framebuffer без отдельного
draw buffer. PPA/DMA2D выключены; аппаратный порт живёт до reset. GUI stack 8 КиБ internal.
Перед fullscreen 1024×600 показывается чёрный экран Loading со статусом и прогрессом;
после подготовки UI он плавно исчезает. Диагностическая сцена,
переключение Fixture / Full UI и тестовые lifecycle-кнопки больше не вызываются.

- [Архитектура и lifecycle](docs/ARCHITECTURE.md)
- [Будущее подключение к хосту](docs/INTEGRATION.md)
- [Ручная сборка и подготовка ABI fixture](docs/BUILDING.md)
- [Матрица приёмки](docs/VALIDATION.md)
- [Standalone hardware profiles](docs/HARDWARE_PROFILES.md)
- [Источники, hash и версии](docs/SOURCES.md)

Состояние раскладки A (04.11.2026): **исходники и C-сценарии подготовлены;
компиляция, запуск тестов и device-проверка NOT RUN**. Команды для C-сценариев —
[tests/README](tests/README.md). Агент не выполняет сборку, flash или monitor.
Историческое состояние аппаратного baseline: **предыдущий partial-вариант HW V1.0 / P4 legacy имел Build PASS
и подтверждение тестов пользователя, затем отмечено подрагивание анимации.
Новый direct + avoid_tearing вариант ожидает ручной сборки и device-проверки**.
Приёмка относится к текущей испытанной плате; остальные экземпляры/диагонали
автоматически не квалифицируются. Embedded integration остаётся UNTESTED.
Остальные сочетания не собраны.
Автоматической регистрации нет: нужно минимальное явное подключение, которое здесь
только описано. Наличие standalone V1.3 не добавляет её поддержку BSP хоста.
Сборку/устройство проверяет владелец. `dependencies.lock` получен пользовательской
резолюцией; версии совпали с целевым стеком.
