#ifndef VDR_OSR_BROWSER_KEYCODES_H
#define VDR_OSR_BROWSER_KEYCODES_H
#include <string>
#include <SDL_keyboard.h>
#include <unordered_map>

extern std::unordered_map<std::string, int> keyCodes;
extern std::unordered_map<SDL_Keycode, std::string> keyCodesSDL;

#endif //VDR_OSR_BROWSER_KEYCODES_H
