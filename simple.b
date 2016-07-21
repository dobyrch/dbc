main() {
	auto b;
	b = 'A';

	if (b < 'Z' + 1) {
		/* this doesn't work right since putchar expects 32 bit int */
		b =+ 1;
		putchar(b);
	} else
		putchar(48);
}
