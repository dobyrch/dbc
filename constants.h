#define WORDSIZE (1 << WORDPOW)

#define MAX_STRSIZE 4096
#define MAX_ARGS 256

#define EOT 4

#define SEC_PER_MIN 60
#define SEC_PER_HOUR (60 * SEC_PER_MIN)
#define SEC_PER_DAY (24 * SEC_PER_HOUR)
/* Average, counting leap years */
#define SEC_PER_MONTH 2629746
#define SEC_PER_YEAR (12 * SEC_PER_MONTH)
