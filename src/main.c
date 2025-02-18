#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

#include "ui.h"
int main()
{
    ui_init();
    draw_logo();
    for (int i = 0; i <= 100; i++)
    {
        /* code */
        ui_draw_progress(i);
        sleep(1);
    }
}
