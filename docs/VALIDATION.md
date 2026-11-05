# Приёмка Kitchen Timers раскладки A

Дата исходников: 04.11.2026. Текущее ТЗ: kitchen_att2_spec_A.docx.
Исторические PASS ниже относятся к прежним версиям и не подтверждают раскладку A.

| Категория | Статус текущей версии | Основание |
| --- | --- | --- |
| Статический просмотр | PASS в пределах просмотра исходников | Проверены diff, ссылки символов, GUI/worker ownership; не является компиляцией или runtime test |
| C-сценарии ядра/хранения | NOT RUN | По политике владельца агент не компилирует и не запускает |
| Firmware build | NOT RUN | Выполняет владелец по BUILDING.md для своей платы/silicon |
| NVS power-loss/reset | NOT RUN | Нужна реальная панель |
| UI/touch/audio/heap/lifecycle | NOT RUN | Нужна реальная панель |
| Реальная host integration | NOT RUN | В эту задачу не входит |

## Закрытие overlay после Repeat

Изменены components/kitchen_ui/kitchen_ui.c, docs/TIMERS.md и этот файл.
A_REPEAT закрывает modal через существующий close_modal только после успешной
команды KITCHEN_REPEAT. Статический просмотр и diff hygiene: PASS в пределах
исходников; сборка/устройство NOT RUN. Вручную: дождаться DONE, открыть карточку,
нажать Repeat — сразу видна главная сетка, нужный слот RUNNING с сохранённой
длительностью; повторное открытие работает, имя/значок сохранены. Проверить
несколько повторов и отсутствие повторного запуска от одного нажатия.

## Возврат прежней раскладки DONE по фото

Изменены components/kitchen_ui/kitchen_ui.c, docs/TIMERS.md,
docs/ARCHITECTURE.md и этот файл. DONE: time y0/status y92, +1min слева,
Repeat справа на y140 (56px), progress y212, OK отдельно по центру на y252.
Жирные шрифты, статичная рамка overlay, фиксированные позиции PAUSED и
исправление NULL transitions сохранены. Старые пункты про центрированную
пару OK/Repeat ниже описывают предыдущую версию.

Статический просмотр и diff hygiene: PASS в пределах исходников.
Сборка и устройство: NOT RUN. Вручную проверить вход в DONE, Repeat с короткой
и длинной длительностью, +1min, OK, возврат из редактора и переход в RUNNING.
Сравнить расположение с фото; проверить отсутствие обрезки и пересечений.

## Исправление panic при открытии Set up

Пользовательский лог сборки/прошивки build-layout-a-hw1.0-p4legacy,
ELF SHA256 prefix80985a143: сборка PASS для той версии, настройка таймера FAIL.
Load access fault, MTVAL0; lv_obj_set_state:913 разыменовывает tr->props,
вызов из refresh_overlay при lv_obj_add_state(icon_button, LV_STATE_CHECKED).
Причина в добавленных ранее локальных transition=NULL у кнопок значков:
в LVGL8.4 такой descriptor небезопасен, включая скрытые кнопки при первом refresh.

Изменены только components/kitchen_ui/kitchen_ui.c, docs/ARCHITECTURE.md
и этот файл. A_ICON снимает стили темы сразу после lv_btn_create, до размеров
и явных стилей приложения. Убраны NULL transitions и избыточные overrides.
Это сохраняет отсутствие grow/анимаций/pressed filter, border и checked fill;
не меняет vendor LVGL, GUI ownership, модель, NVS, аппаратные профили или задачи.

Статический просмотр исправления: PASS в пределах исходников. Сборка и
аппаратная проверка исправленной версии: NOT RUN. Предыдущий успешный build
не подтверждает новую правку.

Ручная проверка: повторить сборку/прошивку своего прежнего каталога, открыть
Set up у каждого EMPTY слота; раскрыть значки, удержать/отпустить каждый,
включая No icon и выбранный. Проверить CHECKED/PRESSED, первый внешний тап,
создать таймер, повторно открыть, изменить значок и перезагрузить для проверки
сохранения. Повторить 20 открытий/закрытий без panic и роста кнопок.
Повторно стирать NVS для этого исправления не требуется.

## Текущая коррекция статичного overlay и pressed значков

Изменены components/kitchen_ui/kitchen_ui.c, CMakeLists.txt, FONT_LICENSE.txt,
assets/README.md, tools/generate_ui_extensions.py, docs/TIMERS.md,
docs/ARCHITECTURE.md, docs/BUILDING.md и этот файл. Добавлен
components/kitchen_ui/kitchen_overlay_fonts.c. Старые generated assets не менялись.

Статический просмотр: PASS в пределах исходников — pressed grow темы перекрыт
локальными стилями; update_glow вызывается только для карточек главной сетки;
статус не меняет вертикальные anchors, нижняя пара сохраняет glyph width.
Inter700 asset создан из местного TTF без компиляции, preview просмотрен.
Firmware build, runtime, touch, erase NVS и аппаратная приёмка: NOT RUN.

После ручной сборки/прошивки своего профиля проверить:

1. Удержание каждого значка, включая No icon и уже выбранный: размер клетки
   постоянен, только оранжевая рамка; нет выхода за строку, сдвига и следов после
   release/PRESS_LOST. Checked fill и первый внешний тап сохранены.
2. Overlay в READY/RUNNING/urgent/PAUSED/DONE, включая редакторы/имя/значки:
   рамка6px статична. После закрытия на главной сетке urgent/DONE пульсируют.
3. READY→RUNNING→PAUSED→RUNNING→DONE: time y12, ±1min y120 и progress y196
   не сдвигаются. PAUSED/DONE y88 целиком помещаются между видимыми цифрами
   и кнопками; проверить 00:01,59:59,23:59:59, DONE>суток.
4. DONE: +1min по центру, OK и Repeat внизу вместе по центру, без обрезки
   glyph/подписей. Repeat60s/23:59:59 и переход Repeat/+1min в RUNNING;
   Cancel timer закрывает overlay, имя/значок/постоянная длительность сохранены.
5. Имя Inter700/32px в обычной строке и при вводе: строчные/заглавные/цифры,
   длинное имя, placeholder, удаление до пустого, Done/Cancel. Высота и baseline
   не прыгают; горизонтальная прокрутка, клавиатура и отмена работают.
6. Pause/Resume/OK/Cancel timer/Start timer/Repeat жирнее; disabled стиль,
   ширина по тексту и glyph, сохранение черновика при чужом DONE не нарушены.
   Проверить flash budget новой сборки; фактический прирост пока не измерялся.
7. По отдельному решению владельца полный сброс standalone по BUILDING.md:
   после успешного erase/reboot все шесть EMPTY, volume40, новые параметры
   сохраняются и переживают повторную перезагрузку. В crow-ha-fw не выполнять.

## Текущая коррекция пропорций карточек и активности рамки

Изменены components/kitchen_ui/kitchen_ui.c, include/kitchen_ui.h,
components/kitchen_ui/CMakeLists.txt, FONT_LICENSE.txt, assets/README.md,
tools/generate_ui_extensions.py, docs/TIMERS.md, docs/ARCHITECTURE.md и этот файл.
Добавлен components/kitchen_ui/kitchen_large_assets.c. Старые generated fonts
не менялись. Новый размер88px подготовлен из ранее скачанного официального TTF
с проверенным SHA-256. Asset preview40/56/88px и схема компоновки просмотрены.
Firmware build, runtime, frame rate и touch validation: NOT RUN.

После ручной пересборки и прошивки текущего HW/silicon профиля проверить:

1. Cup/Pot/Pan/Oven на S1 явно крупнее средних:88/56/40px, glyphs не обрезаны.
   Имя с/без значка, длинное имя и пустая строка сохраняют вертикальный ритм.
2. Основные кнопки S1:81px, label32/control48; S2/S3:56px,label24; маленькие:
   48px,label18. Все пять подписей Set up/Start timer/Pause/Resume/OK помещаются.
   No icon/Settings не пропадает; glyph не задевает округлённую кнопку.
3. Проверить00:01,09:00,59:59,1:00:00,23:59:59 и DONE>суток: цифры целиком,
   time/status/progress/button не пересекаются. Статусы36/24/18px в правильном
   масштабе, нажатия кнопки не открывают overlay.
4. DONE цикл1s и срочный RUNNING1.3s: заметная пульсация ширины и opacity,
   максимум border не ближе5px к кнопкам. Все шесть одновременно, Pause/OK,
   строго60000ms и ниже. Увеличенные углы не оставляют trails.
5. Сравнить heap/GUI stack и плавность после прогрева; frame обновляет только
   края. Overlay не теряет клавиатуру/черновик при чужом DONE; frozen background
   и sound/storage heartbeat продолжают работать.
6. Проверить размер firmware/factory slot по ручной сборке: новые const glyphs
   увеличивают image, фактический прирост ещё не измерялся.

## Текущая коррекция иерархии статусов по фото

Изменены components/kitchen_ui/kitchen_ui.c, include/kitchen_ui.h,
components/kitchen_ui/CMakeLists.txt, components/kitchen_ui/FONT_LICENSE.txt,
components/kitchen_ui/assets/README.md, tools/generate_ui_extensions.py,
docs/TIMERS.md, docs/ARCHITECTURE.md и этот документ. Добавлен generated
components/kitchen_ui/kitchen_status_24.c. Старые generated fonts не менялись.
Генератор нового asset выполнен без компилятора/SDK build.
Firmware build, UI и панельная приёмка этой коррекции: NOT RUN.

