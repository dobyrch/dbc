main() {
	extrn n, b_putchar, v;
	auto i, c, col, a;

	i = col = 0;

foo:
	v[i++] = 1;
	if (i < n)
		goto foo;


	while(col<2*n) {
		a = n+1 ;
bar:
		c = i = 0;
		while(i<n) {
			c =+ v[i]*10;
			v[i++] = c%a;
			c =/ a--;
		}

		b_putchar(c+'0');
		if(!(++col%5))
			b_putchar(col%50?' ':10);
	}

	puts("hello world");
}

v[2000];
n 2000;
