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

Это не проверка UI, FreeRTOS worker, физической атомарности NVS, lifecycle или звука.
Для них нужна ручная матрица в docs/VALIDATION.md. Статус всех этих C-сценариев
до получения вывода владельца: NOT RUN.
