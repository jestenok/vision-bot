#pragma once
#include <string>
#include <utility>

namespace vb {

// Имя клавиши ("a", "esc", "f8", "space") → виртуальный код Windows.
int key_to_vk(const std::string& name);

void send_key_down(int vk);
void send_key_up(int vk);
void send_mouse_click(const std::string& button);  // "left"/"right"/"middle"

// Удержание двух клавиш как стейта. Не повторяет keyDown.
class KeyHolder {
public:
    KeyHolder(const std::string& left, const std::string& right);
    void press_left();
    void press_right();
    void release_all();
    char state() const;  // 'L' / 'R' / '.'

private:
    int left_vk_, right_vk_;
    bool left_down_ = false;
    bool right_down_ = false;
};

// --- Действие сторожа -------------------------------------------------------

struct IAction {
    virtual ~IAction() = default;
    virtual void perform() = 0;
    virtual std::string label() const = 0;
};

// Нажатие клавиши с человекоподобным удержанием.
class KeyTap : public IAction {
public:
    explicit KeyTap(std::string key,
                    std::pair<double, double> hold_ms = {30.0, 90.0});
    void perform() override;
    std::string label() const override;

private:
    std::string key_;
    int vk_;
    std::pair<double, double> hold_ms_;
};

// Клик мышью в текущей позиции курсора.
class MouseClick : public IAction {
public:
    explicit MouseClick(std::string button = "left")
        : button_(std::move(button)) {}
    void perform() override { send_mouse_click(button_); }
    std::string label() const override { return "click (" + button_ + ")"; }

private:
    std::string button_;
};

}  // namespace vb
