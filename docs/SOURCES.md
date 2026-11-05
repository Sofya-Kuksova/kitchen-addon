# Источники baseline

Снимок исходников: 30.10.2026. Опорные проекты использованы только для чтения. Аппаратные источники ниже относятся только к standalone. Целевые контракты и стек хоста приведены далее; прежний LVGL9-стек снят с цели.

## WarehouseFlow

Локальный каталог: `/Users/sonyakuks/esp/crowpanel_p4_WarehouseFlow/Crowpanel_P4_Warehouse-Flow_RTLS-Demo`.

HEAD: `0d7d797d4fb5dd8516f59dbc081fe35601379284`. Рабочее дерево: чистое.
Локальные отличия используемых файлов от HEAD: нет.

| Файл | SHA-256 |
| --- | --- |
| `AGENTS.md` | `513bda5039e367674293d1058613d3c56d2beb33e7d5e0de5d5d9d56315335ef` |
| `docs/HARDWARE_PROFILES.md` | `ae63e4fd976714ba0f69203d9ea1f9917444556ac9dfd5906bbd1b9ed4b3ce96` |
| `docs/DISPLAY_AND_LVGL.md` | `26369c6e68abead78c1716b75ced6209126133ce05315165d0fe2aebe2c7fac8` |
| `components/crowpanel_board/crowpanel_board.c` | `e1505eb44ef4825abf979ee1b93ee88055b8fbb30148cd090b194dd1b659a106` |
| `components/crowpanel_board/include/crowpanel_board.h` | `353a218dcc79d56b99f767298c453aeccd47754d68a25854feb27c27893ad3f5` |
| `components/crowpanel_board/Kconfig` | `8e004dbfe705d4cb9527f09e1011c91d32a5e42a36f3648f1ed94c76b32bbcfe` |
| `components/crowpanel_board/crowpanel_board_validate.c` | `21f734f2236f91c5013265c1294447ad68a8541f2829734ca3771ddfc7d39201` |
| `components/crowpanel_bsp/crowpanel_bsp.c` | `3e2a55fa78a44470fe7c6e23600cfc33f645931ab095f5f00058bcb4839341fd` |
| `components/crowpanel_bsp/idf_component.yml` | `8d3fb89a71ca7106c2a9e4d5bf1e9a17cd3021d76d07561a8ef4afceb15c6ae8` |
| `components/display_port/display_port.cpp` | `c7abf36236dc669bf0f4f0e6a5a82e29d48c34754d337a3f84c5c60f30a223e0` |
| `components/display_port/idf_component.yml` | `7552c698c9632cd555be7eddbf744b4d70dc715b163ae2737b8beb922b34f89a` |
| `sdkconfig.defaults` | `f2067cc05c361a1a3ac7091c7d53ebb89651ba1cbfa39c26ae3efcd61f8ac401` |
| `sdkconfig.v1.0` | `ecffb03a9453101c533571fedb2627fa8858612a2cdead46e9e9e61cf8414fa1` |
| `sdkconfig.v1.2` | `d86c2b9ec8bcde900d776cd3d59c22e758c91d772272b2cbd71e5652dc9a0a1d` |

## Leader

Локальный каталог: `/Users/sonyakuks/esp/crowpanel_p4_meeting room_lead/meetingroom_p4_-leader-`.

HEAD: `5abe1a8dacce8419c171eb2a1ae42d61c9dfeda9`. Рабочее дерево: чистое.
Локальные отличия используемых файлов от HEAD: нет.

| Файл | SHA-256 |
| --- | --- |
| `sdkconfig.board-v1.2.defaults` | `e77e2caaddc6e844d790d23386fc39ac554f6b814a585bc2af80742b7d44f455` |
| `main/include/crowpanel_board.h` | `7dee46b72b4a46cdba167cb2680cb333005172590f6d48cd117bfcb2f67f8365` |
| `components/espressif__esp32_p4_function_ev_board/esp32_p4_function_ev_board.c` | `076cee776fb80e4e0004fce0041625b27648a8876bce15886052dbe8990ba6ec` |
| `components/espressif__esp32_p4_function_ev_board/include/bsp/display.h` | `642e4ced4b5b364d18ec55249f72944b4c8516cb1acac76d8061b8d7ed65c6d6` |
| `components/espressif__esp32_p4_function_ev_board/include/bsp/esp32_p4_function_ev_board.h` | `b9dba4a11ff952ac42f20314ef8fec7db7e87affbe04667ce62e5d0c655c7341` |
| `sdkconfig.defaults` | `b83544a37601608604c5452818abbc450126d81d3f5e11f3270d1897b0ca2533` |

## Проект коллеги

Локальный каталог: `/Users/sonyakuks/esp/p4-test-inch7-9-10_1-idf`.

