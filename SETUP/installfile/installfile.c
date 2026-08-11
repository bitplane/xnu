/*
 * Copyright (c) 2012 Apple, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <err.h>
#include <sysexits.h>

#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/param.h>
#include <sys/time.h>

#ifndef O_SYMLINK
#define O_SYMLINK 0
#endif

void usage(void);

static const char *program_name;

static int
copy_file(int srcfd, int dstfd)
{
	char buffer[64 * 1024];
	ssize_t count;

	while ((count = read(srcfd, buffer, sizeof(buffer))) != 0) {
		ssize_t offset = 0;

		if (count < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}

		while (offset < count) {
			ssize_t written = write(dstfd, buffer + offset, count - offset);

			if (written < 0) {
				if (errno == EINTR)
					continue;
				return -1;
			}
			if (written == 0) {
				errno = EIO;
				return -1;
			}
			offset += written;
		}
	}

	return 0;
}

int main(int argc, char * argv[])
{
	struct stat sb;
	mode_t mode;
	bool gotmode = false;
	int ch;
	int ret;
	int srcfd, dstfd;
	const char *src = NULL;
	const char *dst = NULL;
	char dsttmpname[MAXPATHLEN];

	program_name = strrchr(argv[0], '/');
	program_name = program_name == NULL ? argv[0] : program_name + 1;

	while ((ch = getopt(argc, argv, "cSm:")) != -1) {
		switch(ch) {
			case 'c':
			case 'S':
				/* ignored for compatibility */
				break;
			case 'm': {
				char *end;
				unsigned long value;

				gotmode = true;
				errno = 0;
				value = strtoul(optarg, &end, 8);
				if (errno != 0 || end == optarg || *end != '\0' || value > 07777)
					errx(EX_USAGE, "Unrecognized mode %s", optarg);
				mode = (mode_t)value;
				break;
			}
			case '?':
			default:
				usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 2) {
		usage();
	}

	src = argv[0];
	dst = argv[1];

	srcfd = open(src, O_RDONLY | O_SYMLINK, 0);
	if (srcfd < 0)
		err(EX_NOINPUT, "open(%s)", src);

	ret = fstat(srcfd, &sb);
	if (ret < 0)
		err(EX_NOINPUT, "fstat(%s)", src);

	if (!S_ISREG(sb.st_mode))
		err(EX_USAGE, "%s is not a regular file", src);

	snprintf(dsttmpname, sizeof(dsttmpname), "%s.XXXXXX", dst);

	dstfd = mkstemp(dsttmpname);
	if (dstfd < 0)
		err(EX_UNAVAILABLE, "mkstemp(%s)", dsttmpname);

	ret = copy_file(srcfd, dstfd);
	if (ret < 0)
		err(EX_UNAVAILABLE, "copyfile(%s, %s)", src, dsttmpname);

	ret = futimes(dstfd, NULL);
	if (ret < 0)
		err(EX_UNAVAILABLE, "futimes(%s)", dsttmpname);

	if (gotmode) {
		ret = fchmod(dstfd, mode);
		if (ret < 0)
			err(EX_NOINPUT, "fchmod(%s, %ho)", dsttmpname, mode);
	}

	ret = rename(dsttmpname, dst);
	if (ret < 0)
		err(EX_NOINPUT, "rename(%s, %s)", dsttmpname, dst);

	ret = close(dstfd);
	if (ret < 0)
		err(EX_NOINPUT, "close(dst)");

	ret = close(srcfd);
	if (ret < 0)
		err(EX_NOINPUT, "close(src)");

	return 0;
}

void usage(void)
{
	fprintf(stderr, "Usage: %s [-c] [-S] [-m <mode>] <src> <dst>\n",
			program_name);
	exit(EX_USAGE);
}
