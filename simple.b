myarray[] 1, 2, 3, 4, 5;

main() {
	auto b;
	b = 'A';

	if (b < 'Z') {
		b =+ 1;
		/* this doesn't work right since putchar expects 32 bit int */
		putchar(b);
	} else {
		return(1);
	}
}