После ручной пересборки и прошивки текущего HW/silicon профиля проверить:

1. PAUSED/DONE: S1 32px, S2/S3 24px, S4–S6 18px, все Inter Bold700. Проверить
   рядом с MM:SS / 23:59:59 / DONE >суток: цифры и статус не перекрываются.
   У средних карточек time82..170, status172..202, progress206..222, button228..276.
2. Обычный overlay: статус24 рядом с цифрами, −/+ на разных краях общей колонки,
   progress под ними, основные действия отдельным нижним рядом y=252. READY
   без зарезервированной пустой строки; Start по центру. PAUSED/DONE сохраняют
   место для явного статуса. Проверить Pause/Resume/OK, ±1min и Repeat.
3. Открыть Duration/Remaining/Add more time, изменить draft, дождаться DONE
   и вернуться Cancel/Apply. Черновик не теряется, normal_layout пересчитывается
   даже если статус поменялся в скрытом normal-виде. Первый Apply на deadline
   не запускает таймер автоматически.
4. Ввод имени, выбор значков и title по-прежнему работают без изменения своей
   компоновки. Другие таймеры продолжаются. 50 open/close не накапливают объекты.
5. Длинные Repeat/Cancel/Resume помещаются у края общей колонки; нет пересечения
   с рамкой11px, активная/disabled кнопки сохраняют состояния.

## Текущая коррекция title и первого запуска

Изменены только components/kitchen_ui/kitchen_ui.c, docs/TIMERS.md и этот файл.
Статически просмотрены назначения heading, возврат из имени, переход в DONE,
child glyph кнопки Apply и очистка объектов. Firmware build/device: NOT RUN.

После ручной пересборки и прошивки прежнего HW/silicon профиля проверить:

1. EMPTY: title Set up timer, Start timer с Play и полной подписью, в disabled
   состоянии серая, при выборе 1 min/другого пресета жёлтая. Ноль не запускает.
2. Настроенный слот: Timer; открытие Duration/Remaining time/Add more time
   меняет только title в левом верхнем углу. Над барабанами нет второго заголовка.
3. В каждом режиме открыть имя: Timer name. Done/Cancel возвращают прежний title
   и значения барабанов; выбор значков не добавляет отдельный title.
4. RUNNING заканчивается при Remaining time: title Add more time, значения
   барабанов сохранены. Если DONE впервые обнаружен на Apply — первое нажатие
   только показывает завершение и Start timer; второе запускает отсчёт.
5. DONE во время имени: Timer name остаётся до Done/Cancel, затем актуальный
   title предыдущего режима. Проверить чужой DONE и 20 повторных open/close.

## Текущая коррекция размеров значков и текста

В этой коррекции изменены: components/kitchen_ui/kitchen_ui.c,
components/kitchen_ui/CMakeLists.txt, components/kitchen_ui/FONT_LICENSE.txt,
components/kitchen_ui/assets/README.md, components/kitchen_ui/assets/SHA256SUMS,
docs/TIMERS.md, docs/ARCHITECTURE.md, docs/SOURCES.md и этот документ.
Добавлены components/kitchen_ui/kitchen_font_extensions.c,
tools/generate_ui_extensions.py, components/kitchen_ui/assets/material/settings_36px.svg.
Старые kitchen_fonts.c/kitchen_symbols.h не изменены. Официальный TTF загружен,
SHA-256 совпадает с прежним; новые glyphs и жирные статусы просмотрены на PNG.
Это PASS только проверки asset generation. Firmware build/runtime: NOT RUN.

После ручной пересборки/прошивки своего профиля проверить:

1. Settings виден на всех EMPTY без transform_zoom; около 33px ink height.
   На S2/S3 кухонные значки 56px, имя не пересекается с glyph и временем.
2. PAUSED/DONE жирнее (Inter700) на всех размерах и overlay, без обрезки.
3. Набирать/стирать имя, вводить 32 ASCII, Shift/пробел, переводить из пустого
   placeholder к тексту: baseline не прыгает, курсор виден. Padding 10/10 и
   border1 оставляют 34px для 30px строки. Горизонтальный scroll допустим.
4. В RUNNING/PAUSED ±1min выше progress, высота56/Inter24, полностью видны.
   В DONE +1/Repeat также помещаются. Progress совпадает краями с icon/name.
5. Close в (550,0) размером48×48, видимая форма glyph точно в центре круга.
   Круг не обрезан скруглением панели; весь target закрывает окно.
6. Строка значков появляется/скрывается целиком, без растягивания. Pressed
   рамка оранжевая, selected фон оранжевый, glyph при нажатии не сдвигается.
7. Убедиться, что flash image помещается в прежний factory slot; после прогрева
   сравнить heap и плавность шести DONE. Сборка/аппаратные команды агентом не запускались.

## Текущая коррекция общей колонки и inline-редактирования

Изменены kitchen_ui.c, include/kitchen_ui.h, TIMERS.md, ARCHITECTURE.md и этот файл.
Сборка и панельные проверки: NOT RUN. После ручной сборки текущего профиля:

1. Проверить LV_SYMBOL_SETTINGS на всех EMPTY: штатный gear, около 32px по высоте,
   без пустого места, многоточия, обрезки и наложения на Set up. Остальные иконки
   чуть полупрозрачные; поля внутри кнопок сохраняются после смены статуса.
2. Сравнить внешние края шести пресетов, трёх rollers/captions и строки icon/name:
   одна колонка, rollers уже прежних; заголовок не зажат строкой (y=56).
   Все шесть пресетов доступны и по-прежнему не запускают отсчёт сразу.
3. Раскрыть значки: ровно пять вариантов, без шестого текущего слева. Selected
   всегда оранжевый независимо от цвета слота. Тап внутри панели вне строки только
   сворачивает её; тап backdrop закрывает окно. Close круглый, полупрозрачный,
   цвета слота, target 48×48; его фон не окрашивает всю панель.
4. Нажать имя в Setup/Duration/Remaining/DONE: та же panel и её верх y=70, нижний край поднимается до y=372, поле ровно на месте
   имени, клавиатура снизу с оранжевыми pressed keys, Done/Cancel под строкой.
   При возврате сохранены выбранные секунды, статус, фокус/черновик при DONE.
   Двадцать повторных открытий не создают дополнительные окна/обработчики.
5. На всех карточках проверить максимальную рамку DONE 11px: кнопка отстоит
   минимум на 16px от внешнего края, есть видимый зазор, иконка не задевает рамку.
   На S4–S6 проверить Start timer, PAUSED/DONE, 23:59:59, длинное имя, progress:
   цифры не обрезаны, кнопки не пересекаются со статусом/полосой.
6. Cancel timer из RUNNING и PAUSED: главная сетка, слот READY на прежнем месте
   с исходной длительностью; другие таймеры продолжают идти. На границе DONE
   отклонённый Cancel не подтверждает завершение и не теряет его сигнал.

Оптимизации direct mode/avoid tearing, frozen background и edge-strip invalidation
сохранены; нужны новая визуальная/heap проверка на панели, особенно native gear zoom.
Ниже остаётся история предыдущих UI-вариантов, а не текущие параметры.

## Текущая коррекция компактного overlay и производительности

Изменены только kitchen_ui.c, include/kitchen_ui.h, TIMERS.md, ARCHITECTURE.md
и этот документ. Обновления модели/хранилища, audio assets и BSP не выполнялись.
Сборка и runtime проверки этой коррекции: NOT RUN, их выполняет владелец.

После ручной пересборки и прошивки того же профиля проверить:

1. Шестерёнка видна на всех EMPTY карточках, около 32px, без пустого места;
   Play/Pause/Resume/OK и подписи не перекрываются. Полный текст кнопки Repeat
   (включая 30 min и 23:59:59) не заменяется точками и не обрезается.
2. Overlay 600×440 центрирован (212,80), появляется и закрывается сразу,
   крестик ближе к краю (4px справа/2px сверху, target 48×48), backdrop закрывает.
   Шесть пресетов 1/3/5/10/15/30 min помещаются в строку; Hours/Minutes/Seconds
   не перекрывают три видимых строки барабанов. Выбор 04:30 не выделяет пресет.
3. При нулевом времени Start timer остаётся видимой серой и не нажимается;
   после выбора времени становится жёлтой, после возврата к нулю снова серой.
4. Клавиатура печатает, Shift/Backspace/Done/Cancel работают, нажимаемая клавиша
   оранжевая с тёмной буквой. Name-панель 600×208, y=86, поле и курсор видны.
5. У RUNNING <60s / DONE пульсирует ширина/прозрачность рамки (6–9 / 7–11px),
   внутреннее свечение слабее и уже. При 60s и PAUSED пульсации нет. Геометрия
   текста и кнопок не меняется. Оценить шесть одновременных DONE и плавность.
6. При открытом overlay фон неподвижен, но завершившийся таймер переходит в DONE
   и звучит; редактор не теряет черновик. После закрытия все цифры актуальны.
   Выполнить 50 циклов open/close и hide/delete parent с открытой строкой значков,
   сравнить heap после прогрева. Проверить отсутствие слоя fade всего окна.

