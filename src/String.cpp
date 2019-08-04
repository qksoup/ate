#include <ate/ate.hpp>

#include <ctype.h>

// strsep() implementaion for where it is missing, taken from bsd source.
#ifdef __sun 
/* 
 * Copyright (c) 1990, 1993
 * The Regents of the University of California.  All rights reserved.
 */
char* strsep(char **stringp, const char *delim)
{
    register char *s;
    register const char *spanp;
    register int c, sc;
    char *tok;
    
    if ((s = *stringp) == NULL)
        return (NULL);
    for (tok = s; ; ) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
}
#endif  // end of __sun: strsetp() implemenation

namespace ate {
    char* trimLeft(char* str, const char* delim)
    {
        if (str == NULL) 
            return NULL;
        while(*str != '\0' && strchr(delim, *str)) 
            ++str;
        return str;
    }
    
    char* trimRight(char* str, const char* delim)
    {
        if (str == NULL)
            return NULL;
        size_t len = strlen(str);
        char* tail = str + len - 1;
        while (tail >= str && strchr(delim, *tail)) {
            --tail; 
        }
        *(tail + 1) = '\0';
        return str;
    }
    
    char* trim(char* str, const char* delim)
    {
        if (str == NULL)
            return NULL;
        trimRight(str, delim);
        return trimLeft(str, delim);
    }

    void toLower(char* str)
    {
        if (str == NULL) 
            return;
        while(*str != '\0') {
            *str = tolower(*str);
            ++str;
        }
    }
    
    void toUpper(char* str)
    {
        if (str == NULL)
            return;
        while(*str != '\0') {
            *str = toupper(*str);
            ++str;
        }
    }

    void tokenize(std::vector<char*>& tokens, char* str, const char* delim)
    {
        char* saveptr;
        for (char* token = strtok_r(str, delim, &saveptr); token != NULL;
             token = strtok_r(NULL, delim, &saveptr)) 
            tokens.push_back(token);
    }

    void split(std::vector<char*>& tokens, char* str, const char* delim)
    {
        char* token;
        while ((token = strsep(&str, delim)) != NULL)
            tokens.push_back(token);
    }
}
