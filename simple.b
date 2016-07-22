myarray[] 65, 66, 67, 68, 69;

main() {
	auto b;
	b = getchar(1);

	if (b < 91) {
		b =+ 1;
		/* this doesn't work right since putchar expects 32 bit int */
		putchar(myarray[4]);
	} else {
		return(1);
	}
}