Подтверждено исходниками: display_port продолжает direct_mode/avoid_tearing с
двумя borrowed LCD framebuffer, без отдельного draw buffer. Удалён введённый
ранее panel opacity fade; аппаратная плавность требует нового наблюдения владельца.
Секции о 160ms fade и transform glyph ниже описывают предыдущую версию.

## Новый сигнал таймера от 05.11.2026

Заменён main/assets/kitchen_timer_chime.wav. Media validation: PCM16 mono 44100 Hz,
91728 frames/2.08 s, 183500 bytes, без saturated samples, последний sample 0.
Формат сопоставлен с существующим parse_wav; путь EMBED_FILES и linker symbols
не менялись. Firmware build и аппаратное воспроизведение: NOT RUN.

После ручной сборки/прошивки проверить один DONE и шесть одновременных DONE:
новый bell один раз на событие, без длинной пустой паузы и обрыва хвоста. Проверить
0/40/100% громкости, отсутствие слышимых щелчков/искажений и прежний volume preview.
Фактический размер firmware/остаток factory slot взять из вывода новой сборки.
Исторические сведения о прежнем chime ниже не подтверждают новый asset.

## UI-коррекция от 05.11.2026

Правки источников: kitchen_ui.c, TIMERS.md, ARCHITECTURE.md и этот документ.
Сборка, запуск, visual/touch/keyboard/heap проверка: NOT RUN; агент их не выполнял.
Статически подтверждён конфликт позиции клавиатуры: constructor LVGL задаёт
BOTTOM_MID, прежний set_pos(0,380) добавлял смещение относительно этой привязки.
Теперь после set_size выполняется align(BOTTOM_MID,0,0); штатный input callback
сохранён, textarea получает фокус и отвязывается при Done/Cancel.

После ручной сборки своего профиля проверить:

1. Сначала Set up на EMPTY, затем имя READY/RUNNING/DONE: клавиатура видна внизу,
   печатает ровно один символ на tap; Backspace, Shift, пробел, 32 ASCII, пустое имя,
   Done/Cancel работают. Повторить открытие 20 раз и DONE во время ввода.
2. Панель 600×528 центрирована, No icon расположен под Block в одной ячейке,
   все пять вариантов помещаются. Первый tap по остальной панели только сворачивает
   строку; tap по затемнению закрывает окно и отменяет неподтверждённый ввод.
3. Открытие/закрытие — спокойный fade 160 ms, без перемещения и изменения размеров;
   быстрое открытие/закрытие и hide/delete parent в середине fade не дают callback
   к удалённым объектам. Замерить heap после прогрева с шестью шестерёнками и fade.
4. На всех шести карточках видны подписи Set up/Start timer/Pause/Resume/OK,
   кнопки помещаются и центрированы, шестерёнка около 32 px. Проверить 23:59:59,
   длинное имя, PAUSED/DONE: нет перекрытия с progress 10/16 px и кнопками 48 px.
5. Оценить яркость 3/4/5-px рамок и свечения 16/26 px, все шесть DONE одновременно,
   строго 60000 ms и ниже, Pause/OK. Свечение остаётся внутри собственного контура.

## Модель и хранение

Host-only C11 сценарии и точная команда владельца: [tests/README](../tests/README.md).
Они не добавлены в firmware build и не заменяют аппаратную проверку.

| ID | Ручная проверка | Критерий |
| --- | --- | --- |
| T01 | Первый запуск без kitchen records | 6 EMPTY, фиксированные цвета, пустые строки, volume 40% |
| T02 | Пресет и Start без имени/значка | Слот RUNNING без обязательного ввода метаданных |
| T03 | Изменить EMPTY черновик и закрыть крестиком | EMPTY, запись слота отсутствует |
| T04 | Pause/Resume между целыми секундами | Сохраняется точный остаток, нет ошибки от UI tick |
| T05 | 3 min → +1 min → DONE → OK | READY на 3 min, прежние ID/цвет/метаданные |
| T06 | READY Duration Apply и прямые ±1 min | Новая длительность сохраняется, Start/Repeat используют её |
| T07 | 1 s, 23:59:59, ноль, границы ±1 | Ноль запрещён, нет переполнения/зацикливания/обрезки цифр |
| T08 | DONE при открытых Remaining time барабанах | Черновик сохранён; запускает только явный Start timer |
| T09 | Apply непосредственно на deadline | Первое обнаружившее DONE нажатие не перезапускает |
| T10 | DONE больше часа и суток | Растущий H:MM:SS, без плюса/Overdue/сброса через сутки |
| T11 | 6 одновременных завершений | Все DONE на местах, 6 последовательных сигналов, повторный tick без звука |
| T12 | Подтверждённая запись всех слотов и volume, reboot | Те же параметры; только EMPTY/READY, без звука |
| T13 | Потеря питания во время set/commit одного слота | Старая или новая целая запись, без смешения полей |
| T14 | Одна повреждённая запись / неверный icon ID | Безопасный EMPTY только этого слота, остальные и volume сохранены |
| T15 | Ошибка set/commit, новые RAM-изменения, Retry | Ошибка видима, сеанс идёт, Retry сохраняет последний snapshot |
| T16 | Быстрые READY изменения и поздние ответы | Последний snapshot побеждает; старый успех не снимает новую ошибку |

Для T12 повторить reboot из READY, RUNNING, PAUSED, DONE, аппаратный/программный
reset и deep sleep с перезагрузкой. Громкость отдельно 0/40/100%; default после
повреждения 40%. Проверить неизвестную версию и oversized record: диагностика,
безопасный default, сохранение исходной записи даже после редактирования и Retry.
Загрузочная конфигурация должна появляться сразу, без кадра промежуточной пустой сетки.

Для T13 сравнить длительность/имя/значок как единую запись. Не ожидать общей транзакции
всех шести слотов. Подтверждение успеха backend означает успешные set_blob и commit;
полагаться на наличие записи только в RAM до commit нельзя. Режим инъекции ошибок
использовать через fake backend C-сценариев; физический power-loss проверяет владелец.

## UI и жизненный цикл

| ID | Ручная проверка | Критерий |
| --- | --- | --- |
| U01 | Главный экран | 6 карточек A, без scroll/drag/добавления/перестановок |
| U02 | Основные кнопки S1–S6 | Все с символом и подписью, ширина по label, ≥48×48; шестерёнка сопоставима с Play |
| U03 | Все комбинации пустого имени/значка | Нет placeholders/разделителей/подставленных названий на сетке |
| U04 | Открыть каждый слот и закрыть | Панель 600×528 по центру (212,36), fade 160 ms без расширения; tap снаружи закрывает, фон неподвижен |
| U05 | Выбор значка, повторное и внешнее касание | 5 вариантов в заданном порядке, выбор отмечен, 200 ms, первый внешний tap только закрывает |
| U06 | Имя 32 ASCII, пустое, Done/Cancel | Поле/курсор видны, возврат в прежний редактор, Done не запускает |
| U07 | Остаток ровно 60000 ms, затем ниже, Pause | Срочность только строго ниже, на паузе затухает |
| U08 | Все срочные и DONE одновременно | Собственные цвета, только внутренняя подсветка, без засветки соседей |
| U09 | Свой/чужой DONE во время имени/значков/времени | Нет потери текста, фокуса, барабанов и автоматического открытия |
| U10 | DONE OK, крестик, +1, Repeat | OK закрывает в READY; крестик оставляет DONE; продления оставляют панель |
| U11 | Длинные имена, 23:59:59 и DONE >суток | Имя с многоточием, цифры целиком, кнопки не прыгают |
| U12 | 50 циклов open/close, hide/suspend/stop/delete parent/destroy | Нет утечек/поздних callbacks, сеансы живого instance продолжаются |
| U13 | Volume drag/release/PRESS_LOST, 0%, недоступное аудио | Immediate gain, один preview и save на жест, 0% тихо, disabled без audio |
| U14 | Пользовательская сборка и панель | Зафиксированы фактический профиль, вывод и результат, без заимствования старых PASS |

В U12 дополнительно: destroy при ожидающей записи и немедленное создание нового
instance. Записи должны завершаться без app/UI pointers, новый create видит результат
ранее принятых записей и READY. Проверить внешнее удаление parent с открытой
клавиатурой, строкой значков, анимацией и slider gesture. Сравнивать free heap,
largest block и GUI/audio/storage stack на одинаковой стадии после прогрева.
Новые сохранения не создают задачу на каждый запрос; standalone имеет один storage
worker 3072 bytes stack, одну прежнюю audio task и прежний LVGL worker.

Проверить error banner и Retry при открытом overlay/клавиатуре: они не закрывают
редактор и не меняют геометрию сетки. Без storage у хоста — RAM-режим с явным
уведомлением. Без audio — визуальные таймеры и недоступная кнопка Volume.
Не обещается звуковое событие при остановленном LVGL worker или deep sleep без reboot.

## Порядок ручной приёмки владельцем

1. Скомпилировать/запустить C-сценарии по tests/README.md и сохранить полный вывод.
2. Выполнить существующую build-команду BUILDING.md именно для своей PCB/silicon,
   сохранив текущие defaults, ABI fixture и pinned SDK. Записать размер app и остаток
   standalone factory slot 0x300000; не менять partition table для обхода переполнения.
3. Прошить/запустить привычным способом владельца и выполнить T01–T16, U01–U14.
   Отдельно отметить результат power-loss, визуальной плавности, touch и звука.
