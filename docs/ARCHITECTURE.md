# Архитектура совместимого модуля

## Dependency closure

```text
embedded: kitchen_app -> app_manager (host ABI)
             |-> kitchen_platform -> esp_common
             |-> kitchen_core (plain C, no SDK/LVGL)
             `-> kitchen_ui -> kitchen_core + LVGL8 + esp_common

standalone main -> standalone_runner -> kitchen_app
             |-> display_port -> crowpanel_bsp -> crowpanel_board
             `-> standalone/app_manager (header-only ABI fixture, not host manager)

future host main -> integration bridge -> existing BSP lock + app_manager intents
```

Ни один `kitchen_*` компонент не включает собственный BSP, display_port,
LVGL init/task, NVS, Wi-Fi, HA/audio/settings/sleep services или standalone main.
Файлы `integration/` не входят в standalone build и не импортируются автоматически.
Не существует параллельной LVGL9 UI-реализации.

Core: фиксированная модель до 20 таймеров и команды со стабильными ID.
Running хранит monotonic deadline, paused — точный остаток, done — исходный дедлайн.
Порядки active/saved отдельны от слотов модели. Стартовая длительность не меняется
при корректировке текущего остатка. Состояние и все команды описаны в [TIMERS](TIMERS.md).
Модель принадлежит app instance; UI callbacks и tick работают под существующим GUI-lock.
LVGL user_data содержит только ссылки на владельца и bindings, живущие до удаления root.
Embedded-модуль не создаёт FreeRTOS задач или аппаратных сервисов. Однократные
завершения обслуживаются app-owned LVGL heartbeat; standalone звук — отдельный main backend.

## Platform и контексты

`kitchen_platform_t` — C ABI: context, lock/unlock, now_ms, navigate, optional
play_chime/set_volume (оба или ни одного, неблокирующие).
Lock возвращает esp_err_t, timeout в **миллисекундах**, 0 = бесконечно;
бесконечный lock обязан завершаться успешно при живом GUI. Bridge преобразует
bool bsp_display_lock в ESP_OK/ESP_ERR_TIMEOUT. now_ms — monotonic, без I/O.
Navigate только ставит публичный intent в очередь без ожидания: timeout=0 ticks.
Host app_interface timeout не переинтерпретируется как platform ms, v1 его не использует.
Lifecycle-lock имеет предел 1000 мс; destroy использует бесконечный lock, потому что
app_base_destroy освобождает self независимо от результата callback.

Наблюдаемые контексты хоста:

| Callback | Контекст | Kitchen lock |
| --- | --- | --- |
| create | регистрация, без LVGL | не нужен, модели ещё нет у UI |
| create_content | jobs_queue LVGL timer, GUI уже заблокирован | повторно не берётся |
| UI events/motion | LVGL worker | повторно не берётся |
| start/stop/pause/resume/minimize/restore | управляющая задача | предоставленный lock |
| on_view_change, before suspend/resume | управляющая задача, часто manager mutex | предоставленный lock |
| destroy | вне GUI lock, после/до удаления parent | lock до синхронного удаления root |

Новых задач/подписок/async UI dispatch в embedded-модуле нет. Ни один callback
не содержит network/flash I/O, joins или ожиданий очередей. UI Back не ждёт manager
mutex: только nonblocking event enqueue, что исключает цикл manager→GUI→manager.
Интегратор должен сохранять эти условия контекстов; fixture им следует.

## Lifecycle и parent ownership

Descriptor реализует все перечисленные callbacks актуального ABI. Widget/background
выключены. Suspend разрешён: состояние остаётся в instance, UI tick и анимации остановлены;
дедлайны продолжают отсчёт. Resume активирует UI только если он fullscreen, running
и не приостановлен хостом; это не пользовательская Pause конкретного таймера.

create_content создаёт только свой root внутри переданного parent; parent не
удаляет, lv_scr_load не вызывает. До нового create_content прежний root удаляется.
Сцена рассчитана на 1024×600. Standalone всегда передаёт полный контейнер,
не создаёт диагностическую сцену и не имеет переключения Fixture / Full UI.

При hide/minimize/pause/suspend анимации отменяются, UI timer приостановлен,
root скрывается. Show актуализирует состояния по now_ms и возобновляет UI timer
без worker/subscription. Stop удаляет root и UI timer, сохраняя модель экземпляра. Destroy повторяет идемпотентный cleanup, затем хост освобождает instance.

LV_EVENT_DELETE собственного root обрабатывается также при внешнем удалении parent:
отменяются анимации и UI timer, root/child pointers обнуляются и view generation увеличивается.
После этого stop/destroy не удаляют объект повторно. Callback хранится на root,
который синхронно уничтожается до free(instance); dangling delete subscriptions нет.
Queued events с pointer на instance/view в модуле отсутствуют, поэтому view generation
не используется как замена реальному владению. Любое будущее async API потребует
отдельного owned payload/cancellation/token контракта.

## Standalone fixture и аппаратный lifecycle