Git-метаданных нет; commit ID недоступен. Снимок определяется SHA-256 ниже. README указывает HW V1.2; применимость к новой HW V1.3 9/10.1 основана на сообщении коллеги, переданном в ТЗ.

| Файл | SHA-256 |
| --- | --- |
| `README.md` | `f0e317b2457df92b26206bd8bb688adb6d09b02e5e8e275d29e1b333ed6c9984` |
| `peripheral/bsp_display/bsp_display.c` | `aa50ca91f8ce6b672f1e8c447560f92fcc4507df4a5a3308b0de9ca6d3ee92a6` |
| `peripheral/bsp_display/include/bsp_display.h` | `1acb9795f4905d3873d9aa393fa662669273b885e052bca24a295c6063078462` |
| `peripheral/bsp_i2c/bsp_i2c.c` | `d2dc566b5fdabd60d7162351988bcc7c1f3bb626935fd4d3dcd8f8567f21496f` |
| `peripheral/bsp_i2c/include/bsp_i2c.h` | `39f49ef002c0ac3e08f095282dd5f5f081a87ecefd5eb504d35e7b366fd8b83e` |
| `sdkconfig.defaults.esp32p4` | `46d13dbcb3614062b7ed54fe5db949a591f4c8ac216bb2bb2c9c24cf652c58a6` |
| `sdkconfig` | `60b0aa9cde342b734d2d76fccbf07fb8b3fdb6a999f5faf6995ba532733c2d17` |
| `main/idf_component.yml` | `5ca8c22d4b1331806a709419ea6708d22bb39dc2bf358de474012fea59d51131` |

## Авторитетный host snapshot

`feature-panel-microphone@c3675d87075` — обозначение из имени распакованного каталога
`/Users/sonyakuks/Downloads/crow-ha-fw-feature-panel-microphone@c3675d87075`.
Git отсутствует; это не независимо подтверждённый полный commit. Старый checkout
crow-ha-fw не использовался как контрактный источник. `keys/` не читался и не копировался.
Хост не изменялся. Ниже SHA-256 конкретных прочитанных файлов.

| Контрактный файл хоста | SHA-256 |
| --- | --- |
| `dependencies.lock` | `d79f05d4ba05a4a560f66b41d327cbc2cedd0622b6a562b5a838e67494bf5a62` |
| `main/idf_component.yml` | `66d29ef6bdebd998b8af2b7198b2316077952d59a825eacd836acb5565cadcb0` |
| `sdkconfig.defaults` | `084c224d6886385ec8f6898dd8894203a84442805ea47b881226d5b9414b611f` |
| `partitions.csv` | `52eb1d981a588bc652e8d13a6622979d967bded042bdc7fc7fd467e08f5b5e8e` |
| `components/app_manager/include/app_base.h` | `b105002ee0833cb2ed10314ee12cc5ace698a410a5638d86b30b9c81e5c5b75f` |
| `components/app_manager/include/app_manager.h` | `d2a40a15600d072c1b1820edd39ecec89ae3def84daa7103c9d823258c6662c3` |
| `components/app_manager/include/jobs_queue.h` | `8530fffcaacba5a5a3f1ecb5f341d328d9a36c8226f20f8f6e123b76d71f00a4` |
| `components/app_manager/src/app_base.c` | `daecb14aec64c1b0415c39083c4324ffd74c27889edfc566673fd5a612128e91` |
| `components/app_manager/src/app_manager.c` | `54194d99c2ac9bcf3efd1faa2aab6f41d8946e5922680720b9d0554738e004b1` |
| `components/app_manager/src/jobs_queue.c` | `ecd11ed3a9ecafec5a1375d6873af0c10a6fee73d1f35d35ce1fd55c185cd665` |
| `components/home_assistant/src/ha_app.c` | `077b5861c8733d9c174582662c13708a44c9cdcc2c337ff7412c1f1e468e328c` |
| `components/home_assistant/src/ha_ui.c` | `e4662b8e15f54320022efeeb47b4eae49a33b416cf41ecd2de90792755ee92a6` |
| `components/home_assistant/src/ha_webview.cpp` | `53063ddbbf4deaecf3ad32b605f72d8b08e3326493744d4766b114db31398c33` |
| `main/main.cpp` | `f2a17d655f92cbe5ce3802485944b46fd4ec30948777568eeecb4bc2ed1251d7` |
| `components/espressif__esp32_p4_function_ev_board/include/bsp/esp32_p4_function_ev_board.h` | `78de991a7b63d997c47d85e4f40124aad8775109048d0d67ec1677ff6b35f528` |
| `components/espressif__esp32_p4_function_ev_board/esp32_p4_function_ev_board.c` | `f5856a75ea50617e36fc46a37f98414aaaeec27002d462eafea1ffdc2e878472` |
| `components/ha_audio/README.md` | `90511cf4b03afff73c4aa5648fb402130c3882c001a1e079d2e0a03fd6ab3cb9` |
| `components/ha_audio/include/ha_audio.h` | `574fd1c8e0449fb326df6c993aa8852adec1fcd897d9373ef618c1821d2dc33d` |
| `components/espressif__esp32_p4_function_ev_board/include/bsp_audio_input.h` | `c382e1a885cd9d9ecdb738fc71f37ddb7bc455bc628d404c4421cb0f96cb37f9` |
| `components/settings/include/settings_config.h` | `d55e4a6dd06e02c1ec940f61d54806921f0c1d215668ec8b4018f73b1c2c2176` |
| `components/settings/include/settings_events.h` | `ed38514f6e6318f8b0f60fea38148b1e9f3c81cf2bae931b4cd4803159062b31` |
| `components/sleep_mode/include/sleep_mode.h` | `501cba8c0d4e882bcf228ffb69dd8cac32f753b3b4b6d1dac146ff5bbc4f7f7b` |

