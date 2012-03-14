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

#include "ipm.hpp"

#include <cstdlib>
#include <iostream>

/* helping function */

/**
 * A function, which prints the short usage text for this program.
 *
 * @param
 * argv0	the argv[0], or the command used to run this program
 *
 * @return	This function always returns zero (0).
 */
int print_short_usage (const char *argv0) {
	std::cout << "Usage:\t" << argv0 << "\t[options] filename\n\n"
		"This will read the file 'filename' and computes\n"
		"the inverse probability of matching (IPM)\n"
		"of any two arbitrarily chosen characters "
		"in the input file.\n\n";
	return (0);
}

/**
 * A function, which prints the help text for this program.
 *
 * @param
 * argv0	the argv[0], or the command used to run this program
 *
 * @return	This function always returns zero (0).
 */
int print_help (const char *argv0) {
	print_short_usage(argv0);
	std::cout << "Additional options:\n"
		"-e <file_encoding>\tSpecifies the character encoding\n"
		"\t\t\tof the input file 'filename'. The default value\n"
		"\t\t\tis UTF-8. The valid encodings are all those\n"
		"\t\t\tsupported by the iconv.\n"
		"-v\t\t\tDisplays the number of character occurrences.\n";
	return (0);
}

/**
 * A function, which prints the full usage text for this program.
 *
 * @param
 * argv0	the argv[0], or the command used to run this program
 *
 * @return	This function always returns zero (0).
 */
int print_usage (const char *argv0) {
	print_short_usage(argv0);
	std::cout << "For the list of additional options, run: " <<
		argv0 << " -h\n";
	return (0);
}

/* the main function */

/**
 * The main function.
 * It executes a function to compute the inverse probability of matching (IPM)
 * of the specified input file.
 *
 * @param
 * argc		the argument count, or the number of program arguments
 * 		(including the argv[0])
 * @param
 * argv		the argument vector itself, or an array of argument strings
 *
 * @return	If the benchmark is successful or was not requested,
 * 		this function returns EXIT_SUCCESS.
 * 		Otherwise, this function returns EXIT_FAILURE.
 */
int main (int argc, char **argv) {
	char c = '\0';
	int getopt_retval = 0;
	int display_character_occurrences = 0;
	long double computed_ipm = 0;
	/* By default, we suppose that the input file encoding is UTF-8 */
	const char *input_file_encoding = "UTF-8";
	char *input_filename = NULL;
	std::cout << "Inverse probability of matching (IPM) computation\n\n";
	if (argc == 1) {
		print_usage(argv[0]);
		return (EXIT_SUCCESS);
	}
	/* parsing the command line options */
	while ((getopt_retval = getopt(argc, argv, "e:vh")) !=
			(-1)) {
		c = (char)(getopt_retval);
		switch (c) {
			case 'e':
				input_file_encoding = optarg;
				break;
			case 'v':
				display_character_occurrences = 1;
				break;
			case 'h':
				print_help(argv[0]);
				return (EXIT_SUCCESS);
			case '?':
				return (EXIT_FAILURE);
		}
	}
	if (optind == argc) {
		std::cerr << "Missing the 'filename' parameter!\n\n";
		print_usage(argv[0]);
		return (EXIT_FAILURE);
	}
	input_filename = argv[optind];
	if (optind + 1 < argc) {
		std::cerr << "Too many parameters!\n\n";
		print_usage(argv[0]);
		return (EXIT_FAILURE);
	}
	/* command line options parsing complete */
	if (compute_ipm_from_file(input_filename,
				input_file_encoding,
				display_character_occurrences,
				&computed_ipm) > 0) {
		std::cerr << "Error: the call to the function "
			"compute_ipm_from_file\n"
			"has been unsuccessful. Exiting!\n";
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