4. Передать compiler output, снимки UI и наблюдения. Только после них выставлять
   текущие Build/Device PASS. Остальные профили и реальная host integration остаются NOT RUN.

## Изменённые и добавленные файлы раскладки A

Полный список этого изменения (24 файла):

| Группа | Файлы | Назначение |
| --- | --- | --- |
| Core | `components/kitchen_core/kitchen_core.c`, `components/kitchen_core/include/kitchen_core.h`, `components/kitchen_core/CMakeLists.txt` | 6 слотов и переходы |
| Codec и координатор | `components/kitchen_core/kitchen_settings.c`, `components/kitchen_core/include/kitchen_settings.h` | Формат, validation, tickets, Retry |
| UI | `components/kitchen_ui/kitchen_ui.c`, `components/kitchen_ui/include/kitchen_ui.h` | Сетка, overlay, редакторы, input guard, внутренняя подсветка |
| App | `components/kitchen_app/kitchen_app.c` | Загрузка, точки записи, опрос сохранений |
| Platform | `components/kitchen_platform/kitchen_platform.c`, `components/kitchen_platform/include/kitchen_platform.h`, `components/kitchen_platform/CMakeLists.txt` | Optional storage group и её проверка |
| Standalone | `main/standalone_storage.c`, `main/standalone_storage.h`, `main/standalone_runner.c`, `main/CMakeLists.txt` | NVS worker и подключение |
| Bridge | `integration/kitchen_host_bridge.c` | Явный RAM fallback без host storage |
| Сценарии | `tests/kitchen_tests.c`, `tests/README.md` | Ручные C11 сценарии с fake backend |
| Документация | `README.md`, `docs/TIMERS.md`, `docs/VALIDATION.md`, `docs/ARCHITECTURE.md`, `docs/INTEGRATION.md`, `docs/BUILDING.md` | Поведение, ABI, команды и текущие статусы |

BSP, display/touch, audio source/assets, fonts, sdkconfig, partitions, dependency lock,
root CMake и аппаратные профили не изменены. Build/flash/monitor не выполнялись.
Коммит, ветка и push не создавались.

## Исторические проверки до раскладки A

Ниже — архив прежних исходников и результатов. Он не задаёт текущие интерфейсы,
число таймеров, хранение или визуальное поведение; для них см. TIMERS.md и разделы выше.

# Проверка host-compatible kitchen

Дата: 30.10.2026. Приоритет: [HOST_COMPAT_CORRECTION](BASELINE_HOST_COMPAT_CORRECTION.md).
Исходный BASELINE_SPEC помечен superseded для стека/архитектуры.

## Текущая реализация таймеров

### Текущая правка: прямой fullscreen, detail и новые значки

По запросу пользователя старый тестовый экран отключён от main и startup dependency;
нет Fixture / Full UI, brightness и lifecycle controls. Исторические проверки
Fixture ниже относятся к предыдущим версиям и не являются инструкциями текущего UI.

Статически найдено: update_card считывал lv_obj_get_width до завершения layout
и передавал результат в position_card. Теперь геометрия хранится в самой карточке,
включая промежуточные кадры анимации; detail при обновлении не теряет размер 448 px.

После ручной сборки проверить:

1. После включения сразу виден кухонный экран, тестовых кнопок нет.
2. Saved timers закреплена справа снизу под New timer. Три больших таймера и их
   кнопки полностью видны; нижний центральный заканчивается на y=580.
3. Открыть running/paused/done/saved: слева виден круг, время и имя, справа управление.
   Поменять остаток, подождать перехода секунды и открыть Remaining time/Duration:
   круг не исчезает. Проверить анимации 2↔3↔4 и повторные открытия detail.
4. Stockpot белая; размеры glyphs в выборе 48 px, на карточке 40 px, кружок 64 px.
   Проверить Stockpot/Cup/Pan/Oven и No icon в New timer и Edit timer, обрезания нет.
5. Проверить запуск saved, возврат стрелкой, клавиатуру и доступность Save/Cancel.
6. Hide/suspend/destroy/parent deletion теперь проверяются на отдельном host/harness
   этапе; старого тестового экрана для этих действий в текущем приложении нет.

Новая правка — source/static only; сборка и runtime ожидаются от пользователя.

### История: возврат из Fixture (экран впоследствии удалён)

Пользовательский лог подтвердил сборку и запись предыдущего варианта HW V1.0 /
P4 legacy: app 0x8a0d0 (565456 байт), запуск LVGL 8.4.0/direct/avoid_tearing.
Пользователь сообщил, что возврат в UI не работает; в предоставленном фрагменте
нет crash/backtrace или сообщения об ошибке lifecycle.

В исходниках найдено: Full UI только менял высоту контейнера, а Open при живом
экземпляре и parent сразу возвращал ESP_OK. После Back/Hide сохранялось скрытое
состояние, после Suspend — флаг suspended. Теперь Open восстанавливает существующий
экземпляр через on_before_resume/restore/on_view_change; вход в Full UI использует
этот же путь. При отсутствии parent UI пересоздаётся в существующем instance.
Новая правка проверена только статически; пользовательской сборки после неё нет.

После ручной сборки проверить:

- Fixture → Back → Full UI и Fixture → Hide → Full UI: кухонный экран виден.
- Fixture → Suspend → Full UI: UI снова реагирует на касания.
- Back/Hide/Suspend → Open: существующий UI восстановлен; затем Full UI даёт 600 px.
- Parent del → Full UI: UI создан заново, карточки сохранены.
- Close → Full UI: создан новый экземпляр с пустой моделью.
- Таймер running учитывает время скрытия; пользовательский paused остаётся paused.

**Текущий код таймеров: source/static only, пользовательская сборка и панель UNTESTED.**
Прежние Build PASS ниже относятся к baseline до добавления таймеров и новых шрифтов.
Статически просмотрены команды, дедлайны, порядок карточек, UI lifetime, границы
массивов и ссылки на API локального LVGL 8.4. Компиляция и runtime не выполнялись.

Ручная приёмка текущего варианта:

1. Собрать вручную по BUILDING.md для своей подтверждённой board/LCD/silicon
   конфигурации; предоставить полный вывод, размеры app/bootloader и resolved versions.
2. В Full UI проверить 0, 1, 2, 3, 4, 5, 6 и 20 карточек; круги 280/190, три крупных без прокрутки, прокрутку при >5,
   закреплённую навигацию, сохранение scroll active/saved и читаемость с 4–5 метров.
3. Проверить 00:00:00 (запрет), 00:00:01, 00:59:59, 01:00:00 и 23:59:59; пресеты,
   клавиатуру только для имени, пустое имя/No icon, 32 ASCII-символа и длинное имя.
4. Проверить Pause/Resume с долями секунды, все ±3/5/10, блокировку неположительного
   результата и верхней границы; Set remaining time без смены пользовательской паузы.
5. Установить старт 10 min, добавить 5 min во время работы: Repeat должен предлагать
   10 min. Saved Set duration меняет следующий запуск. Cancel/Acknowledge перемещают
   в начало saved, Start — в конец active, Repeat/дополнительное время оставляют позицию.
6. Открыть Remaining time перед завершением: барабаны не сбрасываются, появляется
   Start timer; Cancel оставляет done. Проверить нажатие Apply на границе дедлайна.
   Открытый Edit timer при завершении закрывается без сохранения черновика.
7. Убедиться, что done показывает 00:00 → 00:01 без плюса, полный цветной контур;
   карандаш скрыт. Проверить +3/5/10, Set duration, Repeat и Acknowledge.
8. Проверить долгий перенос, краткое касание и прокрутку раздельно, автопрокрутку
   обоих краёв, контур назначения, отпускание и отмену при потере касания. Проверить
   анимации 2↔3↔4↔5, исчезновение, заполнение ячейки и отсутствие пересортировки по срочности.
9. Запустить короткий таймер, скрыть UI или вызвать host suspend дольше его остатка,
   вернуть UI: done/elapsed должны учитывать всё прошедшее время. Пользовательский
   Pause при hide/suspend должен сохранить остаток. Звука и пробуждения не ожидается.
10. На отдельном host/harness этапе проверить удаление parent → create_content с сохранённой моделью; Close (stop + destroy)
    → Open с пустой моделью. Stop без destroy сохраняет модель живого экземпляра. Выполнить 20 open/close и 20 hide/show, проверить отсутствие
    поздних callbacks и роста памяти на одинаковых стадиях. Измерить heap/GUI stack
    с 20 карточками, клавиатурой и во время анимации.

Сборка другой платы и standalone PASS не заменяют отдельную embedded-приёмку.

## Раздельные доказательства baseline

| Уровень | Текущее состояние |
| --- | --- |
| Source/static | просмотрено, проверки ниже |
| User standalone build | Предыдущий partial: PASS; direct + avoid_tearing: ожидается сборка |
| Standalone device | Partial: тесты подтверждены, позже отмечено подрагивание; direct: UNTESTED |
| Embedded host integration/build/device | UNTESTED, хост не подключён и не изменён |

Агент не запускал firmware build/compile, flash/monitor/esptool/OpenOCD/JTAG.
Пользователь предоставил полный успешный вывод сборки HW V1.0 / P4 legacy:
`Project build complete`. Build PASS относится только к этому standalone-сочетанию.
Signed-image размеры хоста и совместное HA/audio/microphone потребление памяти
пока не подтверждены.

### Пользовательская сборка HW V1.0 legacy

