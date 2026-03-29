#include "stdio.h"
#include "types.h"
#include "vga.h"

#define PRINTF_STATE_NORMAL         0
#define PRINTF_STATE_LENGTH         1
#define PRINTF_STATE_LENGTH_SHORT   2
#define PRINTF_STATE_LENGTH_LONG    3
#define PRINTF_STATE_SPEC           4

#define PRINTF_LENGTH_DEFAULT       0
#define PRINTF_LENGTH_SHORT_SHORT   1
#define PRINTF_LENGTH_SHORT         2
#define PRINTF_LENGTH_LONG          3
#define PRINTF_LENGTH_LONG_LONG     4

static const char g_HexChars[] = "0123456789abcdef";

static uint32_t* printf_number(uint32_t* argp, int length, int sign, int radix)
{
    char buffer[32];
    uint32_t number = 0;
    int number_sign = 1;
    int pos = 0;

    switch (length)
    {
        case PRINTF_LENGTH_SHORT_SHORT:
        case PRINTF_LENGTH_SHORT:
        case PRINTF_LENGTH_DEFAULT:
        case PRINTF_LENGTH_LONG:
        case PRINTF_LENGTH_LONG_LONG:
            if (sign)
            {
                int n = *(int*)argp;
                if (n < 0)
                {
                    n = -n;
                    number_sign = -1;
                }
                number = (unsigned long long)n;
            }
            else
            {
                number = *(unsigned int*)argp;
            }
            argp += (length == PRINTF_LENGTH_LONG_LONG) ? 2 : 1;
            break;
    }

    do
    {
        unsigned int rem = (unsigned int)(number % (uint32_t)radix);
        number /= (uint32_t)radix;
        buffer[pos++] = g_HexChars[rem];
    } while (number > 0);

    if (sign && number_sign < 0)
        buffer[pos++] = '-';

    while (--pos >= 0)
        putc(buffer[pos]);

    return argp;
}

void printf(const char* fmt, ...)
{
    uint32_t* argp = (uint32_t*)&fmt;
    int state = PRINTF_STATE_NORMAL;
    int length = PRINTF_LENGTH_DEFAULT;
    int radix = 10;
    int sign = 0;

    argp++;

    while (*fmt)
    {
        switch (state)
        {
            case PRINTF_STATE_NORMAL:
                switch (*fmt)
                {
                    case '%':
                        state = PRINTF_STATE_LENGTH;
                        break;
                    default:
                        putc(*fmt);
                        break;
                }
                break;

            case PRINTF_STATE_LENGTH:
                switch (*fmt)
                {
                    case 'h':
                        length = PRINTF_LENGTH_SHORT;
                        state = PRINTF_STATE_LENGTH_SHORT;
                        break;
                    case 'l':
                        length = PRINTF_LENGTH_LONG;
                        state = PRINTF_STATE_LENGTH_LONG;
                        break;
                    default:
                        goto PRINTF_STATE_SPEC_;
                }
                break;

            case PRINTF_STATE_LENGTH_SHORT:
                if (*fmt == 'h')
                {
                    length = PRINTF_LENGTH_SHORT_SHORT;
                    state = PRINTF_STATE_SPEC;
                }
                else
                {
                    goto PRINTF_STATE_SPEC_;
                }
                break;

            case PRINTF_STATE_LENGTH_LONG:
                if (*fmt == 'l')
                {
                    length = PRINTF_LENGTH_LONG_LONG;
                    state = PRINTF_STATE_SPEC;
                }
                else
                {
                    goto PRINTF_STATE_SPEC_;
                }
                break;

            case PRINTF_STATE_SPEC:
            PRINTF_STATE_SPEC_:
                switch (*fmt)
                {
                    case 'c':
                        putc((char)*argp);
                        argp++;
                        break;

                    case 's':
                        puts(*(const char**)argp);
                        argp++;
                        break;

                    case '%':
                        putc('%');
                        break;

                    case 'd':
                    case 'i':
                        radix = 10;
                        sign = 1;
                        argp = printf_number(argp, length, sign, radix);
                        break;

                    case 'u':
                        radix = 10;
                        sign = 0;
                        argp = printf_number(argp, length, sign, radix);
                        break;

                    case 'X':
                    case 'x':
                    case 'p':
                        radix = 16;
                        sign = 0;
                        argp = printf_number(argp, length, sign, radix);
                        break;

                    case 'o':
                        radix = 8;
                        sign = 0;
                        argp = printf_number(argp, length, sign, radix);
                        break;

                    default:
                        break;
                }

                state = PRINTF_STATE_NORMAL;
                length = PRINTF_LENGTH_DEFAULT;
                radix = 10;
                sign = 0;
                break;
        }

        fmt++;
    }
}
