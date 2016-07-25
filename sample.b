main() {
	/* TODO: Allow functions to be declared extrn */
	extrn n, v;
	auto i, c, col, a;

	i = col = 0;

	while(i<n)
		v[i++] = 1;

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
			putchar(col%50?' ':'*n');
	}
}

v[2000];
n 2000;
