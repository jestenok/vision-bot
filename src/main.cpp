// vision-bot (C++) — точка входа.
//
//   vision_bot.exe [профиль]
//
// Без аргумента берётся профиль kDefaultProfile.
#include <windows.h>

#include <cstdio>
#include <exception>
#include <string>
#include <utility>

#include "core/runner.hpp"
#include "profiles/profile.hpp"

namespace {
constexpr const char* kDefaultProfile = "nte-fishing";
}

int main(int argc, char** argv) {
    SetProcessDPIAware();  // координаты регионов = реальные пиксели экрана

    const std::string name = (argc > 1) ? argv[1] : kDefaultProfile;
    auto profile = vb::find_profile(name);
    if (!profile) {
        std::printf("profile not found: %s\n", name.c_str());
        std::printf("available: nte-fishing, reaction-test, cigame\n");
        return 1;
    }

    try {
        vb::GameBot bot(std::move(*profile));
        bot.run();
    } catch (const std::exception& e) {
        std::printf("fatal: %s\n", e.what());
        return 1;
    }
    return 0;
}
