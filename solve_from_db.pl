#!/usr/bin/perl -lw

use strict;

use File::Slurp;
use JSON;

my $diff = shift;
my $index = shift;

my $puzzles = decode_json scalar read_file 'www/data/puzzles.json';
my $puzzle = $puzzles->{$diff}[$index -1];
my $str = join ',', '', @{$puzzle->{puzzle}};

my $w = length $puzzle->{puzzle}[0];
my $h = scalar @{$puzzle->{puzzle}};
my $p = ("$str") =~ tr/0-9//;

die if system "MAP_HEIGHT=$h MAP_WIDTH=$w PIECES=$p cmake . && make ";
my $res = qx(bin/mklinjat --solve="$str" --solve_progress_file=foo);

    
    
