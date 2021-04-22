#!/usr/bin/perl -w

use strict;
use Data::Dumper qw(Dumper);

if (@ARGV != 1) {
     print "Usage: filter_dups <list>\n";
}

my $fh;
my $line;

my %list;
open($fh, '<:encoding(UTF-8)', $ARGV[0]) or die "Could not open file '$a' $!";
while ($line = <$fh>) {
    chomp $line;

    $line =~ /^(.*?):(.*?):(.*?)$/;
    my $key = $1 . ":" . $3;

    if (not exists $list{$key}) {
        $list{$key} = ();
        print $line, "\n";
    }
}

close($fh);
