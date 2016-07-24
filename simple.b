myarray[] 65, 66, 67, 68, 69;

main() {
	auto a, b, c;

	a = 1;
	b = 4;
	c = &b;

	return(a + *c + myarray[2]);
}
