#include <stdio.h>
#include <ctype.h>

int main()
{
    char *str = " DD99clea1234567890123 USD";
    char buff[20] = {
        0,
    };
    int i = 0;

    while (*str)
    {
        if (isdigit(*str))
        {
            buff[i++] = *str;
        }
        str++;
    }
    printf("%s\n", buff);
}