SDK 6.0.2 активирован, app_base.h подготовлен; его SHA-256 совпадает с ожидаемым.
Резолюция: IDF 6.0.2, LVGL 8.4.0, esp_lvgl_port 2.8.0~1, EK79007 1.0.4,
GT911 1.2.0~3, esp_lcd_touch 1.2.1, cmake_utilities 0.5.3. Настоящий lock сохранён
без ручного изменения; он остаётся untracked до отдельного разрешения на commit.

Причина CONFIGURE FAIL: отдельный процесс раннего сбора requirements не получает
обычную cache-переменную KITCHEN_HOST_CONTRACT_DIR. Fixture теперь объявляет на раннем
проходе только REQUIRES; путь и SHA-256 по-прежнему проверяются на основном проходе.
Повторный пользовательский вывод подтвердил успешные configure/generate после исправления.
Generated build_properties.temp.cmake и SDK агентом не изменялись.

Следующая остановка: crowpanel_board.c, `-Werror=type-limits` при сравнении
беззнакового chip.revision с CONFIG_ESP_REV_MIN_FULL=0. Сравнение с нижней границей
теперь включается только при MIN_FULL>0; проверка модели и верхней границы остаётся
без изменений. Для v3 нижняя граница 300 по-прежнему проверяется. Исправление
просмотрено статически; повторный пользовательский вывод больше не содержит этой ошибки.

Следующий COMPILE FAIL: crowpanel_bsp.c, передача адреса const vendor в поле
esp_lcd_panel_dev_config_t.vendor_config типа void*. У локального vendor убран const;
casts и подавление предупреждений не добавлены. Драйвер EK79007 1.0.4 использует
структуру во время constructor и сохраняет нужные поля отдельно, её lifetime не меняется.
Предупреждение deprecated esp_lcd_touch_get_coordinates относится к esp_lvgl_port,
не является причиной остановки; managed-код не изменялся. Агент сборку не запускал,
последующий успешный пользовательский build подтвердил устранение ошибки.

### Результат успешной сборки

Каталог: `build-compat-hw1.0-p4legacy`. Defaults:
`sdkconfig.defaults;config/board-v1.0.defaults;config/lcd-legacy.defaults;config/silicon-legacy.defaults`.

| Артефакт | Размер | Свободно в выделенной области |
| --- | --- | --- |
| kitchen-addon.bin | 0x7f360 (521 056 байт) | 0x280ca0 (2 624 672 байт), 83% слота 0x300000 |
| bootloader.bin | 0x5a70 (23 152 байт) | 0x590 (1 424 байт), 6% по отчёту сборки |

Значения взяты из предоставленного вывода. Partition table соответствует standalone:
NVS 0x9000/24K, PHY 0xf000/4K, factory 0x10000/3M. Сообщения SDK о Kconfig bool/hex
и локали не остановили сборку. Это не результат прошивки, device test или host integration.

### Пользовательская прошивка и первый запуск

На /dev/cu.wchusbserial110 записаны bootloader/partition table/app; esptool подтвердил
хеши трёх областей. ROM/bootloader/app сообщают фактическую chip revision 1.3 — это
silicon legacy, не номер PCB. Выбран профиль HW V1.0; физическая диагональ по логу
не устанавливается. CPU 360 МГц, PSRAM 32 МиБ / 200 МГц, memory test OK. GT911 вернул
ID 911 и config version 130. Это подтверждает ответ контроллера, не touch acceptance.

Остановка startup: ledc_set_duty_and_update требует fade service, не установленный
в standalone BSP. Заменён на ledc_set_duty + ledc_update_duty: без fade allocation,
с прежним PWM mapping и сериализацией вызывающим display_port/UI lock. Cleanup
после остановки выполняется после удаления UI; конкурентных brightness callbacks нет.
Build/device результаты после изменения ещё не предоставлены; агент их не запускал.

Сопутствующие сообщения:

- esp_lvgl_port при rotation=0 безусловно вызывает swap_xy(false), неподдерживаемый
  EK79007. Ошибка не остановила регистрацию, но изображение/ориентация требуют проверки.
  Managed-код не изменялся, fake driver hooks не добавлялись.
- Flash физически 16 МиБ, image header 4 МиБ. SDK использует 4 МиБ согласно минимальному
  standalone-профилю; эта разница не вызвала остановку.
- ESP_ERR_NOT_SUPPORTED при cleanup — задокументированное удержание живого port/hardware
  до reset; первичная причина остановки — LEDC, не этот cleanup status.

## Статический просмотр

- Сверены app_base/app_manager ABI, реальный jobs_queue, порядок callbacks/container
  deletion, main registration, BSP lock/lifecycle, публичные audio/microphone/settings/sleep
  интерфейсы и lock актуального Downloads snapshot. SHA-256 — в SOURCES.
- Сверены точные Registry-заголовки/реализации LVGL 8.4.0, port 2.8.0~1, EK79007 1.0.4,
  GT911 1.2.0~3 и локальный IDF 6.0.2. Все defaults-символы найдены текстовым поиском
  Kconfig, все используемые lv_/lvgl_port функции найдены в заголовках этих выпусков.
  Это не CMake/Kconfig/resolver/компилятор.
- Просмотрены CMake closure четырёх kitchen-компонентов: нет собственного BSP,
  display_port/port init, network/NVS/audio services, main или standalone fixture.
  Только kitchen_ui требует LVGL >=8.4,<8.5; standalone exact pins находятся в main.
- Просмотрены parent ownership, LV_EVENT_DELETE до удаления детей, motion cancellation,
  идемпотентный stop/destroy, синхронные UI events без async instance pointers.
- Fixture queue ограничена 8 scalar payloads; overflow отвергается, старые generation
  не исполняются. Нет task joins, network/flash I/O и synchronous manager calls под GUI-lock.
- Просмотрены startup failure paths: до port partial BSP освобождается; после port
  подсветка гасится, handles сохраняются до reset из-за асинхронного vendor deinit.
- Проверены standalone flash/framebuffer арифметика, CMake source paths, Markdown links,
  восемь новых команд/defaults, отсутствие активного LVGL9/adapter кода и whitespace.
- Хеши прочитанных host-файлов после изменений совпадают с исходным снимком; hardware
  donors не изменялись. keys не читались/копировались, host ABI в поставку не включён.

Не выполнялись tests/linters/analyzers, способные скомпилировать проект. Python
проверки только читают/разбирают текст и пути; не импортируют firmware build tools.

## Ручная standalone-проверка

Подготовить ABI header и собрать по [BUILDING](BUILDING.md) с новым sdkconfig.
Зафиксировать resolved versions/lock, размеры app/bootloader и полный вывод.
Проверить отказ при несовпадении ABI hash, отсутствии board/LCD/silicon, несовместимой
PCB/LCD паре или silicon branch/min, LVGL9/другом SDK и неправильной глубине цвета.

| PCB | Silicon | Standalone build | Device |
| --- | --- | --- | --- |
| V1.0 | legacy | Partial PASS; direct UNTESTED | Partial: тесты подтверждены, плавность с замечанием; direct UNTESTED |
| V1.0 | v3 | UNTESTED | UNTESTED |
| V1.1 | legacy | UNTESTED | UNTESTED |
| V1.1 | v3 | UNTESTED | UNTESTED |
| V1.2 | legacy | UNTESTED | UNTESTED |
| V1.2 | v3 | UNTESTED | UNTESTED |
| V1.3 | legacy | UNTESTED | UNTESTED |
| V1.3 | v3 | UNTESTED | UNTESTED |

На каждой доступной плате:

1. Сохранить PCB/диагональ/фактическую chip revision, выбранные defaults, SDK/lock,
   startup log. Три холодных запуска и три reset, изображение RGB/белый/чёрный,
   текст/движение, отсутствие driver errors. Partial rendering может выявить tearing;
   это фиксируется как открытая аппаратная проблема, а не скрывается.
2. Touch в четырёх углах и центре, pressed/released и корректные координаты. DEMO step
   увеличивает local counter/time без HA соединений. UI Back скрывает view через fixture intent.
3. Яркость стенда 100%, промежуточная, 0% и восстановление 70% через 5 секунд.
   Kitchen не меняет глобальную яркость: footer controls принадлежат baseline_ui.
4. Кнопка `20 cycles`: минимум 20 open/close и 20 hide/show. Сравнить память после
   прогрева на одинаковых стадиях до create/после open/после close/в конце.
   Проверить, что скрытая view не анимируется; дополнительных tasks/subscriptions нет.
5. Отдельно Suspend → Resume; Hide → Suspend → Resume → Show, Back → Show,
   повторные Close/Open. Состояние demo сохраняется при hide/suspend, новый instance
   после Close получает начальную модель.
6. Parent del → Open, Parent del → Close, Parent del → Close → Open. Не должно быть
   double-free, обращений к старым детям, поздних animation/events после destroy.
   Parent остаётся ресурсом fixture, компонент удаляет только собственный root.
7. Быстрые последовательные controls до/после Close/Open и во время автопрогона:
   queue overflow/stale generation должны отвергаться без UAF. Проверить оставшийся
   UI после цикла, memory/GUI stack HWM, минимум 30 минут движения без reset/WDT.

Аппаратный port здесь не переинициализируется: его lifetime — до reset. Полное
освобождение hardware/port не является результатом kitchen open/close теста.
Для V1.3 9″ и 10.1″ отдельные отчёты; отсутствующие панели остаются UNTESTED.

