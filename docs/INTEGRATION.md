# Будущее подключение к crow-ha-fw

Статус: подготовленный компонент, **не интегрированное приложение**. Ни Downloads
snapshot, ни старый checkout crow-ha-fw не изменялись. Точка регистрации и полный
интеграционный build/device test — отдельный разрешаемый этап.

## Что переносить

Только каталоги `components/kitchen_core`, `kitchen_platform`, `kitchen_ui`,
`kitchen_app`. В реальном host используется его app_manager и app_base.h;
`standalone/app_manager` и `.host-contract` переносить нельзя. Не переносить
main, baseline_ui, display_port, crowpanel_board/BSP, sdkconfig, partitions,
root CMake, standalone manifest или lock. Они принадлежат отдельному стенду.

Embedded UI manifest ограничивает LVGL диапазоном >=8.4.0,<8.5 и допускает IDF 6.0.2;
не требует migration, esp_lvgl_adapter или второго esp_lvgl_port. Итоговые версии
всё равно проверяются по будущей резолюции пользователя. Хостовый lock остаётся его.
Audio codecs в зависимости kitchen не добавлены.

## Минимальная явная точка подключения (пока не применена)

Два файла `integration/kitchen_host_bridge.c/.h` подготовлены для включения в main
будущим интегратором. Они используют существующий BSP GUI-lock и public intents;
не запускают дисплей/порт/сервисы. Скопировать их рядом с host main.cpp.

В **существующую** регистрацию main component добавить source `kitchen_host_bridge.c`
и зависимости `kitchen_app`, `app_manager`, `esp_timer`,
`espressif__esp32_p4_function_ev_board`, сохранив остальные source/dependencies.
Пример фрагмента, не замена всего host CMakeLists:

```cmake
# Add to existing idf_component_register lists:
# SRCS ... "kitchen_host_bridge.c"
# PRIV_REQUIRES ... kitchen_app app_manager esp_timer espressif__esp32_p4_function_ev_board
```

После существующих GUI/BSP/app_manager init, рядом с явным списком register и до
launch_all:

```cpp
#include "kitchen_app.h"
#include "kitchen_host_bridge.h"

ESP_ERROR_CHECK(kitchen_host_configure());
ESP_ERROR_CHECK(app_manager_register(get_kitchen_app_desc(), NULL));
```

Getter имеет C linkage. Descriptor name="kitchen", fullscreen=true, widget=false,
background=false, suspend=true; launch_policy NONE. Регистрация сама не добавляет
widget или кнопку меню. Чтобы открыть fullscreen, существующий навигационный UI
должен выдать публичный intent, например вне GUI blocking-переходов:

```cpp
app_id_t id = app_manager_find_app_id_by_name("kitchen");
if (id != APP_ID_INVALID) {
    app_manager_send_navigation_event(id, APP_NAV_INTENT_OPEN_FULLSCREEN, 0);
}
```

Нужен явный способ вызвать этот intent в будущем интеграционном этапе; скрытых
static constructors, подмен getter, linker hooks и auto-discovery нет.

## Контракты и ограничения

- Bind один раз до создания instances; operations/context живут дольше всех instances.
  Переконфигурация при живом instance запрещена.
- Lifecycle callbacks вызываются вне GUI-lock; create_content, UI events и delete
  callbacks — внутри него. Bridge нормализует bool lock, ms и esp_err_t. Kitchen
  не использует приватные jobs_queue или BSP handles в публичном UI-контракте.
- Back использует APP_NAV_INTENT_BACK_TO_HOMESCREEN с неблокирующей постановкой.
  Global touch/buttons, HA routing, sleep и screens остаются у host manager.
- Host владеет parent; kitchen удаляет только свой root. Внешнее удаление parent
  безопасно для последующего destroy kitchen. Возобновление view после такого
  удаления требует нового валидного create_content от владельца.
- Никаких HA entity calls/login/token copies/WebSocket, NVS init, audio capture,
  микрофонных permissions или изменения глобальной brightness/settings/OTA.
- Hardware profile выбирает только хост. Standalone V1.3 не квалифицирует BSP хоста;
  несовместимость BSP не исправляется kitchen переинициализацией LCD.

## Отдельная будущая приёмка

Пользовательский host build: сравнить реальные signed images до/после, вычислить
прирост и остаток каждого app slot 0x680000. Не менять storage/sec_storage/nvs_key,
размеры слотов, signing/encryption и keys. Исторические проценты README хоста не
используются как текущие измерения.

На устройстве: HA↔kitchen, Back/minimize, sleep/wake, изменение Wi-Fi,
одновременные разрешённые HA video/audio/microphone sessions, heap/stack/flash budget,
отсутствие влияния на host brightness/settings/OTA. Fixture PASS этого не доказывает.
При требовании нулевых изменений даже registration статус останется component-ready.

## Таймеры, громкость и опциональный audio backend

Configure/getter приложения сохранены. В конец kitchen_platform_t добавлены:

```c
esp_err_t (*play_chime)(void *context);
esp_err_t (*set_volume)(void *context, uint8_t percent);
```

Задать оба callbacks или оставить оба NULL. Они обязаны возвращаться без ожидания:
play_chime ставит scalar request в собственную очередь host audio, set_volume меняет
целевой gain/публикует запрос. Запрещены блокирующие playback/codec writes под GUI-lock.
Context/backend живут дольше kitchen instances. Без audio hooks громкость UI недоступна,
визуальные таймеры работают. Пример не подменяет несуществующим API сервисы хоста:
подключение его реальной очереди и принятие политики громкости — отдельная интеграция.

C struct увеличился: пересобрать вызывающие/использующие компоненты. Старые designated
initializers компилируются с NULL новыми полями, двоичной совместимости старых объектов
не заявляется. Core/UI internal interfaces также пересобираются вместе с компонентами.

App-owned LVGL heartbeat (50 ms) живёт независимо от UI root и фиксирует завершения
при hide/suspend, пока экземпляр running и GUI worker обслуживает LVGL timers.
can_background=false не изменён: это не гарантия работы при выгрузке, остановленном
GUI worker или deep sleep. Для точного audio deadline в этих режимах host должен
предоставить отдельный scheduling/wake contract; standalone этого не доказывает.

WAV, I2S/PA, audio worker и его очередь входят только в standalone main. Kitchen
не трогает host hardware/codec/I2C, не добавляет audio SDK packages и не пишет NVS.
Volume хранится в RAM экземпляра, default=40%, mute=0. События завершения потребляются
один раз, ошибки очереди логируются без повторного проигрывания. DONE/elapsed сохраняются
до Acknowledge. Публичный getter/descriptor app_manager и host bridge не менялись.

LVGL должен включать ARC, LABEL, BTN, ROLLER, TEXTAREA, KEYBOARD, SLIDER и текстовые
bitmap-шрифты. LV_SYMBOL_MUTE/VOLUME_MID/VOLUME_MAX отображаются через существующий fallback
на LV_FONT_DEFAULT, который должен содержать стандартные LVGL symbols.

### Optional volume preview hook

В конец kitchen_platform_t добавлен play_volume_preview(void *context), возвращающий
esp_err_t. Callback необязателен независимо от пары play_chime/set_volume; без него
громкость меняется без звука preview. Он неблокирующий, не сохраняет UI/app pointers.
KITCHEN_PREVIEW_VOLUME обрабатывается kitchen_app, не изменяет модель core и не
использует timer completion events. Struct ABI снова требует совместной пересборки.
