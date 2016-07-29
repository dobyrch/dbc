main() {
	extrn n, putchar, v, printf;
	auto i, c, col, a, baz;

	i = col = 0;

	switch (col) {
	case 0:
		printf("col is 0%c", 10);
		goto foo;
	case 1:
		printf("col is 1%c", 10);
		goto foo;
	}

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

	baz = printf + 1;
	(baz - 1)("hello %s, the year is %lu", "world", 2000 + 16);
}

v[2000];
n 2000;
