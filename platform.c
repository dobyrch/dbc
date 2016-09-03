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

#include "platform.h"

/*
 * This file exists so that compilation fails immediately on unknown
 * platforms, rather than spewing out dozens of errors as a result of
 * undefined macros.
 *
 * The following declaration suppresses a warning about an empty
 * translation unit.
 */

extern void *dummy;