### Версии, разрешённые lock хоста

| Компонент | Версия |
| --- | --- |
| idf | 6.0.2 |
| lvgl/lvgl | 8.4.0 |
| espressif/esp_lvgl_port | 2.8.0~1 |
| espressif/esp_lcd_ek79007 | 1.0.4 |
| espressif/esp_lcd_touch_gt911 | 1.2.0~3 |
| espressif/esp_lcd_touch | 1.2.1 (транзитивный контракт) |
| espressif/esp_codec_dev | 1.5.11 (только host service) |
| espressif/esp_audio_codec | 2.5.0 (только host service) |

Kitchen не копирует lock хоста и не добавляет audio codecs. Standalone закрепляет
SDK/LVGL/port/LCD/GT911 точно; embedded UI допускает IDF >=6.0.2,<6.1 и LVGL >=8.4.0,<8.5.
Новый `dependencies.lock` создаёт только реальная пользовательская резолюция.

### Проверенные API и ограничения

Прочитаны официальные архивы Registry [port 2.8.0~1](https://components.espressif.com/components/espressif/esp_lvgl_port/versions/2.8.0~1/readme),
[LVGL 8.4.0](https://components.espressif.com/components/lvgl/lvgl/versions/8.4.0/readme),
[EK79007 1.0.4](https://components.espressif.com/components/espressif/esp_lcd_ek79007/versions/1.0.4/readme),
[GT911 1.2.0~3](https://components.espressif.com/components/espressif/esp_lcd_touch_gt911/versions/1.2.0~3/readme).
SDK сверялся с локальными заголовками/Kconfig ESP-IDF 6.0.2. Это статическая сверка,
не успешная сборка. Vendor-код в репозиторий не помещался и не изменялся.

- Публичный kitchen UI использует `lv_obj_t`, `lv_disp_t`, `lv_obj_del`, `lv_timer_del`,
  `lv_anim_set_time`/`lv_anim_set_playback_time`. LVGL9 descriptors/assets отсутствуют.
- EK79007 1.0.4 constructor принимает mipi_config/lane_num; macro с устаревшим
  pixel_format не раскрывается. Standalone DPI задаётся явно через IDF6
  in/out_color_format. В драйвере нет disp_on_off hook.
- GT911 driver_data хранится в BSP постоянно; адрес задаётся reset/INT драйвера.
- LVGL8 port lock — recursive mutex, bool, timeout в мс, 0 = бесконечно.
  Bridge нормализует bool в esp_err_t. Kitchen create_content и UI callbacks
  не берут lock повторно; lifecycle вызывается вне GUI lock.
- LVGL8 port deinit только сбрасывает running; завершение worker не подтверждается
  публичным join API. remove_disp освобождает draw buffers даже при заимствованных
  LCD кадрах и не является безопасным teardown для zero-copy схемы.
  Первоначальный partial-вариант заменён по запросу пользователя на direct_mode +
  avoid_tearing с двумя borrowed LCD frames. Порт удерживается до reset; remove_disp
  не вызывается. Ошибка регистрации, способная освободить borrowed frames внутри
  vendor error path, завершает приложение через abort. Kitchen open/close освобождает
  только модуль, не порт и кадры.
- App manager удаляет borrowed containers через UI jobs; create_content приходит из
  GUI timer, lifecycle/on_view_change — вне этого timer. Удаление UI root извне
  отменяет motion и обнуляет указатели до освобождения instance.
- jobs_queue хоста имеет 32 элемента и synchronous waits; kitchen не создаёт свою
  копию и не вызывает jobs_queue_release. UI Back посылает публичный nonblocking intent.
- HA webview — существующие изображение/сессия/audio/microphone permissions, не API
  entities/service calls. Kitchen не подключает HA/audio/settings/sleep services.

### Архивы API, SHA-256

- `kitchen-port280.zip`: `dcd28de3ae303a757cdd3132d571b8f56114ee98a9e7344741eeef0e8f07ac5a`.
- `kitchen-lvgl840.zip`: `7eec856737ed6eda9c92ec97c6c2a0e72750ccfc0f383af1882feeed81a8a2fd`.
- `kitchen-ek104.zip`: `33c7f233d9241a5cf308135f0492b368e74c566bd02eca5b351749865cf3099a`.
- `kitchen-touch120.zip`: `e518bf14c4d6dc877e769cbfafb8d182ea9dfa3b47a685ed5312c7e12dbf5199`.

### Заимствованные контракты и лицензия

Host implementations и assets не копируются. ABI app_base.h подключается из реального
app_manager при embedded-сборке. Для приватного standalone fixture пользователь явно
предоставляет неизменённый app_base.h; helper копирует ровно этот файл в игнорируемый
локальный каталог с проверкой hash. В снимке не найден root LICENSE; это не даёт
основания назначать хосту новую лицензию. Копия заголовка не поставляется в репозитории;
при дальнейшей дистрибуции контрактов нужно сохранить условия правообладателя.

## Kitchen chime и аудио reference (обновлено 05.11.2026)

- User input: /Users/sonyakuks/Downloads/soundreality-bell-sound-effect-613873.mp3.
- Source MP3: stereo 48000 Hz, decoded duration 4.824 seconds; original unchanged.
- Source SHA-256: e21b0eed44a355efb01ebdb215ae4eec91add6a6efb0766867665f0a0861f33f.
- Runtime asset: main/assets/kitchen_timer_chime.wav; 183500 bytes.
- WAV SHA-256: 45a52177178de3d31a9d2130edeeeeb399bc1c1c82df6a11683828d335ab431e.
- PCM mono, 16-bit little-endian, 44100 Hz, 91728 frames, 2.08 seconds.
- Trailing signal stays below -60 dB after 1.978646 s. Trim at 2.08 s retains
  the bell decay; final 50 ms fade avoids an abrupt end. No leading silence
  >=100 ms was detected at -50 dB, so the start was retained.
- Mono is the average of left/right. Resampling briefly peaks at +0.035 dBFS
  in float, so gain -0.6 dB is applied after resampling to avoid PCM16 clipping.
  Final absolute sample peak 30706, no saturated samples, last sample zero.
- The old 1.55-s WAV was replaced at the same embedded path; CMake symbols,
  parser, worker, software volume and preview asset are unchanged. Asset growth
  is 46746 bytes; actual firmware size is pending the owner's manual build.

Reproduction from the repository root (media conversion only, not a firmware build):

```sh
ffmpeg -y -i /Users/sonyakuks/Downloads/soundreality-bell-sound-effect-613873.mp3 \
  -map_metadata -1 \
  -af 'pan=mono|c0=0.5*c0+0.5*c1,atrim=end=2.08,asetpts=PTS-STARTPTS,aresample=44100,volume=-0.6dB,afade=t=out:st=2.03:d=0.05' \
  -ar 44100 -ac 1 -c:a pcm_s16le -bitexact main/assets/kitchen_timer_chime.wav
```

- Reference (read only): Product Explainer components/demo_audio/src/demo_audio.cpp,
  demo_ui/src/demo_speaker_check.c, demo_ui/src/demo_bg_motion.c,
  espressif__esp32_p4_function_ev_board/esp32_p4_function_ev_board.c and my_codec.c.
  BCLK22, WS21, DOUT23, PA30 active-low. No source/vendor code copied or modified.
  The new standalone backend uses IDF I2S directly and software gain; no microphone,
  NVS, ES8311 control bus or esp_codec_dev dependency is introduced.

Volume preview: main/assets/volume_release.pcm, unchanged copy of Product Explainer
components/demo_audio/assets/sfx/volume_release.pcm. 34596 bytes, 17298 samples,
PCM16 LE mono 16000 Hz; gain 0.40 from kSfxVolumeReleaseGain. SHA-256: 70979d8d26fead2a5feff98b085f7faf6f06c1e4d9194aa6de860e26f5189618.

## Дополнительные UI assets

Официальные источники Settings и Material Symbols TTF, hash и точная команда
генерации записаны в components/kitchen_ui/assets/README.md. Новый
components/kitchen_ui/assets/material/settings_36px.svg — reference, Apache-2.0.
Новый kitchen_font_extensions.c содержит 56px kitchen glyphs, 36px Settings
и Inter Bold 700 status subsets. Inter.ttf не менялся, лицензия SIL OFL 1.1
сохранена. Ассеты визуально просмотрены в /tmp/kitchen-font-extensions.png;
это проверка растеризации, не firmware/device PASS.
