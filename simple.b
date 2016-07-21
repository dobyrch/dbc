main() {
	auto b;
	b = 'A';
	b = &b;

	if (0 < b)
		/* this doesn't work right since putchar expects 32 bit int */
		putchar(*b);
	else
		putchar(48);
}
