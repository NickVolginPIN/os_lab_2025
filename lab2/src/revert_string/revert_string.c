#include "revert_string.h"
#include <string.h>

void RevertString(char *str)
{
	int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char buffer = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = buffer;
    }
}

