#include "hal.h"
#include "lcd.h"

void game_main(void);

int main(void)
{
    hal_init();
    lcd_init();
    game_main();
    return 0;
}
