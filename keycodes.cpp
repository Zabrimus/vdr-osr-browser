#include "keycodes.h"

std::unordered_map<std::string, int> keyCodes = {
        // official key codes for hbbtv apps
        {"VK_BACK",      0x01CD},    // this is not backspace
        {"VK_LEFT",      0x0025},    // left
        {"VK_UP",        0x0026},    // up
        {"VK_RIGHT",     0x0027},    // right
        {"VK_DOWN",      0x0028},    // down
        {"VK_ENTER",     0x000D},    // enter
        {"VK_RED",       0x0193},    // red
        {"VK_GREEN",     0x0194},    // green
        {"VK_YELLOW",    0x0195},    // yellow
        {"VK_BLUE",      0x0196},    // blue
        {"VK_PLAY",      0x0050},    // play
        {"VK_PAUSE",     0x0051},    // pause
        {"VK_STOP",      0x0053},    // stop
        {"VK_FAST_FWD",  0x0046},    // fast forward
        {"VK_REWIND",    0x0052},    // rewind
        {"VK_PAGE_UP",   0x0021},    // page up, vk_Prior
        {"VK_PAGE_DOWN", 0x0022},    // page down, vk_Next
        {"VK_0",         0x0030},    // 0
        {"VK_1",         0x0031},    // 1
        {"VK_2",         0x0032},    // 2
        {"VK_3",         0x0033},    // 3
        {"VK_4",         0x0034},    // 4
        {"VK_5",         0x0035},    // 5
        {"VK_6",         0x0036},    // 6
        {"VK_7",         0x0037},    // 7
        {"VK_8",         0x0038},    // 8
        {"VK_9",         0x0039},    // 9

        // list of other codes. not yes used. But who knows...
        {"VK_TAB",       0x0009},    // Tabulator
        {"VK_CLEAR",     0x000C},
        {"VK_SHIFT",     0x0010},    // Shift-Taste
        {"VK_CONTROL",   0x0011},    // STRG
        {"VK_MENU",      0x0012},    // Win95 Tastatur Menu
        {"VK_PAUSE",     0x0013},    // Pause-Taste
        {"VK_CAPITAL",   0x0014},    // CAPS LOCK
        {"VK_ESCAPE",    0x001B},    // ESC-Taste
        {"VK_SPACE",     0x0020},    // Leertaste
        {"VK_END",       0x0023},    // Ende-Taste
        {"VK_HOME",      0x0024},    // POS1-Taste
        {"VK_SELECT",    0x0029},
        {"VK_PRINT",     0x002A},    // Druck
        {"VK_EXECUTE",   0x002B},
        {"VK_SNAPSHOT",  0x002C},    // Druck ?
        {"VK_INSERT",    0x002D},    // Einfügen
        {"VK_DELETE",    0x002E},    // Entf
        {"VK_A",         0x0041},    // A
        {"VK_B",         0x0042},    // B
        {"VK_C",         0x0043},    // C
        {"VK_D",         0x0044},    // D
        {"VK_E",         0x0045},    // E
        {"VK_F",         0x0046},    // F
        {"VK_G",         0x0047},    // G
        {"VK_H",         0x0048},    // H
        {"VK_I",         0x0049},    // I
        {"VK_J",         0x004A},    // J
        {"VK_K",         0x004B},    // K
        {"VK_L",         0x004C},    // L
        {"VK_M",         0x004D},    // M
        {"VK_N",         0x004E},    // N
        {"VK_O",         0x004F},    // O
        {"VK_P",         0x0050},    // P
        {"VK_Q",         0x0051},    // Q
        {"VK_R",         0x0052},    // R
        {"VK_S",         0x0053},    // S
        {"VK_T",         0x0054},    // T
        {"VK_U",         0x0055},    // U
        {"VK_V",         0x0056},    // V
        {"VK_W",         0x0057},    // W
        {"VK_X",         0x0058},    // X
        {"VK_Y",         0x0059},    // Y
        {"VK_Z",         0x005A},    // Z
        {"VK_NUMPAD0",   0x0060},    // Numerische Tastatur
        {"VK_NUMPAD1",   0x0061},
        {"VK_NUMPAD2",   0x0062},
        {"VK_NUMPAD3",   0x0063},
        {"VK_NUMPAD4",   0x0064},
        {"VK_NUMPAD5",   0x0065},
        {"VK_NUMPAD6",   0x0066},
        {"VK_NUMPAD7",   0x0067},
        {"VK_NUMPAD8",   0x0068},
        {"VK_NUMPAD9",   0x0069},
        {"VK_MULTIPLY",  0x006A},    // Sternchen
        {"VK_ADD",       0x006B},    // Addition
        {"VK_SEPARATOR", 0x006C},
        {"VK_SUBTRACT",  0x006D},     // Subtraktion
        {"VK_DECIMAL",   0x006E},     // Komma
        {"VK_DIVIDE",    0x006F},     // Division
        {"VK_F1",        0x0070},
        {"VK_F2",        0x0071},
        {"VK_F3",        0x0072},
        {"VK_F4",        0x0073},
        {"VK_F5",        0x0074},
        {"VK_F6",        0x0075},
        {"VK_F7",        0x0076},
        {"VK_F8",        0x0077},
        {"VK_F9",        0x0078},
        {"VK_F10",       0x0079},
        {"VK_F11",       0x007A},
        {"VK_F12",       0x007B},
        {"VK_F13",       0x007C},
        {"VK_F14",       0x007D},
        {"VK_F15",       0x007E},
        {"VK_F16",       0x007F},
        {"VK_F17",       0x0080},
        {"VK_F18",       0x0081},
        {"VK_F19",       0x0082},
        {"VK_F20",       0x0083},
        {"VK_F21",       0x0084},
        {"VK_F22",       0x0085},
        {"VK_F23",       0x0086},
        {"VK_F24",       0x0087},
        {"VK_NUMLOCK",   0x0090},    // NumLock
        {"VK_SCROLL",    0x0091}     // Rollen
};



