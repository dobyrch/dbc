/* The following function will print a non-negative number, n, to
   the base b, where 2<=b<=10.  This routine uses the fact that
   in the ANSCII character set, the digits O to 9 have sequential
   code values.  */

printn(n,b) {
   extrn putchar;
   auto a;

   if(a=n/b) /* assignment, not test for equality */
      printn(a, b); /* recursive */
   putchar(n%b + '0');
}


/* The following function is a general formatting, printing, and
   conversion subroutine.  The first argument is a format string.
   Character sequences of the form '%x' are interpreted and cause
   conversion of type 'x' of the next argument, other character
   sequences are printed verbatim.  Thus

      printf("delta is %d*n", delta);

   will convert the variable delta to decimal (%d) and print the
   string with the converted form of delta in place of %d.  The
   conversions %d-decimal, %o-octal, %s-string and %c-character
   are allowed.

   This program calls upon the function 'printn'. (see section
   9.1) */

printf(fmt, x1,x2,x3,x4,x5,x6,x7,x8,x9) {
   extrn printn, char, putchar;
   auto adx, x, c, i, j;

   i = 0; /* fmt index */
   adx = &x1; /* argument pointer */
loop:
   while((c=char(fmt,i++)) != '%') {
      if(c == '*e')
         return;
      putchar(c);
   }
   x = *adx++;
   switch c = char(fmt,i++) {

   case 'd': /* decimal */
   case 'o': /* octal */
      if(x < 0) {
         x = -x;
         putchar('-');
      }
      printn(x, c=='o'?8:10);
      goto loop;

   case 'c': /* char */
      putchar(x);
      goto loop;

   case 's': /* string */
      j = 0;
      while((c=char(x,j++)) != '*e')
         putchar(c);
      goto loop;
   }
   putchar('%') ;
   i--;
   adx--;
   goto loop;
}