## Будущая интеграционная приёмка

Только после отдельного подключения из [INTEGRATION](INTEGRATION.md): host build
на его lock/SDK, сравнение реального signed image до/после и запаса в 0x680000 слотах;
HA↔kitchen, Back/minimize, sleep/wake, Wi-Fi transitions, разрешённые HA video/audio/
microphone sessions. Проверить общую память/stack и отсутствие влияния на host
brightness/settings/OTA/signing/encryption/storage. Fixture не доказывает эти пункты.

## Форма отчёта

```text
Дата / тестировщик / исходники:
Режим: standalone / embedded
PCB / диагональ / фактическая chip revision:
SDK / LVGL / port / LCD / touch / lock / ABI hash:
Build-каталог / defaults / sdkconfig / вывод / размеры:
User build: PASS / FAIL / UNTESTED
Изображение / touch / яркость стенда / cold boots / resets:
20 open-close / 20 hide-show / suspend-resume / Back:
Parent-first deletion / stale controls / callbacks после destroy:
Internal/SPIRAM free-largest на сравнимых стадиях:
GUI stack HWM / длительность / reset-WDT-driver errors:
Standalone device: PASS / FAIL / UNTESTED
Embedded integration: PASS / FAIL / UNTESTED
Signed-image delta / slot margin (только будущий host build):
Логи / фото / ограничения:
```

## Файлы этой корректировки и Git

Ни один исходный компонент не удалён: аппаратные board/BSP, defaults профилей и
partitions.csv сохранены. Изменённые и добавленные файлы относительно состояния
перед этой корректировкой перечислены ниже (в Git весь проект пока untracked).
HOST_COMPAT_CORRECTION.md не изменялся. Commit/push/ветки не создавались.

| Файл | Изменение |
| --- | --- |
| `.gitignore` | Изменён: игнорирование приватного ABI fixture |
| `CMakeLists.txt` | Изменён: IDF6.0.2 и отдельный standalone ABI fixture |
| `README.md` | Изменён: host-compatible цель и актуальные evidence gates |
| `components/baseline_ui/CMakeLists.txt` | Изменён: зависимости/границы baseline_ui |
| `components/baseline_ui/baseline_ui.c` | Изменён: LVGL8 standalone footer, яркость и диагностика |
| `components/baseline_ui/include/baseline_ui.h` | Изменён: публичный C-контракт baseline_ui |
| `components/crowpanel_board/Kconfig` | Изменён: удалена неприменимая adapter PPA опция |
| `components/display_port/CMakeLists.txt` | Изменён: зависимости/границы display_port |
| `components/display_port/display_port.c` | Изменён: LVGL8 esp_lvgl_port, малый draw buffer и безопасное удержание hardware |
| `components/display_port/include/display_port.h` | Изменён: публичный C-контракт display_port |
| `docs/ARCHITECTURE.md` | Изменён: новые границы и lifecycle/lock контракты |
| `docs/BASELINE_SPEC.md` | Изменён: явная superseded пометка исходного стека |
| `docs/BUILDING.md` | Изменён: ABI preparation и восемь новых команд IDF6.0.2 |
| `docs/HARDWARE_PROFILES.md` | Изменён: профили только standalone и новый render режим |
| `docs/SOURCES.md` | Изменён: host snapshot hashes, версии, API и происхождение контрактов |
| `docs/VALIDATION.md` | Изменён: раздельные проверки, ограничения и эта ведомость |
| `main/CMakeLists.txt` | Изменён: standalone runner и зависимости |
| `main/app_main.c` | Изменён: запуск совместимого standalone fixture |
| `main/idf_component.yml` | Изменён: точные версии host-совместимого стека |
| `sdkconfig.defaults` | Изменён: настройки LVGL8 вместо LVGL9 |
| `components/kitchen_app/CMakeLists.txt` | Добавлен: зависимости/границы kitchen_app |
| `components/kitchen_app/include/kitchen_app.h` | Добавлен: публичный C-контракт kitchen_app |
| `components/kitchen_app/kitchen_app.c` | Добавлен: реальный descriptor ABI, lifecycle и владение instance |
| `components/kitchen_core/CMakeLists.txt` | Добавлен: зависимости/границы kitchen_core |
| `components/kitchen_core/include/kitchen_core.h` | Добавлен: публичный C-контракт kitchen_core |
| `components/kitchen_core/kitchen_core.c` | Добавлен: локальная demo-модель без LVGL/SDK |
| `components/kitchen_platform/CMakeLists.txt` | Добавлен: зависимости/границы kitchen_platform |
| `components/kitchen_platform/include/kitchen_platform.h` | Добавлен: публичный C-контракт kitchen_platform |
| `components/kitchen_platform/kitchen_platform.c` | Добавлен: валидация инъецируемых lock/time/navigation операций |
| `components/kitchen_ui/CMakeLists.txt` | Добавлен: зависимости/границы kitchen_ui |
| `components/kitchen_ui/idf_component.yml` | Добавлен: диапазоны IDF/LVGL, совместимые с host |
| `components/kitchen_ui/include/kitchen_ui.h` | Добавлен: публичный C-контракт kitchen_ui |
| `components/kitchen_ui/kitchen_ui.c` | Добавлен: единственный LVGL8 UI в borrowed parent |
| `docs/INTEGRATION.md` | Добавлен: будущая явная регистрация, CMake и host limits |
| `integration/kitchen_host_bridge.c` | Добавлен: неприменённый host lock/time/navigation bridge |
| `integration/kitchen_host_bridge.h` | Добавлен: C-linkage контракт будущего bridge |
| `main/standalone_runner.c` | Добавлен: bounded queue и lifecycle fixture по реальному ABI |
| `main/standalone_runner.h` | Добавлен: контракт запуска fixture |
| `standalone/app_manager/CMakeLists.txt` | Добавлен: header-only fixture, без реализации manager |
| `tools/prepare_host_contract.py` | Добавлен: явная пользовательская подготовка одного ABI header с hash |

## Подтверждение завершения standalone-тестов

После PWM-исправления пользователь предоставил успешные build/flash/startup logs:
app 0x7f030 (520 240 байт), свободно 0x280fd0 (83% app slot), фактический silicon
P4 v1.3, профиль HW V1.0 legacy. Fixture сообщил `20 open/close + 20 hide/show: ESP_OK`.
В коротком логе PSRAM free/largest оставались 31 010 760 / 30 932 992 байта;
GUI stack HWM достиг 5900 байт, reset/WDT не наблюдались.

После перечисления оставшихся ручных проверок (изображение/ориентация/touch,
яркость и восстановление после 0%, suspend/resume, Parent del, холодные включения,
30 минут работы) пользователь подтвердил: «тесты пройдены».
Device PASS зафиксирован как сообщение пользователя, без выдуманных дополнительных
измерений, длительных логов или фото. Диагональ и серийный номер не предоставлены;
результат относится только к текущей испытанной плате с выбранным профилем.
Предыдущая startup-ошибка LEDC выше сохранена как история исправления.
Другие PCB/silicon/диагонали и embedded integration остаются UNTESTED.

## Сравнительный direct + avoid_tearing вариант

По запросу пользователя standalone переключён с partial/40 строк на direct_mode,
double_buffer и avoid_tearing. Локальные исходники esp_lvgl_port 2.8.0~1 подтверждают:
ветка avoid_tearing заимствует два кадра DPI, не выполняя allocation draw buffers;
DSI использует on_refresh_done и semaphore ожидания кадра. BSP уже создаёт ровно два
кадра и проверяет их PSRAM pointers. Full refresh=false, rotation=0, PPA/DMA2D off.

Изменение просмотрено только статически. SDK/библиотеки, частоты, GUI task scheduling,
optimization flags, kitchen UI и host не менялись. Runtime teardown hardware не добавлен;
при ошибке display registration применяется abort из-за vendor error-path ownership.

Владелец повторяет прежнюю build/flash команду для build-compat-hw1.0-p4legacy и
сохраняет новый startup log с `direct_mode=on avoid_tearing=on` и отсутствием extra draw
buffer. Сравнить движение той же сцены с partial-вариантом: ровность скорости,
контуры/разрывы, touch latency, memory free/largest и GUI HWM. Отдельные 81 920 байт
partial buffer больше не нужны, но точную разницу heap нужно измерить после прогрева.
Повторить 20 open/close, 20 hide/show, suspend/resume, Parent del, brightness 0% recovery,
холодные старты/reset и 30 минут наблюдения без WDT/driver errors. При registration
abort сохранить panic log. До этих результатов новый вариант не имеет Build/Device PASS.

## Визуальная коррекция по эскизам (после замечания пользователя)

Исходники: Inter вместо семисегментных цифр, Google Material Symbols Outlined
вместо самодельных линий/Font Awesome, пиктографические кнопки выбора, группировка
New timer, увеличенный круг detail 448 px и заголовки Inter Bold. Правила состояний
и отсчёта не изменялись. Источники/лицензии — kitchen_ui/assets/README.md.

Статическая геометрия трёх карточек 280 px: в content y=76…600 верхние x=32/712,
y=8, нижняя x=372,y=168. Нижняя кнопка заканчивается на абсолютном y=588;
Saved timers находится справа на x=764,y=542 и не перекрывает центральную карточку. Обёртки карточек по горизонтали не пересекаются.
Растеризация шрифтов и SVG/TTF ассетов не является firmware build или runtime test.

