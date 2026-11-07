# Архитектура совместимого модуля

## Dependency closure

```text
embedded: kitchen_app -> app_manager (host ABI)
             |-> kitchen_platform -> esp_common + kitchen_core
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

Core: пять постоянных слотов с ID 1–5, EMPTY/READY/RUNNING/PAUSED/DONE,
plain-C сериализация и координатор сохранения kitchen_settings.
Конфигурация отделена от сеанса; слот и цвет не зависят от состояния.
Running хранит monotonic deadline, paused — точный остаток, done — исходный дедлайн.
Динамических порядков active/saved больше нет. Стартовая длительность не меняется
при корректировке текущего остатка. Состояние и все команды описаны в [TIMERS](TIMERS.md).
Модель принадлежит app instance; UI callbacks и tick работают под существующим GUI-lock.
LVGL user_data содержит только ссылки на владельца и bindings, живущие до удаления root.
Embedded-модуль не создаёт FreeRTOS задач или аппаратных сервисов. Однократные
завершения обслуживаются app-owned LVGL heartbeat; standalone звук — отдельный main backend.

## Platform и контексты

`kitchen_platform_t` — C ABI: context, lock/unlock, now_ms, navigate, optional
play_chime/set_volume (оба или ни одного, неблокирующие), preview и optional
storage load/submit/poll (полная группа или ни одного). Формат записей — TIMERS.md.
load вызывается только в create вне GUI-lock; submit/poll не блокируют и не хранят
app/UI pointers. kitchen_settings принимает только актуальный ticket.
Короткие mailbox critical sections standalone не охватывают NVS I/O.
Worker standalone сериализует запись и commit; load ждёт ранее принятые записи.
Backend и его tickets живут дольше всех app instances, не создают callbacks в instance.
Без backend отображается уведомление о работе в RAM. Структуры platform/core/UI
изменились: требуется совместная пользовательская пересборка зависимых компонентов.
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

Новых задач/подписок/async UI dispatch в embedded-модуле нет. GUI callbacks не содержат network/flash I/O, joins или ожиданий очередей.
Исключение для загрузки: create вызывает storage.load вне GUI-lock; этот backend
может ждать ранее принятые записи и читать flash. UI Back не ждёт manager
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

Главный экран раскладки A использует фиксированную геометрию шести карточек.
Имена обрезаются многоточием, время подбирает существующий шрифт по ширине строки.
Существующие glyphs используются без повторной генерации.

## Исторические детали аудио до раскладки A

Ниже сохранена история исходной реализации. Для текущего поведения, хранения
и интерфейса приоритет имеет TIMERS.md. Размер модели теперь пять слотов;
standalone audio queue по-прежнему содержит 20 запросов.

## Audio boundary и независимый heartbeat

kitchen_core фиксирует RUNNING→DONE в ограниченном pending_chimes (до 6),
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

## Раскладка A и сохранение

UI создаёт пять прямоугольных карточек один раз. Tick обновляет изменившиеся
цифры, полосу и внутренние контуры; старые списки, drag и halo bitmap cache удалены.
Единственный timer overlay сохраняет локальные черновики при DONE; клавиатура
не пересоздаёт барабаны. Все bindings живут в ui, анимации используют только ui
и удаляются при hide/delete/destroy. Parent остаётся borrowed.

Heartbeat также опрашивает сохранение. Stop удаляет UI и приостанавливает heartbeat,
сохраняя модель живого instance; backend продолжает уже принятые записи. Destroy
удаляет UI/heartbeat без join storage worker. Новый create ждёт backend barrier
вне GUI-lock и восстанавливает только постоянные настройки, без сеансов.

В standalone добавлен main/standalone_storage.c: одна задача 3072 bytes stack,
семь фиксированных mailbox и mutex только для NVS I/O вне GUI. Глобальный NVS
инициализирует только standalone. Автоматический erase отсутствует; формат и ошибки
описаны в TIMERS.md. Требуется nvs_flash из существующего IDF, новых пакетов нет.

## UI-коррекция от 05.11.2026

Timer overlay теперь фиксирован 600×528, центр (512,300), анимация только opacity
160 ms. Backdrop закрывает его с отменой текущего черновика. Name-панель 600×300
центрируется в области над клавиатурой; keyboard использует bottom alignment без
дополнительного смещения. GUI-lock, callback ownership и storage контракт сохранены.
Кнопки действий измеряют label и имеют минимум 48×48; No icon — вертикальная
композиция в одной ячейке. Settings glyph увеличивается штатным transform_zoom
только на маленьком label, не меняя font asset или всю карточку. Fade панели
использует штатную LVGL opacity layer, её стоимость требует проверки heap на панели.

## Уточнение производительности и компоновки

Текущий UI заменяет предыдущий fade: panel 600×440 открывается/закрывается без
opacity/transform animation. Шестерёнка 32px рисуется arc/line примитивами без
font fallback и маленького transform layer. Border pulse инвалидирует четыре
20px краевые полосы. Фон под modal не перерисовывается, кроме изменения статуса;
core heartbeat/audio/storage продолжаются. Direct mode + avoid tearing, два
borrowed LCD framebuffer и отсутствие дополнительного draw buffer сохранены.
Размер массива presets увеличен с 4 до 6, все bindings остаются owned by UI.
Отмена анимации строки значков и очистка при hide/delete/destroy сохранены.
Прежние описания fade/transform в разделе UI-коррекции выше — история.

## Общая колонка и inline-ввод имени

Текущий panel 600×460, центр (512,300). ui.fields_width измеряет шесть preset
кнопок один раз. layout_fields выставляет те же границы для rollers/captions,
header icon/name и горизонтального выбора значков. Небольшой label со штатным
LV_SYMBOL_SETTINGS масштабируется отдельно; native glyph не заменяется рисованным.
Main buttons оставляют минимум 16px до края, вне максимальной 11px рамки.
Для имени переиспользуются panel/modal; textarea заменяет name button в той же
строке. Существующий panel сохраняет y=70, меняет только высоту до 302px
(нижний край y=372, 8px до keyboard), body скрывается; keyboard
по-прежнему BOTTOM_MID 1024×220. Черновики и UI objects барабанов не удаляются.
Закрытие и Cancel timer не оставляют queued callbacks; root/delete очищает
roller_labels и кэш fields_width. Ограниченный transform layer одного gear label
не возвращает отменённый full-panel opacity fade или bitmap halo cache.

## Дополнительные glyphs и стабилизация имени

Новый kitchen_font_extensions.c подключён отдельным source: kitchen_symbols_56,
kitchen_settings_36 и status18/20 Inter Bold 700. Получены существующим plain
Pillow asset helper через tools/generate_ui_extensions.py; SDK/compiler не
запускаются. Старые generated font files и vendor не изменены. Runtime FreeType,
SVG decoder и новые зависимости отсутствуют. Небольшие const bitmap arrays
находятся в firmware image; фактический прирост flash проверяет сборка владельца.

Settings не использует transform_zoom. Icon row больше не имеет width animations
или ready callbacks. Pressed border резервирует одинаковые 2px во всех состояниях.
Textarea content height исправлен с 26px до 34px для строки Inter24 высотой 30px;
это устраняет обнаруженную в исходниках причину vertical scroll-to-cursor.
Direct mode/avoid tearing, frozen background и edge-strip invalidation сохранены.

## Иерархия статусов и распределение normal overlay

Статусы главной сетки имеют три уровня: 32px (существующий Inter700 heading font)
на S1, новый ограниченный subset kitchen_status_24 на S2/S3 и status18 на S4–S5.
Overlay также status24. tools/generate_ui_extensions.py --status24-only создаёт
только новый kitchen_status_24.c; прежние generated fonts не изменяются.

normal_layout_valid кэширует расположение time/status/controls/progress. При
смене статуса normal-вида или возврате из редактора позиции актуализируются;
editor objects и drafts не пересоздаются. Верхний normal-блок time/status теснее,
левая/правая кнопки корректировок и нижние действия опираются на поля общей
колонки. Основные действия сохраняют y=252. Отдельных timers/tasks/allocations
для layout нет. Frozen background и edge-only border invalidation сохранены.

## Адаптивные размеры главной сетки

make_card выбирает glyph/font tier по ширине карточки, затем рассчитывает
геометрию от w/h, метрик шрифтов и min touch target. inset/radius хранятся
в kitchen_card_t, без heap allocations или новых задач. Основная кнопка
подбирает label/control font и padding по своей высоте: overlay кнопки48px
сохраняют прежний размер, large main card получает81px/Inter32/control48.
Крупные glyphs поставляются новым kitchen_large_assets.c, без изменения прежних
generated файлов. В runtime не добавлены SVG/FreeType/transform layers.

Border pulse ускорен до1000/1300ms, ширина ограничена inset−5. invalidation
учитывает радиус карточки+2, чтобы перерисовывать увеличенные углы без trails,
сохраняя частичную перерисовку. Main layout рассчитывается при создании, на tick
меняются только значения и активные стили. Аппаратный display_port не менялся.

## Статичная рамка раскрытой карточки

overlay_card хранит фиксированные border_width6/border_opa255, glow0.
refresh_overlay больше не вызывает update_glow: пульсация и краевые invalidation
остаются у cards главной сетки. Draw callback переиспользуется без style border,
поэтому не меняется начало координат детей. Статус меняет только visibility,
общие вертикальные anchors READY/RUNNING/PAUSED постоянны. DONE возвращён к
прежней раскладке: +1min/Repeat по краям общей колонки над progress, OK отдельно
по центру; вертикальные anchors выбираются при смене состояния/возврате в normal.
primary_caption сохраняет пространство под glyph. Повторного создания
UI/animations/tasks нет.

Для A_ICON button удаляет стили темы сразу после создания, затем задаёт явные
стили размера, заливки, текста и border. Поэтому theme grow/transition/filter
отсутствуют, checked fill и border сохраняются без изменения размеров.
NULL transition descriptor не используется: LVGL8.4 перебирает transition
каждого style и разыменовывает tr->props без проверки tr на NULL.
Новый const bitmap subset
Inter700/20px подключён CMake, исходный Inter32 переиспользован для имени.
Модель, storage worker, синхронизация и lifecycle этой коррекцией не меняются.

## Раскладка B: пять слотов

KITCHEN_MAX_TIMERS=5, KITCHEN_SETTINGS_KEYS=6 (volume + S1–S5). Размеры модели,
UI-массива, settings entries и bounds backend согласованы через эти константы;
key6/ID6 больше не принимаются. Формат version1 и постоянные ID1–5 сохранены.
Короткие карточки определяются по h<240, не по ширине; S4/S5 теперь292×208.

Standalone worker при старте под s_io выполняет точечный erase slot6/commit.
Флаг s_ready под s_mux открывает load только после попытки миграции, load ждёт
вне GUI-lock и дополнительно сохраняет прежний барьер поколений записей.
Миграция не содержит app/LVGL pointers и не создаёт новую задачу. Ошибка видна
в логе, остальные ключи доступны, следующий boot повторяет попытку. Реальная
host-интеграция не менялась, host RAM-only не имеет NVS-миграции.
