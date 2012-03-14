/*
 * Copyright 2012 Peter Bašista
 *
 * This file is part of the IPM Utility
 *
 * The IPM Utility is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
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

int compute_ipm_from_file (const char *input_filename,
		const char *input_file_encoding,
		int display_character_occurrences,
		long double *computed_ipm);
