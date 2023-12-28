#include "os.h"
#include "user_api.h"

int if_equal_to_myprintfWithoutArgument(const char *s)
{
    const char *target = "myprintfWithoutArgument";
    for (int i = 0; i < 23; i++)
    {
        if (*(s + i) != target[i])
        {
            return 0;
        }
    }
    return 1;
}

