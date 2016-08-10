#include <stdint.h>
#include <stdio.h>

int64_t b_putchar(int64_t c)
{
	return putchar(c);
}

int64_t b_puts(int64_t str)
{
	char *s = (char *)str;

	while (*s != EOF) {
		if (*s != '\0')
			putchar(*s);

		s++;
	}

	return 0;
}