Пользовательская проверка после ручной сборки: сравнить 1–5, New timer, четыре
состояния detail и Edit timer с А.1–А.12; подтвердить отсутствие прокрутки при трёх,
отсутствие обрезания H:MM:SS и значков, доступность Save/Cancel/Start timer при
клавиатуре. Повторно проверить анимации 2↔3↔4 и расход flash/heap с новыми шрифтами.
Текущие визуальные изменения на устройстве пока UNTESTED.

## Срочность и штатные символы LVGL

Новая правка source/static only: внутренняя рамка 2 px заменена внешним outline
6 px с зазором 2 px. Прозрачность 140…240/255 изменяется по сглаженному циклу
2200 мс на существующем UI tick; новых таймеров и задач нет. Подсветка не меняет
геометрию дуги, порядок карточек или точность отсчёта.

Управляющие пиктограммы используют LV_SYMBOL_* и штатный bundled Font Awesome
из локального LVGL, без Google-версий. Google оставлен только для четырёх кухонных
предметов. PLUS/MINUS также применены к кнопкам изменения времени; STOP включён
в native subset, отдельное новое действие не добавлено.

После ручной сборки проверить: у ближайшего running рамка заметна и мягко пульсирует;
после Pause рамка переходит следующему running, порядок карточек сохраняется;
на Done рамка остаётся, после Acknowledge пропадает. Проверить отсутствие обрезания
рамки на верхней строке и в detail, а также значки Pause/Play/OK/Close/Plus/Minus/
Up/Down/Edit/Trash/Back/Repeat на светлых и тёмных кнопках и No icon в редакторе.

## Коррекция оформления 31.10.2026 — текущая версия

Заменила предыдущую пульсирующую рамку: используется тень вокруг круга,
24 px blur, spread −12…−5 px, opacity 0…220, сглаженный период 1200 мс.
Обновление остаётся в существующем UI timer, без новых задач и LVGL timers.
No icon возвращён к перечёркнутому кругу; поверхности нейтральные, без синего оттенка.
Ширина кнопок списков определяется по реально используемому шрифту, а не по ещё
не рассчитанному layout. Заголовок Saved timers теперь слева сверху; переходы
между списками справа с зеркальными отступами сверху/снизу.

Пользовательская проверка после ручной сборки: убедиться, что ореол расширяется
и сжимается, а не только мигает; проверить отсутствие обрезания сверху, читаемость
кнопок по содержимому и симметричную навигацию. Сравнить New timer/Edit timer при
открытой клавиатуре, проверить No icon и минимум зоны касания 48×48. Проверить
плавность с 20 завершёнными карточками: отрисовка тени требует больше работы, чем
изменение opacity рамки. Аппаратной проверки этой правки пока нет.

## Полное скрытие ушедших карточек и кнопки-капсулы

Исходная правка: перед уходом карточки отменяется её геометрическая анимация.
По завершении fade-out карточка явно получает HIDDEN, opacity=0; область списка
однократно инвалидируется целиком, включая старые положения и выступающие тени.
При прерывании fade-out скрытием UI повторный sync без анимации также завершает
уход карточки, даже если present уже false. Сетка при этом не пересоздаётся.
Кнопки под кругами имеют форму капсулы, ширину glyph +32 px полей и зону касания
не меньше 48×48 px.

Ручная проверка после пользовательской сборки: Acknowledge на основном экране
при 1/3/4/5 таймерах, несколько быстрых подтверждений во время перестроения,
переход Timers → Saved → Timers до конца fade, повторный запуск saved. Убедиться,
что дуга/значок/тень не остаются в старой или промежуточной позиции. На host/harness
дополнительно проверить hide/show во время fade-out. Проверить форму и размеры
Pause/Resume/Acknowledge/Start. Сборка и runtime этой правки агентом не выполнялись.

## Новое правило срочности и возврат перемещения (31.10.2026)

Текущая правка заменяет прежнее выделение одного ближайшего таймера и тень:
каждый RUNNING при 0 < remaining_ms <= 60000 получает outline 6 px, pad 2 px,
сглаженный цикл opacity 120…255 за 1200 мс. PAUSED/DONE/SAVED не пульсируют.
Порядок/размеры/цвет карточек сохраняются. Уход в Saved больше не отменяет
перемещение в начале fade-out; финальное скрытие и инвалидирование viewport
сохранены. Repeat использует Google Replay из кэшированного исходного font.

После пользовательской сборки проверить одновременно 3 running с остатками
59/60/61 s: первые два выделены, третий получает рамку на пороге 60 s. Проверить
Pause/Resume короткого отсчёта, добавление времени выше 60 s, истечение до DONE;
рамка должна исчезать/возвращаться без перестановки. Сверить это в списке и detail.
Проверить Replay; несколько быстрых Acknowledge во время перестроения, полное
исчезновение ушедшей карточки и её краёв после 220 мс; Saved → Start → Timers.
Правка проверена статически, firmware build и runtime не выполнялись.

## Вход в Saved и увеличение 4–5 карточек (31.10.2026)

Source/static: обычное открытие списка больше не запускает геометрическую анимацию.
Saved всегда сразу получает целевые позиции и сохранённый scroll. Одноразовая
анимация входа в Active выставляется успешным Create/Start/Cancel/Acknowledge из
detail; Start с компактной saved-карточки также выставляет этот флаг. Перенос и
сокращение уже открытого active продолжают анимироваться.

Круги увеличены 190 → 240 px, MM:SS 48 → 64 px, H:MM:SS 36 → 48 px. Фиксированные
позиции четырёх и пяти проверяются с учётом дуги, рамки 8 px снаружи, значка 64 px,
кнопки с минимальной высотой 48 px и нижней правой навигации.

Ручная проверка: несколько раз открыть Saved → Active → Saved и detail → Back,
включая быстрый уход во время перестроения: при обычном входе нет повторной анимации.
Start saved и New timer должны по-прежнему плавно перестраивать Active. Проверить
4/5 с именами, значками и состояниями PAUSED/DONE, таймеры 1:00:00/23:59:59,
пульсирующие рамки, доступность Saved timers, переходы 3↔4↔5↔6 и reorder/scroll
при 6–20. Сборка и устройство после этой правки агентом не проверялись.

## Loading, New timer и detail: текущая правка 31.10.2026

Source/static only. В Leader прочитаны ui_loading_begin/set_progress/finish и
порядок включения подсветки; сам Leader не изменён. Kitchen использует собственный
standalone overlay, без импорта host resources, BSP или сервисов Leader.

Ручная проверка после сборки:

- Холодный запуск: первым видимым кадром должен быть чёрный Loading со статусом
  и полосой, затем мягкий переход в Timers. Никаких вспышек пустой сцены или раннего
  показа приложения; overlay не принимает действия Kitchen и полностью удаляется.
- New timer: выросшие барабаны/пресеты/поле/действия полностью помещаются; заголовки
  Name/Icon белые, «· optional» светло-серое. No icon не пересекает glyph; проверить
  центрирование выбранных и невыбранных значков, включая border 2 px.
- Имя: пустое, 32 символа, очистка; клавиатура не перекрывает Cancel/Start/Save,
  закрытие клавиатуры возвращает действия на исходные позиции. Edit timer — отдельно.
- Running/paused/done/saved detail: увеличенные кнопки и надписи справа, glyph 64 px
  в кружке 96 px, имя 32 px ниже времени. Проверить длинное имя, H:MM:SS, смену секунды,
  ±, Pause/Resume, точный редактор и завершение при открытом редакторе. Слева ничего
  не съезжает вверх и не исчезает. Крупный текст и кнопки не обрезаются.
- Измерить heap/stack/flash после новых glyphs, во время startup fade и после удаления
  overlay. Плавность полноэкранного fade и новых барабанов подтверждается на панели.

Агент не выполнял firmware build, flash, monitor или runtime validation.

## Disabled-фон и тень — текущая правка 31.10.2026

Source/static only: отключены disabled color filter и изменение bg opacity;
надписи/значки приглушены до #7D7D84, фон кнопки остаётся normal/checked.
Существующая рамка сохранена, добавлена пульсирующая тень blur=24,
spread=−12…3, opacity=0…200; цикл 1200 мс синхронен рамке. Текущий UI tick общий,
дополнительных задач/timers нет. Viewport сдвинут вверх на 12 px, координаты ячеек
относительно него компенсированы, поэтому положение карточек на экране неизменно.

Ручная приёмка после пользовательской сборки: нулевая длительность Start/Apply,
недопустимые −3/5/10, верхний предел +3/5/10 и 20-card New timer — фон сохраняется,
серой становится только подпись. При возвращении доступности исходный цвет текста
восстанавливается, нажатие снова выполняет действие. Проверить оранжевые и серые
кнопки, а также значки компактных карточек во время ухода в saved.

Проверить несколько RUNNING ≤60 s: заметно расширяются/сжимаются тени и пульсируют
рамки; Pause/Done/добавление времени >60 s убирают оба эффекта. Проверить верхнюю
строку при 4/5, края экрана, Saved timers и очистку следов при Acknowledge. Сравнить
позиции 1–5 и drag preview с предыдущим вариантом. При 20 коротких таймерах проверить
плавность и свободную память, поскольку тени требуют дополнительной отрисовки.
Сборка и работа на панели этой правки агентом не проверялись.

