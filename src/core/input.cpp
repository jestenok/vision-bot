#include "core/input.hpp"

#include <windows.h>

#include <cctype>
#include <chrono>
#include <thread>
#include <unordered_map>

#include "core/random.hpp"

namespace vb {

int key_to_vk(const std::string& name) {
    if (name.size() == 1) {
        const unsigned char c = static_cast<unsigned char>(name[0]);
        if (std::isalpha(c))
            return std::toupper(c);
        if (std::isdigit(c))
            return c;
    }
    static const std::unordered_map<std::string, int> kNamed = {
        {"esc", VK_ESCAPE},   {"escape", VK_ESCAPE},
        {"space", VK_SPACE},  {"enter", VK_RETURN},  {"return", VK_RETURN},
        {"tab", VK_TAB},      {"shift", VK_SHIFT},   {"ctrl", VK_CONTROL},
        {"alt", VK_MENU},     {"backspace", VK_BACK},
        {"left", VK_LEFT},    {"right", VK_RIGHT},
        {"up", VK_UP},        {"down", VK_DOWN},
        {"f1", VK_F1},   {"f2", VK_F2},   {"f3", VK_F3},   {"f4", VK_F4},
        {"f5", VK_F5},   {"f6", VK_F6},   {"f7", VK_F7},   {"f8", VK_F8},
        {"f9", VK_F9},   {"f10", VK_F10}, {"f11", VK_F11}, {"f12", VK_F12},
    };
    const auto it = kNamed.find(name);
    return it != kNamed.end() ? it->second : 0;
}

namespace {

// Скан-коды (как pydirectinput): больше шансов, что игра увидит ввод.
void send_scancode(int vk, bool down) {
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    in.ki.wScan = static_cast<WORD>(MapVirtualKey(vk, MAPVK_VK_TO_VSC));
    in.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0u : KEYEVENTF_KEYUP);
    SendInput(1, &in, sizeof(INPUT));
}

}  // namespace

void send_key_down(int vk) {
    if (vk)
        send_scancode(vk, true);
}

void send_key_up(int vk) {
    if (vk)
        send_scancode(vk, false);
}

void send_mouse_click(const std::string& button) {
    DWORD down = MOUSEEVENTF_LEFTDOWN, up = MOUSEEVENTF_LEFTUP;
    if (button == "right") {
        down = MOUSEEVENTF_RIGHTDOWN;
        up = MOUSEEVENTF_RIGHTUP;
    } else if (button == "middle") {
        down = MOUSEEVENTF_MIDDLEDOWN;
        up = MOUSEEVENTF_MIDDLEUP;
    }
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = down;
    SendInput(1, &in, sizeof(INPUT));
    in.mi.dwFlags = up;
    SendInput(1, &in, sizeof(INPUT));
}

KeyHolder::KeyHolder(const std::string& left, const std::string& right)
    : left_vk_(key_to_vk(left)), right_vk_(key_to_vk(right)) {}

void KeyHolder::press_left() {
    if (right_down_) {
        send_key_up(right_vk_);
        right_down_ = false;
    }
    if (!left_down_) {
        send_key_down(left_vk_);
        left_down_ = true;
    }
}

void KeyHolder::press_right() {
    if (left_down_) {
        send_key_up(left_vk_);
        left_down_ = false;
    }
    if (!right_down_) {
        send_key_down(right_vk_);
        right_down_ = true;
    }
}

void KeyHolder::release_all() {
    if (left_down_) {
        send_key_up(left_vk_);
        left_down_ = false;
    }
    if (right_down_) {
        send_key_up(right_vk_);
        right_down_ = false;
    }
}

char KeyHolder::state() const {
    if (left_down_)
        return 'L';
    if (right_down_)
        return 'R';
    return '.';
}

KeyTap::KeyTap(std::string key, std::pair<double, double> hold_ms)
    : key_(std::move(key)), vk_(key_to_vk(key_)), hold_ms_(hold_ms) {}

void KeyTap::perform() {
    send_key_down(vk_);
    const int ms = static_cast<int>(uniform(hold_ms_.first, hold_ms_.second));
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    send_key_up(vk_);
}

std::string KeyTap::label() const {
    std::string up = key_;
    for (char& c : up)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return up + " pressed";
}

}  // namespace vb
