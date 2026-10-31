# Ручная сборка совместимого standalone

Все команды выполняет владелец. Агент не запускает SDK build, compiler, flash или monitor.
Используйте новые build-каталоги: старые LVGL9 build/sdkconfig не переиспользуются.

```bash
. /Users/sonyakuks/esp/esp-idf-v6.0.2/export.sh
cd /Users/sonyakuks/esp/kitchen_addon_forHA/kitchen-addon
idf.py --version
```

Ожидается ESP-IDF v6.0.2. Manifest закрепляет LVGL 8.4.0, port 2.8.0~1,
EK79007 1.0.4, GT911 1.2.0~3. При resolver/API ошибке сохраняйте полный вывод,
не меняйте версии автоматически. dependencies.lock не формируется вручную.

Текущий UI таймеров добавляет локальные bitmap-шрифты и widgets ARC/ROLLER/TEXTAREA/
KEYBOARD. Ранее успешная сборка DEMO не подтверждает текущую реализацию. После
ручной сборки выполнить новую матрицу в VALIDATION.md. Standalone открывает kitchen напрямую на 1024×600, без тестового экрана
и переключателя Fixture / Full UI. Приватный ABI fixture ниже остаётся нужен для
заголовка app_manager; это не тестовый экран.

## Явная подготовка приватного ABI fixture

Хостовый app_base.h не поставляется: вместо копирования хоста нужен один локальный
контрактный файл, SHA-256 которого сверяется скриптом и CMake. Скрипт не запускает
сборку, не читает keys и не копирует app_manager implementation. Он нужен только
standalone; реальный host использует свой app_manager/include/app_base.h.

```bash
python3 tools/prepare_host_contract.py \
  --host-root '/Users/sonyakuks/Downloads/crow-ha-fw-feature-panel-microphone@c3675d87075' \
  --output "$PWD/.host-contract"
```

Это приватная локальная копия неизменённого заголовка; `.host-contract` игнорируется
Git. Условия дистрибуции host-контрактов отдельно описаны в SOURCES. Агент скрипт
копирования не запускал. Если hash отличается — сначала нужен статический пересмотр ABI.

## Восемь новых конфигураций

Каждая команда предполагает **новый** build-каталог. Defaults не переписывают
существующий sdkconfig. Chip revision выбирается независимо от PCB. Команды не
доказывают физическую совместимость всех сочетаний; статусы пока UNTESTED.

### HW V1.0, P4 legacy

```bash
idf.py -B build-compat-hw1.0-p4legacy \
  -D KITCHEN_HOST_CONTRACT_DIR="$PWD/.host-contract" \
  -D SDKCONFIG=build-compat-hw1.0-p4legacy/sdkconfig \
  -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;config/board-v1.0.defaults;config/lcd-legacy.defaults;config/silicon-legacy.defaults" \
  build
```

### HW V1.0, P4 v3

```bash
idf.py -B build-compat-hw1.0-p4v3 \
  -D KITCHEN_HOST_CONTRACT_DIR="$PWD/.host-contract" \
  -D SDKCONFIG=build-compat-hw1.0-p4v3/sdkconfig \
  -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;config/board-v1.0.defaults;config/lcd-legacy.defaults;config/silicon-v3.defaults" \
  build
```

### HW V1.1, P4 legacy

```bash
idf.py -B build-compat-hw1.1-p4legacy \
  -D KITCHEN_HOST_CONTRACT_DIR="$PWD/.host-contract" \
  -D SDKCONFIG=build-compat-hw1.1-p4legacy/sdkconfig \
  -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;config/board-v1.1.defaults;config/lcd-legacy.defaults;config/silicon-legacy.defaults" \
  build
```

### HW V1.1, P4 v3

```bash
idf.py -B build-compat-hw1.1-p4v3 \
  -D KITCHEN_HOST_CONTRACT_DIR="$PWD/.host-contract" \
  -D SDKCONFIG=build-compat-hw1.1-p4v3/sdkconfig \
  -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;config/board-v1.1.defaults;config/lcd-legacy.defaults;config/silicon-v3.defaults" \
  build
```

### HW V1.2, P4 legacy

```bash
idf.py -B build-compat-hw1.2-p4legacy \
  -D KITCHEN_HOST_CONTRACT_DIR="$PWD/.host-contract" \
  -D SDKCONFIG=build-compat-hw1.2-p4legacy/sdkconfig \
  -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;config/board-v1.2.defaults;config/lcd-legacy.defaults;config/silicon-legacy.defaults" \
  build
```

### HW V1.2, P4 v3

```bash
idf.py -B build-compat-hw1.2-p4v3 \
  -D KITCHEN_HOST_CONTRACT_DIR="$PWD/.host-contract" \
  -D SDKCONFIG=build-compat-hw1.2-p4v3/sdkconfig \
  -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;config/board-v1.2.defaults;config/lcd-legacy.defaults;config/silicon-v3.defaults" \
  build
```

### HW V1.3, P4 legacy

```bash
idf.py -B build-compat-hw1.3-p4legacy \
  -D KITCHEN_HOST_CONTRACT_DIR="$PWD/.host-contract" \
  -D SDKCONFIG=build-compat-hw1.3-p4legacy/sdkconfig \
  -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;config/board-v1.3.defaults;config/lcd-v1.3.defaults;config/silicon-legacy.defaults" \
  build
```

### HW V1.3, P4 v3

```bash
idf.py -B build-compat-hw1.3-p4v3 \
  -D KITCHEN_HOST_CONTRACT_DIR="$PWD/.host-contract" \
  -D SDKCONFIG=build-compat-hw1.3-p4v3/sdkconfig \
  -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;config/board-v1.3.defaults;config/lcd-v1.3.defaults;config/silicon-v3.defaults" \
  build
```

## Результаты резолюции и артефакты

После первой пользовательской успешной резолюции/сборки проверить реальный новый
lock: target esp32p4 и пять точных версий. Не копировать lock хоста или прежнего
стенда. Commit/push требуют отдельного разрешения. Generated sdkconfig, managed_components
и build-каталоги не сохраняются в Git.

Разметка исключительно standalone: NVS 0x9000/0x6000, PHY 0xF000/0x1000,
factory app 0x10000/0x300000, partition table 0x8000, image flash 4 МиБ.
Factory заканчивается на 0x310000; фактический размер app проверяет пользовательская
сборка. Здесь нет OTA/security хоста. Никогда не переносить эту разметку в crow-ha-fw.
Не смешивать bootloader/partition/application разных build-каталогов и silicon-веток.

Для 0x14 GT911 нужен отдельный sdkconfig с выбором KITCHEN_TOUCH_ADDRESS_14.
Не менять GPIO перебором. Текущий standalone использует direct mode + avoid_tearing; после перехода
повторите прежнюю build-команду в своём каталоге. PPA/rotation не включены. Сборка реального host и применение integration bridge пока не выполняются;
будущее подключение — [INTEGRATION](INTEGRATION.md).