## Disabled 40%, scrollbar имени, Name/Icon (31.10.2026)

Source/static: текст и custom-drawn glyphs disabled имеют opacity 40% при прежнем
непрозрачном фоне. Textarea имени: scrollbar AUTO, scroll direction HOR, прежние
высота и вертикальные отступы. Заголовки разделены на белое Name/Icon и серый суффикс
«· optional», без recolor-команд и LONG_DOT. Общая модель времени не изменялась.

После пользовательской сборки проверить New timer и Edit timer: обе подписи
полностью видны; поле с пустым/коротким именем без полосы, длинное имя допускает
горизонтальную прокрутку, после очистки полоса исчезает; вертикальной полосы нет.
Проверить контраст disabled на оранжевом/сером фоне, glyphs и восстановление обычного
вида после изменения длительности. Сборка и аппаратная проверка агентом не запускались.

## Однократный звук, громкость и яркий halo — текущая правка

Source/static only. Прочитаны demo_bg_motion, demo_speaker_check, speaker BSP и
my_codec Product Explainer. В текущем дереве проверены локальные IDF 6.0.2 I2S
заголовки и LVGL 8.4, WAV metadata/размер/hash; другие проекты не изменены.

После пользовательской сборки:

1. Проверить resolving/build без codec packages; WAV embedded symbol разрешён,
   сообщить размеры app/bootloader, heap и стек worker при проигрывании.
2. На текущей физической плате подтвердить звук GPIO22/21/23, PA30 active-low,
   stereo оба канала, отсутствие щелчков/обрезания конца. Другие платы — отдельно.
3. Volume сверху справа: 0/1/50/100%, смена уровня во время звука; 0 не оставляет
   отложенных звуков после unmute. New timer снизу слева, Saved справа, без перекрытий.
4. Таймер 1 s: один chime, затем DONE и elapsed без повторов. Открывать/закрывать
   detail и списки после завершения — повторного звука нет. Acknowledge переводит
   в saved; Repeat/дополнительное время создают новый отдельный цикл завершения.
5. Завершить 2–3 одновременно: отдельные сигналы идут последовательно. Проверить
   20 таймеров, очередь, реакцию GUI при playback, отсутствие зависаний.
6. Оставить New timer/Edit/Volume/точный выбор/scroll/reorder открытыми при завершении:
   один звук, точный отсчёт, имена/порядок и пользовательская пауза сохранены.
7. На host/harness: hide/suspend при живом GUI worker, удаление parent, stop/start
   и destroy; нет app callbacks после destroy. Остановка GUI/deep sleep требует
   отдельной host-интеграции и не считается проверенной standalone.
8. Яркое свечение: 32…56 blur, −6…10 spread, 80…255 opacity, 39 px максимум наружу;
   RUNNING ≤60 s и DONE. Проверить заголовок поверх halo, края и удаление карточек,
   нагрузку при 20 DONE и собственный UI/model timer. PAUSED не пульсирует.

При ошибке audio init UI остаётся работоспособным, громкость disabled; проверить
диагностический лог. При переполнении очереди нет повторных попыток/цикла звука.
Агент не собирал проект, не прошивал и не взаимодействовал с устройством.

Лёгкая статическая проверка текущей правки: whitespace/delimiter review 17 текстовых
файлов без замечаний; новые LVGL slider/shadow/symbol API найдены в локальных 8.4
headers, I2S fields/write timeout и embed basename подтверждены IDF 6.0.2 source.
WAV побайтно совпадает с входным файлом. Существующий build-compat-hw1.0-p4legacy
config указывает BOARD_V10/LCD_LEGACY/SILICON_LEGACY, LV_USE_SLIDER и default
Montserrat14 включены; это чтение существующего config, а не результат новой сборки.
Повторная сборка, линковка и физический звук не проверялись.

## Backdrop dismiss, placeholder и внешнее касание клавиатуры

Source/static only: пересмотрены LVGL 8.4 placeholder part, current_target,
порядок event callbacks и обработка удаления объекта во время CLICKED.

После ручной сборки текущего профиля:

1. Edit: изменить имя/значок и нажать снаружи карточки — окно закрывается,
   черновик не применяется. Повторить с открытой клавиатурой. Cancel/крестик
   по-прежнему отменяют, Save применяет один раз. Завершение отсчёта не сохраняет черновик.
2. Volume: поменять уровень, нажать backdrop — окно закрывается, уровень остаётся.
   Касание пустого места внутри карточки и работа slider не закрывают Volume.
3. New/Edit: пустое имя показывает английский placeholder целиком; ввод убирает
   его, очистка возвращает; сохранение пустого имени не сохраняет подсказку.
   Ввод 32 ASCII и горизонтальная прокрутка работают как раньше.
4. В New/Edit открыть клавиатуру, набрать текст, нажать заголовок/пустое место
   внутри формы — она закрывается, текст сохраняется. Повторный tap по имени
   открывает её снова. Клавиши и касание поля не прячут её.
5. С открытой клавиатурой нажать Save/Cancel в Edit и Start/Cancel в New:
   действие срабатывает с первого tap, перемещение не теряет нажатие. В Edit
   выбор значка также применяется к черновику и скрывает клавиатуру.
6. Без клавиатуры нижний отступ Cancel/Save =16 px. С клавиатурой кнопки
   заканчиваются y=374, выше неё (y=380), после скрытия геометрия восстанавливается.
7. Повторить 20 открытий/закрытий и клавиатурных циклов: нет накопления callbacks,
   роста занятой памяти, случайного нажатия на underlying detail/список.

Проект не собирался; аппаратных команд и runtime-проверки агентом не было.

## Speaker-only volume и release preview

Статически: просмотрен volume_event_cb → DEMO_SFX_VOLUME_RELEASE и gain=0.40
в Product Explainer. PCM скопирован побайтно, проверены resample bounds и int64
interpolation (signed product исключает int32 overflow), LVGL transform API и diff.
Новых FreeRTOS задач/очередей и generated font изменений нет.

Вручную собрать текущий профиль и проверить:
- Timers: большой динамик без числа/процентов; диалог: 0%, 100%, текущий процент.
- Передвижение slider меняет уровень; отпускание даёт один знакомый volume beep.
- При 0% тишина; при 1/50/100% уровень корректен, нет щелчков или смены высоты тона.
- Быстрые повторные жесты не накапливают preview; DONE chime остаётся прежним,
  однократным, не обрывается preview. При занятом player preview ждёт своей очереди.
- Backdrop и Done закрывают диалог, после повторного открытия нет лишних callbacks.

Сборка, прошивка, мониторинг и физическая проверка агентом не выполнялись.

## Тень без outline и default volume 40%

Статически проверены diff, диапазоны smoothstep и синхронный default=40% в core/backend.
После ручной сборки проверить RUNNING ≤60 s и DONE: нет дополнительной рамки,
только мягкая пульсация тени с периодом 1.8 s; дуга прогресса сохранена. Проверить
PAUSED/SAVED без свечения и возврат из паузы. На 1/5/20 карточках оценить плавность,
отсутствие обрезания/следов. После reset/нового instance громкость 40%; в живом
instance закрытие/открытие Volume сохраняет выбранный уровень. Звуки завершения
и release preview работают с выбранным gain. Сборка и runtime агентом не выполнялись.

## Коррекция пропусков preview, speaker и стоимости halo

Подтверждено source: atomic pending отбрасывал новые preview до окончания предыдущего;
иконка использовала label transform; native draw_shadow рассчитывает/размывает corner
с размером shadow_width+radius. Связь этого blur с задержками панели вероятна по
наблюдению пользователя, без профилирования не доказана. Источник отсутствующей
иконки точно не установлен; новый DRAW_MAIN обходится без font/transform pipeline.

Лёгкая проверка: API LVGL 8.4, маски/границы arithmetic, дифф, ownership освобождения
на page/root delete и очередь без app/UI pointers. Cache выделяется LVGL malloc;
существующий HW1.0 build config использует custom malloc и SPIRAM preference для
больших allocations. Это чтение существующего config, новая сборка не выполнялась.

Ручная приёмка после сборки:
1. Динамик виден при старте, после возврата из Saved и после циклов Volume.
2. На 40% выполнить 10 отпусканий slider с интервалами 0.3/1.5 s, в том числе
   уводя палец за slider. Новый preview слышен/перезапускается, без пропуска каждого
   второго; RELEASED+PRESS_LOST не дают дубль одного жеста. 0% ожидаемо без звука.
3. Сравнить реакцию New/Volume на 1/2/3/5/20 пульсирующих карточках. Проверить
   1.8-s характер пульсации, цвет, отсутствие лишней рамки и швов масок.
4. В Volume фон карточек остановлен, slider/кнопки отзывчивы; закрытие обновляет
   остатки/DONE. Завершение при открытом Edit закрывает черновик без сохранения.
5. Завершить таймер при preview: completion не теряется, preview уступает ему.
6. 20 циклов list/detail/Volume и parent delete/recreate: стабильная память,
   отсутствие старых image-cache pointers, клиппинга/следов при fade/reorder/scroll.
7. Проверить largest free PSRAM block и heap, загрузку CPU/GUI задержку на панели;
   подтверждённое ускорение и качество масок требуют hardware измерений пользователя.

Сборка, прошивка и мониторинг не выполнялись; коммит не создавался.
