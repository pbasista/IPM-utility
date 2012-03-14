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

/* a feature test macro, which enables the support for large files (> 2 GiB) */
#define _FILE_OFFSET_BITS 64

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iconv.h>
#include <iostream>
#include <iomanip>
#include <map>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/* auxiliary functions */

unsigned long long compute_gcd (unsigned long long a,
		unsigned long long b) {
	unsigned long long c = 0;
	for (;;) {
		c = a % b;
		if (c == 0) {
			return (b);
		}
		a = b;
		b = c;
	}
}

int text_file_read_buffer (int fd,
		size_t buffer_size,
		char *buffer,
		size_t *bytes_read) {
	ssize_t read_retval = read(fd, buffer, buffer_size);
	/* we check whether the read has encountered an error */
	if (read_retval == (-1)) {
		perror("text_file_read_buffer: read");
		/* resetting the errno */
		errno = 0;
		return (1); /* failure */
	/* if we have reached the end of the input file */
	} else if (read_retval == 0) {
		(*bytes_read) = 0;
		return (-1); /* partial success */
	}
	(*bytes_read) = (size_t)(read_retval);
	return (0); /* success */
}

int text_file_convert_buffer (iconv_t *cd,
		size_t bytes_read,
		char *buffer,
		wchar_t *wbuffer,
		size_t wbuffer_size,
		size_t *characters_read) {
	char *inbuf = buffer;
	char *outbuf = (char *)(wbuffer);
	size_t inbytesleft = bytes_read;
	size_t outbytesleft_at_start = wbuffer_size;
	size_t outbytesleft = wbuffer_size;
	size_t retval = 0;
	/*
	 * we try to use iconv to convert the characters
	 * in the input buffer to the characters in the output buffer
	 */
	retval = iconv((*cd), &inbuf, &inbytesleft,
			&outbuf, &outbytesleft);
	/* if the iconv has encountered an error */
	if (retval == (size_t)(-1)) {
		perror("text_file_convert_buffer: iconv");
		/* resetting the errno */
		errno = 0;
		return (1);
	} else if (retval > 0) {
		std::cerr << "text_file_convert_buffer: iconv "
			"converted " << retval << " characters\n"
			"in a nonreversible way!\n";
		return (2);
	} else if (outbytesleft == 0) {
		/*
		 * all the characters expected to be read
		 * from the input buffer have already been read
		 */
	} else if (inbytesleft > 0) {
		std::cerr << "text_file_convert_buffer: iconv could not "
			"convert " << inbytesleft << " input bytes!\n";
		return (3);
	}
	/*
	 * now we compute the number of characters,
	 * which have just been converted from the input buffer
	 */
	(*characters_read) = (outbytesleft_at_start -
			outbytesleft) / sizeof (wchar_t);
	return (0);
}

int text_file_get_character_occurrences(std::map<wchar_t, size_t> &occurrences,
		wchar_t *wbuffer,
		size_t characters_read) {
	size_t i = 0;
	for (; i < characters_read; ++i) {
		/*
		 * FIXME: We are strongly relying on
		 * the size_t being initialized to zero!
		 * How portable is this?
		 */
		++occurrences[wbuffer[i]];
	}
	return (0);
}

