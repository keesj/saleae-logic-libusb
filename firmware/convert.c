#include <stdio.h>

/* tool to convert the data produced by usbmod into an array of bytes */
/* We skip new lines and append 0x to everty two characters read from stdin */
int main(int argc, char **argv)
{
    int count = 0;
    int prev = 0;
    int current = 0;
    while (1) {
	current = getchar();
	if (current == EOF) {
	    return 0;
	}
	if (current == '\n') {
	    continue;
	}
	if (prev != 0) {
	    printf("0x%c%c, ", prev, current);
	    prev = 0;
	    count++;
	} else {
	    prev = current;
	}
	if (count % 12 == 0) {
	    printf("\n");
	}
    }
}