Runner — код в main, не копия app_manager. Он выделяет instance по descriptor,
вызывает реальный ABI и создаёт/удаляет borrowed parent сам. Для ABI нужен явно
предоставленный заголовок с проверкой SHA-256. Реализации app_manager/jobs_queue
и хостовые сервисы не копируются. Скрипт подготовки не является шагом сборки.

Runner создаёт один app instance и fullscreen parent, затем возвращает управление
app_main. Модель и UI живут до reset. Тестовая очередь, controls и автоматический
lifecycle-прогон удалены. В standalone нет внешнего home screen: platform.navigate
возвращает ESP_ERR_NOT_SUPPORTED; обычная навигация между экранами kitchen локальна.
Подключение будущего хоста продолжает использовать существующий bridge.

Существующие crowpanel_board/BSP сохранены только для стенда. BSP владеет GPIO,
LDO3/DSI/IO/LCD с двумя PSRAM framebuffer, I2C0/GT911 и LEDC. display_port использует
esp_lvgl_port 2.8.0~1, LVGL8 direct_mode=true и avoid_tearing=true. Порт получает два
существующих LCD framebuffer через esp_lcd_dpi_panel_get_frame_buffer; отдельного
draw buffer и второй пары кадров нет. Переключение кадра ожидает refresh notification
через semaphore порта. Full refresh, software rotation, PPA и LCD DMA2D выключены.
До успешной инициализации подсветка 0%, после первого refresh — 70%. Стендовый slider
0–100%, 0 восстанавливается до 70% через 5 секунд; embedded UI яркость не меняет.

Port LVGL8 deinit асинхронен, публичного join нет; remove_disp имеет проблемное
владение borrowed LCD buffers. Поэтому аппаратный runtime teardown/restart не
предоставлен: display_port_deinit гасит подсветку и удерживает живой порт/handles
до reset с ESP_ERR_NOT_SUPPORTED. До создания port частичные ресурсы BSP освобождаются
в обратном порядке; при ошибке удаления родитель ресурса сохраняется. Эта политика
не влияет на повторное полное освобождение kitchen instances/UI при open/close.

При неуспешном lvgl_port_add_disp_dsi direct-вариант вызывает abort: error path порта
может освободить borrowed framebuffer, поэтому обычное продолжение или BSP cleanup
опасны. Это аварийный путь, который может приводить к повторным reset при устойчивой
ошибке выделения; такая ошибка требует диагностики. После успешной регистрации
runtime remove_disp/deinit не вызывается. Эта политика не меняет kitchen lifecycle.

## Startup overlay standalone

main/startup_screen создаёт временный непрозрачный overlay в top layer дисплея.
Подсветка включается существующим display_port_start только после первого loading
кадра. Затем создаётся обычный app instance; overlay остаётся выше его parent и
блокирует касания до конца перехода. Прогресс обновляется по этапам подготовки,
после Ready выполняется fade 260 мс, не раньше 900 мс от появления loading.

Все операции overlay и callbacks сериализованы GUI-lock; используется обычный
LVGL animation engine, без новой FreeRTOS задачи/таймера. DELETE отменяет собственную
анимацию и обнуляет ссылки; ready callback удаляет только overlay. Hardware port
и borrowed app parent не удаляются. При ошибке запуска Kitchen overlay остаётся
со строкой ошибки; при ошибке создания loading выполняется прежняя политика
удержания аппаратного порта до reset. В embedded closure splash не входит.

Имя detail позиционируется по сохранённой геометрии карточки, отдельным 32-px шрифтом;
большой glyph 64 px растеризован из векторного исходника и не масштабирует bitmap
компактной карточки. Общая модель времени и platform ABI не изменялись.

## Audio boundary и независимый heartbeat

kitchen_core фиксирует RUNNING→DONE в ограниченном pending_chimes (до 20),
не меняя порядок или состояние после окончания. При volume=0 события звука
не накапливаются. kitchen_app создаёт один LVGL timer на start под GUI-lock,
актуализирует модель каждые 50 ms и потребляет pending ровно один раз. Command
также дренирует события, чтобы переход на границе дедлайна не потерял уведомление.
UI собственный timer по-прежнему существует только при видимом root; heartbeat
не обращается к UI объектам. Destroy удаляет оба timer до освобождения instance.

Optional platform callbacks не блокируют GUI и не получают app/UI pointers.
Standalone play_chime копирует token в FreeRTOS очередь 20 элементов с wait=0;
set_volume делает atomic_store. Внешний worker работает только со своим I2S,
не берёт GUI-lock и не ждёт app_manager. GPIO30 выключает amp после сигнала;
хвост тишины длиннее DMA queue не обрывает последние samples. PCM/gain — integer,
рабочий буфер 1024 bytes, task stack 4096 bytes. Аппаратный lifetime до reset.

Сам модуль kitchen_* не инициализирует speaker/I2S/PA. Main отдельно включает
standalone_audio и бинарный WAV. Реальные хостовые audio/microphone sessions,
разметка, NVS и host sources не менялись. Поля platform struct добавлены в конец:
старые designated initializers с NULL audio сохраняют source compatibility,
но binary ABI размера struct меняется — нужна пересборка. Descriptor ABI прежний.
