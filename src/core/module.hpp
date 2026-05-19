#pragma once
#include <optional>
#include <string>

#include "core/geometry.hpp"

namespace vb {

// Кадр для окна отладки: регион на экране + флаг детекции.
struct DebugBlock {
    PixelRect rect;
    bool detected;
};

// Один независимый блок бота. Модуль сам захватывает свою область,
// детектит и действует. GameBot прокручивает список модулей кадр за кадром.
struct IModule {
    virtual ~IModule() = default;

    virtual const std::string& name() const = 0;

    // Один кадр. Возвращает строку для лога, если что-то сделал.
    virtual std::optional<std::string> tick(double now) = 0;

    // Сброс при паузе/выходе — отпустить клавиши, обнулить состояние.
    virtual void on_stop() = 0;

    // Регион модуля + текущий флаг детекции — для окна отладки.
    virtual DebugBlock debug_block() const = 0;
};

}  // namespace vb
