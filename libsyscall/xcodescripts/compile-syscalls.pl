#!/usr/bin/perl
#
# Copyright (c) 2010 Apple Inc. All rights reserved.
#
# @APPLE_OSREFERENCE_LICENSE_HEADER_START@
# 
# This file contains Original Code and/or Modifications of Original Code
# as defined in and that are subject to the Apple Public Source License
# Version 2.0 (the 'License'). You may not use this file except in
# compliance with the License. The rights granted to you under the License
# may not be used to create, or enable the creation or redistribution of,
# unlawful or unlicensed copies of an Apple operating system, or to
# circumvent, violate, or enable the circumvention or violation of, any
# terms of an Apple operating system software license agreement.
# 
# Please obtain a copy of the License at
# http://www.opensource.apple.com/apsl/ and read it before using this file.
# 
# The Original Code and all software distributed under the License are
# distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
# INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
# Please see the License for the specific language governing rights and
# limitations under the License.
# 
# @APPLE_OSREFERENCE_LICENSE_HEADER_END@
#

use warnings;
use strict;

use Data::Dumper;
use File::Spec;
use IO::File;
use File::Basename ();

my $basename = File::Basename::basename($0);

sub usage {
	print "$basename: <source list> <output archive>";
	exit 1;
}

usage unless scalar(@ARGV) >= 2;

my $sourceList = shift @ARGV;
my $outputFile = shift @ARGV;

my @includes = ();
for my $arg (@ARGV) {
	if ($arg =~ /^-[DI]/ || $arg =~ /^--target=/) {
		push(@includes, $arg);
	} else {
		usage;
	}
}

my $f = IO::File->new($sourceList, 'r');
die "$basename: $sourceList: $!\n" unless defined($f);

my @objects;
my @archs = split / /, $ENV{"ARCHS"};
my @sources = <$f>;
chomp @sources;

undef $f;

# compiler options
my $CC = $ENV{"CC"};
if (!defined($CC) || $CC eq "") {
	chomp($CC = `xcrun -sdk "$ENV{'SDKROOT'}" -find cc`);
}
my @CFLAGS = (
	"-x assembler-with-cpp",
	"-c",
);

if (defined($ENV{"SDKROOT"}) && $ENV{"SDKROOT"} ne "") {
	push(@CFLAGS, "-isysroot", $ENV{"SDKROOT"});
	push(@CFLAGS, "-I".$ENV{"SDKROOT"}."/System/Library/Frameworks/System.framework/PrivateHeaders");
}

push(@CFLAGS, @includes);

my $LIBTOOL = $ENV{"LIBTOOL"};
if (!defined($LIBTOOL) || $LIBTOOL eq "") {
	chomp($LIBTOOL = `xcrun -sdk "$ENV{'SDKROOT'}" -find libtool`);
}
my @LIBTOOLFLAGS = (
	"-static",
);

# architectures
for my $arch (@archs) {
	push(@CFLAGS, "-arch $arch");
}

# do each compile
my $jobs = $ENV{"JOBS"};
if (!defined($jobs) || $jobs !~ /^\d+$/ || $jobs < 1) {
	$jobs = `getconf _NPROCESSORS_ONLN 2>/dev/null`;
	$jobs = `sysctl -n hw.ncpu 2>/dev/null` if $jobs !~ /^\d+$/ || $jobs < 1;
	$jobs = 1 if $jobs !~ /^\d+$/ || $jobs < 1;
}

for my $src (@sources) {
	if ($jobs == 0) {
		if (wait != -1) {
			$jobs++;
		} else {
			printf "wait exited with -1 (no children) and exhausted allowed jobs. Exiting.\n";
			exit 1;
		}
		
		if ($? != 0) {
			printf "$CC exited with value %d\n", $? >> 8;
			exit 1;
		}
	}
	
	(my $o = $src) =~ s/\.s$/\.o/;
	my $compileCommand = "$CC " . join(' ', @CFLAGS) . " -o $o $src";
	printf $compileCommand . "\n";

	$jobs--;
	my $pid = fork();
	if ($pid == 0) {
		exec($compileCommand);
	}
	push(@objects, $o);
}

while (wait != -1) {
	if ($? != 0) {
		printf "$CC exited with value %d\n", $? >> 8;
		exit 1;
	}
}

printf "Finished assembly, beginning link.\n";

# final link

if (-f $outputFile) {
	unlink($outputFile);
}

my $linkCommand = "$LIBTOOL " . join(' ', @LIBTOOLFLAGS) . " -o $outputFile " . join(' ', @objects);

printf $linkCommand . "\n";
system($linkCommand);
if ($? != 0) {
	print "$LIBTOOL exited with value %d\n", $? >> 8;
	exit 1;
}
