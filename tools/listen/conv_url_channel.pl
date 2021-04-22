#!/usr/bin/perl -w

# liest eine hbbtv_urls.list und eine channels.conf ein und erzeugt ein json file

use strict;
use Data::Dumper qw(Dumper);

if (@ARGV != 2) {
     print "Usage: conv_url_channel.pl <app.list> <channels.conf>\n";
}

my $fh;
my $line;

# read channels.conf. Channel names are normalized and possible duplicates are ignored
my %channels;
open($fh, '<:encoding(UTF-8)', $ARGV[1]) or die "Could not open file '$a' $!";
while ($line = <$fh>) {
    chomp $line;

    my @ch = split /:/, $line;

    if (@ch >= 13) {
        chomp $ch[12];
        $ch[0] =~ s/^(.*?)[,;](.*?)$/$1/;
        $ch[0] =~ s/(.*?) HD$/$1/;
        $ch[0] =~ s/(.*?) HD (.*?)$/$1 $2/;
        $ch[0] =~ s/(.*?) \(S\)$/$1/;
        $ch[0] =~ s/_//g;

        if ($ch[0] !~ /^service/i) {
            my $v = $ch[10] . "." . $ch[11] . "." . $ch[9];
            $channels{$ch[0]}{$v} = ();
        }
    }
}

close($fh);

# read app list
my %urls;
open($fh, '<:encoding(UTF-8)', $ARGV[0]) or die "Could not open file '$a' $!";
while ($line = <$fh>) {
    chomp $line;

    $line =~ /^(.*?):(.*?):(.*?):(.*?)$/;

    my $appid = $1;
    my $name = $2;
    my $channels = $3;
    my $url = $4;

    my @ch = split /\,/, $channels;

    foreach my $chname (@ch) {
        $chname =~ s/^\s+|\s+$//g;
        $chname =~ s/(.*?) HD$/$1/;
        $chname =~ s/(.*?) HD (.*?)$/$1 $2/;
        $chname =~ s/(.*?) \(S\)$/$1/;
        $chname =~ s/_//g;

        #filter duplicates
        $urls{$chname . ":" . $name . ":" . hex($appid) . ":" . $url} = ();
    }
}

close($fh);

# merge channels and urls
foreach my $u (sort keys %urls) {
   $u =~ /^(.*?):(.*?):(.+?):(.*?)$/;

   my $ch = $channels{$1};
   foreach my $c (sort keys %$ch) {
      print $1, ":", $c, ":", $3, ":", $2, ":", $4, "\n";
   }
}

