#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

#include "ui.h"

int main(int argc, char *argv[])
{
    ui_init();
    draw_logo();
    if (argc < 2)
        ui_draw_progress(100);
    else
        ui_draw_progress(atoi(argv[1]));
}