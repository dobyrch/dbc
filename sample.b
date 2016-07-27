main() {
	extrn n, putchar, v, puts;
	auto i, c, col, a, baz;

	i = col = 0;

foo:
	v[i++] = 1;
	if (i < n)
		goto foo;

	goto bar;

	col = 999999;

bar:
	while(col<2*n) {
		a = n+1 ;
		c = i = 0;
		while(i<n) {
			c =+ v[i]*10;
			v[i++] = c%a;
			c =/ a--;
		}

		putchar(c+'0');
		if(!(++col%5))
			putchar(col%50?' ':10);
	}

	baz = puts + 1;
	(baz - 1)("hello world");
}

v[2000];
n 2000;
