#include "screen_key_board.h"
#include <public_utils/timer/time.h>

int main()
{
    //ScreenKeyBoard::OpenOskKeyboard();
    ScreenKeyBoard::OpenScreenKeyboard();
    PublicUtils::Sleep(std::chrono::seconds(1));
    ScreenKeyBoard::OpenScreenKeyboard();
    PublicUtils::Sleep(std::chrono::seconds(1));
    ScreenKeyBoard::OpenScreenKeyboard();
    PublicUtils::Sleep(std::chrono::seconds(1));
    ScreenKeyBoard::OpenScreenKeyboard();
    while (true)
    {
        PublicUtils::Sleep(std::chrono::seconds(1));
    }
    return 0;
}