int compute_ipm_from_file (const char *input_filename,
		const char *input_file_encoding,
		int display_character_occurrences,
		long double *computed_ipm) {
	int retval = 0;
	int iconv_retval = 0;
	int fd = 0;
	/* the conversion descriptor used by the iconv */
	iconv_t cd = NULL; /* iconv_t is just a typedef for void* */
	char *buffer = NULL;
	const char *internal_character_encoding = NULL;
	wchar_t *wbuffer = NULL;
	char *inbuf = NULL;
	char *outbuf = NULL;
	size_t inbytesleft = 0;
	size_t outbytesleft = 0;
	size_t buffer_size = 8388608; /* 8 Mi */
	size_t wbuffer_size = 8388608 * sizeof (wchar_t); /* 8 Mi */
	size_t bytes_read = 0;
	size_t characters_read = 0;
	unsigned long long total_characters_read = 0;
	unsigned long long matching_pairs = 0;
	unsigned long long numerator = 0;
	unsigned long long denominator = 1; /* avoiding the implied division by zero */
	unsigned long long gcd = 0;
	std::map<wchar_t, size_t> occurrences;
	std::cout << "Will now try to compute the inverse "
		"probability of matching (IPM)\n"
		"of the input file '" << input_filename << "'\n\n";
	/* we try to open the input file for reading */
	fd = open(input_filename, O_RDONLY);
	if (fd == (-1)) {
		perror("compute_ipm_from_file: open");
		/* resetting the errno */
		errno = 0;
		return (1);
	}
	/*
	 * We check the current size of the character_type
	 * and decide which internal character encoding to use.
	 */
	if (sizeof (wchar_t) == 1) {
		/*
		 * we can not use Unicode, so by default we stick
		 * to the basic ASCII encoding
		 */
		internal_character_encoding = "ASCII";
	} else if ((sizeof (wchar_t) > 1) && (sizeof (wchar_t) < 4)) {
		/*
		 * We can use limited Unicode (Basic Multilingual Plane,
		 * or BMP only). We prefer UCS-2 to UTF-16, because we would
		 * not like to deal with the byte order marks (BOM).
		 */
		/* we suppose we are on the little endian architecture */
		internal_character_encoding = "UCS-2LE";
	} else { /* sizeof (wchar_t) >= 4 */
		/*
		 * We can use full Unicode (all the code points). We prefer
		 * UCS-4 to UTF-32, because we would not like to deal
		 * with the byte order marks (BOM).
		 */
		/* again, we suppose the little endian architecture */
		internal_character_encoding = "UCS-4LE";
	}
	/* we create the desired conversion descriptor */
	if ((cd = iconv_open(internal_character_encoding,
				input_file_encoding)) == (iconv_t)(-1)) {
		perror("compute_ipm_from_file: iconv_open");
		/* resetting the errno */
		errno = 0;
		return (2);
	}
	try {
		buffer = new char[buffer_size];
		wbuffer = new wchar_t[buffer_size];
	}
	catch (std::bad_alloc &) {
		std::cerr << "Memory allocation error!\n";
	}
	std::cout << "The file '" << input_filename <<
		"' has been opened successfully!\n";
	std::cout << "Selected input file encoding: '" <<
		input_file_encoding << "'\n";
	std::cout << "Selected internal character encoding: '" <<
		internal_character_encoding << "'\n\n";
	std::cout << "Computing the IPM\n";
	do {
		if ((retval = text_file_read_buffer(fd, buffer_size,
						buffer, &bytes_read)) > 0) {
			std::cerr << "Error: The call to the function\n"
				"text_file_read_buffer"
				" has not been successful.\n";
			return (3);
		}
		if (text_file_convert_buffer(&cd, bytes_read, buffer,
					wbuffer, wbuffer_size,
					&characters_read) != 0) {
			std::cerr << "Error: The call to the function\n"
				"text_file_convert_buffer"
				" has not been successful.\n";
			return (4);
		}
		if (text_file_get_character_occurrences(occurrences, wbuffer,
					characters_read) != 0) {
			std::cerr << "Error: The call to the function\n"
				"text_file_get_character_occurrences"
				" has not been successful.\n";
			return (5);
		}
		total_characters_read += characters_read;
	} while (retval == 0);
	if (retval != (-1)) {
		std::cerr << "Error: The last call to the function\n"
			"text_file_read_buffer"
			" has not been successful.\n";
		return (6);
	}
	std::cout << "Successfully computed!\n\n";
	std::cout << "Total characters read:\t" << total_characters_read <<
		std::endl << std::endl;
	if (display_character_occurrences != 0) {
		std::clog << "Character occurrences:\n\n";
		if (iconv_close(cd) == (-1)) {
			perror("compute_ipm_from_file: iconv_close");
			/* resetting the errno */
			errno = 0;
			return (7);
		}
		/*
		 * we suppose that the terminal is able to handle
		 * the UTF-8 encoded characters
		 */
		if ((cd = iconv_open("UTF-8",
				internal_character_encoding)) ==
				(iconv_t)(-1)) {
			perror("compute_ipm_from_file: iconv_open");
			/* resetting the errno */
			errno = 0;
			return (8);
		}
	}
	matching_pairs = 0;
	for (std::map<wchar_t, size_t>::iterator it = occurrences.begin();
			it != occurrences.end(); ++it) {
		if (display_character_occurrences != 0) {
			wbuffer[0] = it->first;
			inbuf = (char *)(wbuffer);
			inbytesleft = sizeof (wchar_t);
			/*
			 * we suppose that any UTF-8 character
			 * will take up to 6 bytes
			 */
			buffer[0] = '\0';
			buffer[1] = '\0';
			buffer[2] = '\0';
			buffer[3] = '\0';
			buffer[4] = '\0';
			buffer[5] = '\0';
			buffer[6] = '\0';
			outbuf = buffer;
			outbytesleft = buffer_size;
			/*
			 * we try to use iconv to convert
			 * the current character to the encoding,
			 * which is printable in the terminal
			 */
			iconv_retval = iconv(cd, &inbuf, &inbytesleft,
					&outbuf, &outbytesleft);
			/* if the iconv has encountered an error */
			if (iconv_retval == (size_t)(-1)) {
				perror("compute_ipm_from_file: iconv");
				/* resetting the errno */
				errno = 0;
				return (9);
			} else if (iconv_retval > 0) {
				std::cerr << "compute_ipm_from_file: iconv "
					"converted " << iconv_retval <<
					" characters\n"
					"in a nonreversible way!\n";
				return (10);
			} else if (outbytesleft == 0) {
				/*
				 * all the characters expected to be read
				 * from the input buffer have already been read
				 */
			} else if (inbytesleft > 0) {
				std::cerr << "compute_ipm_from_file: "
					"iconv could not convert " <<
					inbytesleft << " input bytes!\n";
				return (11);
			}
			std::clog << "'" << buffer << "'\t(" <<
			(int)(it->first) << ")\t" <<
				it->second << "\n";
		}
		/*
		 * Integer division by two would be sufficient here,
		 * because we know that exactly one of the integers is even.
		 *
		 * But since we will need to multiply the matching_pairs
		 * by two later, we can abandon this delete and multiply
		 * operations completely.
		 */
		matching_pairs += (unsigned long long)(it->second) *
			(unsigned long long)(it->second - 1);
	}
	if (display_character_occurrences != 0) {
		std::clog << std::endl;
	}
	if (iconv_close(cd) == (-1)) {
		perror("compute_ipm_from_file: iconv_close 2");
		/* resetting the errno */
		errno = 0;
		return (12);
	}
	try {
		delete[] wbuffer;
		delete[] buffer;
	}
	/*
	 * We are not sure which type of exception
	 * does delete throw, if any.
	 */
	catch (std::exception &) {
		std::cerr << "Memory deallocation error!\n";
	}
	std::cout << "Alphabet size:\t" << occurrences.size() << "\n\n";
	numerator = total_characters_read * (total_characters_read - 1);
	denominator = matching_pairs;
	if (denominator == 0) {
		std::cerr << "Each letter in the input file is different!\n"
			"Cannot compute the IPM. Exiting!" << std::endl;
		return (-1);
	}
	gcd = compute_gcd(numerator, denominator);
	/* here, integer division is sufficient */
	numerator /= gcd;
	/* for the same reason as here */
	denominator /= gcd;
	std::cout << "The computed IPM (fraction):\t" << numerator << " / "
		<< denominator << std::endl;
	(*computed_ipm) = (long double)(numerator) /
		(long double)(denominator);
	std::cout << "The computed IPM (floating point):\t" <<
				 std::setprecision(10) <<
	 			 (*computed_ipm) << std::endl;
	return (0);
}
