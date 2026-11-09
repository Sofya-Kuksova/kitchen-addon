# Будущее подключение к crow-ha-fw

Статус: подготовленный компонент, **не интегрированное приложение**. Ни Downloads
snapshot, ни старый checkout crow-ha-fw не изменялись. Точка регистрации и полный
интеграционный build/device test — отдельный разрешаемый этап.

## Текущий storage ABI раскладки A от 04.11.2026

Точка configure/getter и descriptor app_manager сохранены. В конец kitchen_platform_t
добавлена storage — kitchen_storage_backend_t из kitchen_settings.h. Platform теперь
публично зависит от plain-C kitchen_core; closure по-прежнему четыре компонента.
Все использующие структуры компоненты необходимо пересобрать совместно.

Группа load/submit/poll задаётся целиком или остаётся нулевой. Контекст backend
может отличаться от platform.context и обязан жить дольше всех экземпляров.

- load(context,key,bytes,&length) вызывается в create вне GUI-lock. До ответа
  завершает все ранее принятые записи. Возвращает OK/MISSING/ERROR/PROTECTED.
  Начальная ёмкость bytes — KITCHEN_RECORD_MAX=39; не писать за её пределами.
  Oversized/newer schema возвращается PROTECTED без удаления/перезаписи.
- submit(context,key,bytes,length,&ticket) копирует snapshot до возврата и не ждёт
  I/O. Возвращает PENDING с ненулевым backend-wide uint64 ticket либо ошибку.
  Tickets не переиспользуются после destroy/new instance. Запросы ключа выполняются
  по порядку; допустимо объединить ещё не начатые запросы в последний.
- poll(context,key,ticket) не блокирует. PENDING — ожидание, OK — успешны запись
  и commit, ERROR — отказ/вытеснение. Никаких completion callbacks с app/UI pointers.

Key 0 — volume, 1–5 — слоты. Байтовый формат, точки записи, ошибки и Retry описаны
в TIMERS.md. Хост сам выбирает namespace/сервис, владеет worker и глобальным NVS;
kitchen_* не инициализирует, не стирает его и не добавляет FreeRTOS-задачи.
Standalone main/standalone_storage.c в хост не переносить.

Текущий пример integration bridge явно оставляет storage нулевым: работает RAM-режим
с Settings won't be saved. Реальное подключение хранилища и аудио хоста остаётся
отдельной интеграцией. Открытие/отмена редакторов не пишет. Все новые instances
восстанавливают только READY/EMPTY; hide/suspend сохраняют живые дедлайны.

Ниже сохранены детали первоначальной host-интеграции. Старые описания RAM-only,
кругов и ARC не задают поведение раскладки A; для неё приоритет имеют этот раздел
и TIMERS.md. Текущему UI нужны LABEL, BTN, ROLLER, TEXTAREA, KEYBOARD, SLIDER.

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

Configure/getter приложения сохранены. Текущий optional audio-контракт:

```c
esp_err_t (*submit_audio)(void *context, const kitchen_audio_batch_t *batch);
esp_err_t (*set_volume)(void *context, uint8_t percent);
```

Задать submit_audio/set_volume вместе либо оставить оба NULL. Audio callbacks
неблокирующие, вызываются под GUI-lock, не делают codec/I2S writes и не сохраняют
app/UI pointers. Context/backend живут дольше всех instances. Без audio hooks
визуальные таймеры работают, управление громкостью недоступно. Bridge явно
оставляет audio NULL, пока не найден и не проверен фактический host service.
Новые аудио-API у crow-ha-fw этим изменением не предполагаются.

`kitchen_audio_batch_t` — копируемый snapshot без указателей:

- warnings/done — bitmask ID−1 новых событий, два различных вида сигналов;
- session[5] — поколение сеанса, меняется при Cancel/OK/Start/Repeat/Extend;
- deadline_ms[5], running/finished — текущая валидность всех слотов, даже если
  события в этом пакете отсутствуют. now_ms и audio clock должны быть из одной
  монотонной шкалы миллисекунд.

App передаёт snapshot каждые 50 ms и после команд. Хост обязан сначала
синхронно обновить валидность и исключить не начатые события старых поколений,
изменённых deadlines и уже не RUNNING warnings; затем скопировать новые события.
Stop/destroy передают нулевой snapshot, отзывающий ожидающие timer events.
Проверка валидности и admission должны быть сериализованы: отмена, принятая
до старта голоса, исключает его. Уже начавшиеся сигналы могут завершиться.

Внутри одного пакета все warnings объединяются в один warning, все done — в
один DONE. Более поздний пакет не объединять с предыдущим. Хост отвечает за
смешивание разных видов и пакетов без ожидания окончания текущего clip,
ограниченные очередь/голоса, приоритет DONE, общий volume, подавление preview,
устаревание warning (не начинать после deadline), отмену очереди и голосов на
mute, диагностику переполнений и безопасный PA/DMA lifetime. Ошибки callback
не повторяются приложением. Standalone реализует эти обязанности самостоятельно;
реальная host-интеграция и совместное использование его audio/microphone требуют
отдельной проверки. Форматы файлов и уровни — SOURCES.md/TIMERS.md.

C struct/сигнатура аудио изменились: вызывающие компоненты нужно пересобрать
вместе. Прежний `.play_chime` необходимо заменить адаптером `.submit_audio`;
source compatibility старого audio binding и binary compatibility не заявляются.
Initializers без audio полей по-прежнему оставляют их NULL. Core/UI internal interfaces также пересобираются вместе с компонентами.

App-owned LVGL heartbeat (50 ms) живёт независимо от UI root и фиксирует завершения
при hide/suspend, пока экземпляр running и GUI worker обслуживает LVGL timers.
can_background=false не изменён: это не гарантия работы при выгрузке, остановленном
GUI worker или deep sleep. Для точного audio deadline в этих режимах host должен
предоставить отдельный scheduling/wake contract; standalone этого не доказывает.

WAV, I2S/PA, audio worker и его очередь входят только в standalone main. Kitchen
не трогает host hardware/codec/I2C, не добавляет audio SDK packages и не вызывает
NVS напрямую. Volume сохраняется через optional storage backend; без него только
RAM, default=40%, mute=0. События warning/DONE потребляются
один раз, ошибки очереди логируются без повторного проигрывания. DONE/elapsed сохраняются
до Acknowledge. Публичный getter/descriptor app_manager сохранён; bridge обновлён
для явного отсутствия аудиосервиса.

LVGL должен включать LABEL, BTN, ROLLER, TEXTAREA, KEYBOARD, SLIDER и текстовые
bitmap-шрифты. LV_FONT_DEFAULT должен содержать стандартные LVGL symbols (в том числе SETTINGS).
Speaker рисуется прежними примитивами, No icon использует Block из kitchen_symbols_48.

### Optional volume preview hook

В конец kitchen_platform_t добавлен play_volume_preview(void *context), возвращающий
esp_err_t. Callback необязателен независимо от пары submit_audio/set_volume; без него
громкость меняется без звука preview. Он неблокирующий, не сохраняет UI/app pointers.
KITCHEN_PREVIEW_VOLUME обрабатывается kitchen_app, не изменяет модель core и не
использует timer completion events. Struct ABI снова требует совместной пересборки.
