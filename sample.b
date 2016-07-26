main() {
	extrn n, putchar, v;
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

		putchar(c+'0');
		if(!(++col%5))
			putchar(col%50?' ':'*n');
	}
}

v[2000];
n 2000;
