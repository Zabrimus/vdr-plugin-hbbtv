#!/usr/bin/perl

use strict;

my %list;
my $chan;

while (my $line = <>) {
    $line =~ s/(.*?~~~)(.*)/\2/;
    # $line =~ /^(.*~~~)[3](.*?)~~~.*?$/;
    $line =~ /^(.*?)~~~(.*?)~~~(.*?)~~~(.*?)~~~(.*?).*?$/;

    $chan = $4;
    $list{uc $chan} = $line;
}

foreach (sort keys %list) {
    print $list{$_};
}