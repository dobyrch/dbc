main() {
	auto b;
	b = 'A';

	if (b < 'Z') {
		b =+ 1;
		/* this doesn't work right since putchar expects 32 bit int */
		putchar(b);
	} else {
		putchar(48);
	}
}
