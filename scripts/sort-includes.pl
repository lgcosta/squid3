#!/usr/bin/perl
#
# AUTHOR: Francesco Chemolli <kinkie@squid-cache.org> 
#
# Part of the Squid Web Cache project, licensed for use and distribution
# under the terms of the Squid Web Cache; please refer to the files COPYING
# and COPYRIGHT.
#
# 
# USAGE: sort-includes.pl filename.cc >filename.cc.sorted
#
# This tool helps to sort the #include directives in a c or c++ source file
# according to the Squid Coding guidelines.
# 
# The output of the tool is a source file where each block of consecutive
# include directives for project-specific files (#include "header.h")
# is sorted with this specification: squid.h (if present) is alwasy first,
# then the other directives are sorted in case-insensitive alphabetical order.
#
# Suggested usage:
# for file in $(find . -name \*.cc); do /full/path/to/sort-includes.pl $file >$file.sorted; mv $file.sorted $file; done

use strict;
use warnings;
my @acc=(); #if empty, we're not accumulating
while (<>) {
  if (m!^#include "!) {
    if (m!squid.h!) {
      print;
    } else {
      push @acc,$_;
    }
  } else {
    &dump_acc;
    print;
  }
}
&dump_acc;

sub dump_acc {
  return unless @acc;
  print sort {lc($a) cmp lc($b)} @acc;
  @acc=();
}
