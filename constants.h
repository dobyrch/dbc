/*
 * Copyright 2016 Douglas Christman
 *
 * This file is part of dbc.
 *
 * dbc is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define WORDSIZE (1 << WORDPOW)

#define STDOUT_BUFSIZE 1024
#define MAX_STRSIZE 4096
#define MAX_ARGS 256

#define EOT 4

#define SEC_PER_MIN 60
#define SEC_PER_HOUR (60 * SEC_PER_MIN)
#define SEC_PER_DAY (24 * SEC_PER_HOUR)
/* Average, counting leap years */
#define SEC_PER_MONTH 2629746
#define SEC_PER_YEAR (12 * SEC_PER_MONTH)
