#include <windows.h>
#include <stdio.h>
#include <conio.h>

int main(int argc, wchar_t* argv[])
{
    printf(".\n");
    MessageBeep(123);
    getch();

    printf(".\n");
    MessageBeep(123);
    getch();

    printf(".\n");
    MessageBeep(123);
    getch();
    return 0;
}

