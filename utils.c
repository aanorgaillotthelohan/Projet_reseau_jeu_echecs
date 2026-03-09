#include "chess.h"
#include <termios.h>
#include <unistd.h>

/*
 * getch_linux() – remplace getch() de <conio.h> (Windows).
 * Lit un seul caractère sans attendre Entrée, sans écho.
 */
char getch_linux(void)
{
    struct termios oldt, newt;
    char ch;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);   /* mode raw, sans écho */
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    read(STDIN_FILENO, &ch, 1);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); /* restaurer le terminal */
    return ch;
}

/*
 * delay() – pause en secondes (compatible Linux/Windows).
 */
void delay(int number_of_seconds)
{
    int milli_seconds = 1000 * number_of_seconds;
    clock_t start_time = clock();
    while (clock() < start_time + milli_seconds);
}
