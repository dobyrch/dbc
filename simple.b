myarray[] 65, 66, 67, 68, 69;

main() {
	auto a, b, c;

	a = 1;
	b = 4;
	goto foo;
	c = &b;
	return;
	b = 5;


foo:
	b = 7;
}
