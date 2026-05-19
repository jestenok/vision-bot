# vision-bot (C++)

Порт Python-версии (`../python`) на C++20. Та же архитектура: движок (`core/`),
механики (`mechanics/`), профили (`profiles/`), бот — список модулей.

## Зависимости

- Компилятор с C++20 (MSVC 2019+).
- OpenCV (C++), проще всего через [vcpkg](https://vcpkg.io):
  ```
  vcpkg install opencv4:x64-windows
  ```
- Windows SDK (Desktop Duplication API, SendInput) — идёт с Visual Studio.

## Сборка

```
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

Бинарь: `build/Release/vision_bot.exe`.

## Запуск

```
vision_bot.exe [профиль]
```

Без аргумента берётся профиль по умолчанию (`profiles.cpp`, `kDefaultProfile`).
Профили вкомпилированы (в отличие от Python, где они грузятся из файлов):
`nte-fishing`, `reaction-test`, `cigame`.

- F8 — старт/пауза, F9 — выход (или хоткеи профиля).

## Отличия от Python-версии

- Захват экрана — Desktop Duplication API: весь рабочий стол снимается один
  раз за кадр (`Desktop`), модули вырезают свой регион из общего кадра.
- `std::optional` вместо `| None`, `enum class Dir` для направления,
  виртуальные интерфейсы вместо утиной типизации, RAII для COM/GDI-ресурсов.
- Профили — вкомпилированные функции, а не подгружаемые .py-файлы.
