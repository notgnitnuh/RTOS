
#include <curses.h>
#include <stdlib.h>
// #include <time.h>
#include <firework_task.h>
#include <task.h>
#include <UART_16550.h>
#include <stdio.h>
#include <FreeRTOS.h>



/* Dimensions the buffer that the task being created will use as its
stack. NOTE: This is the number of words the stack will hold, not the
number of bytes. For example, if each stack item is 32-bits, and this
is set to 100, then 400 bytes (100 * 32-bits) will be allocated. */
#define FIREWORK_STACK_SIZE 256

/* Structure that will hold the TCB of the task being created. */
StaticTask_t firework_TCB;

/* Buffer that the task being created will use as its stack. Note this
is an array of StackType_t variables. The size of StackType_t is
dependent on the RTOS port. */
StackType_t firework_stack[ FIREWORK_STACK_SIZE ];

#define DELAYSIZE 250
#define COLS 50
#define ROWS 50

void myrefresh(void);
void get_color(void);
void explode(int, int);

int weite;
int level;
int skill_level;

short color_table[] =
{
    COLOR_RED, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
    COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE
};

#define INTENTIONALLY_UNUSED_PARAMETER( param) (void)(param)

void firework_task(void *pvParameters)
{
    int start, end, row, diff, flag, direction;
    short i;

    initscr();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    noecho();

    if (has_colors())
        start_color();

    for (i = 0; i < 8; i++)
        init_pair(i, color_table[i], COLOR_BLACK);

    srand(05242025);
    flag = 0;

    while (getch() == ERR)      /* loop until a key is hit */
    {
        do {
            start = rand() % (COLS - 3);
            end = rand() % (COLS - 3);
            start = (start < 2) ? 2 : start;
            end = (end < 2) ? 2 : end;
            direction = (start > end) ? -1 : 1;
            diff = abs(start - end);

        } while (diff < 2 || diff >= LINES - 2);

        attrset(A_NORMAL);

        for (row = 0; row < diff; row++)
        {
            mvaddstr(LINES - row, row * direction + start,
                (direction < 0) ? "\\" : "/");

            if (flag++)
            {
                myrefresh();
                erase();
                flag = 0;
            }
        }

        if (flag++)
        {
            myrefresh();
            flag = 0;
        }

        explode(LINES - row, diff * direction + start);
        erase();
        myrefresh();
    }

    endwin();
}

void explode(int row, int col)
{
    erase();
    mvaddstr(row, col, "-");
    myrefresh();

    --col;

    get_color();
    mvaddstr(row - 1, col, " - ");
    mvaddstr(row,     col, "-+-");
    mvaddstr(row + 1, col, " - ");
    myrefresh();

    --col;

    get_color();
    mvaddstr(row - 2, col, " --- ");
    mvaddstr(row - 1, col, "-+++-");
    mvaddstr(row,     col, "-+#+-");
    mvaddstr(row + 1, col, "-+++-");
    mvaddstr(row + 2, col, " --- ");
    myrefresh();

    get_color();
    mvaddstr(row - 2, col, " +++ ");
    mvaddstr(row - 1, col, "++#++");
    mvaddstr(row,     col, "+# #+");
    mvaddstr(row + 1, col, "++#++");
    mvaddstr(row + 2, col, " +++ ");
    myrefresh();

    get_color();
    mvaddstr(row - 2, col, "  #  ");
    mvaddstr(row - 1, col, "## ##");
    mvaddstr(row,     col, "#   #");
    mvaddstr(row + 1, col, "## ##");
    mvaddstr(row + 2, col, "  #  ");
    myrefresh();

    get_color();
    mvaddstr(row - 2, col, " # # ");
    mvaddstr(row - 1, col, "#   #");
    mvaddstr(row,     col, "     ");
    mvaddstr(row + 1, col, "#   #");
    mvaddstr(row + 2, col, " # # ");
    myrefresh();
}

void myrefresh(void)
{
    napms(DELAYSIZE);
    move(LINES - 1, COLS - 1);
    refresh();
}

void get_color(void)
{
    chtype bold = (rand() % 2) ? A_BOLD : A_NORMAL;
    attrset(COLOR_PAIR(rand() % 8) | bold);
}
