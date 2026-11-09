# Ручные проверки plain C

Эти файлы не включены в ESP-IDF component registration и не создают build hooks.
Агент не компилировал и не запускал их. Команды ниже выполняет только пользователь.

Из корня репозитория, системным C11-компилятором:

```sh
cc -std=c11 -Wall -Wextra -Werror \
  -I components/kitchen_core/include \
  components/kitchen_core/kitchen_core.c \
  components/kitchen_core/kitchen_settings.c tests/kitchen_tests.c \
  -o /tmp/kitchen_acceptance
/tmp/kitchen_acceptance
```

Ожидаемая строка только после фактического успешного запуска:
`Kitchen core/settings acceptance: PASS`.

Сценарии проверяют управляемые deadline, дробную паузу, границы, независимость
постоянной длительности, DONE после суток, пять событий завершения, mute,
сериализацию, повреждённые записи, неизвестную схему, поколения и Retry с fake
backend. Проверяется также возврат к ранее сохранённому значению при ещё незаконченной
записи другого значения и поздний успех старого запроса после отказа нового enqueue.

Новые сценарии kitchen_tests.c также описывают точное пересечение 5000 ms,
поздний deadline, Pause/Resume, повторное разрешение после продления, короткие
начальные сеансы, новый Repeat/Start/Extend, отмену pending, mute/unmute,
одновременные warning/DONE нескольких слотов, прогресс и отрицательный формат
до 100 часов. Маски проверяются до и после take_audio.

Микшер отдельно (только владелец, также **NOT RUN**):

```sh
cc -std=c11 -Wall -Wextra -Werror -I main \
  main/audio_mixer.c tests/audio_mixer_tests.c -o /tmp/kitchen_audio_acceptance
/tmp/kitchen_audio_acceptance
```

Ожидаемая строка после успешного запуска: `Kitchen audio mixer acceptance: PASS`.
Сценарии: независимые позиции двух голосов, суммирование положительных/отрицательных
samples, stereo/тишина хвоста, peak limiter обоих знаков/PCM16 guard, volume=0,
заполненные голоса и приоритет DONE, подавление/fade preview, сохранение gain
preview 0.40, resampling и точный конец clip 16000→44100 Hz.

Это не проверка UI, FreeRTOS worker, bounded очереди/portMUX/notifications, физической атомарности NVS, lifecycle или звука.
Для них нужна ручная матрица в docs/VALIDATION.md. Статус всех этих C-сценариев
до получения вывода владельца: NOT RUN.
