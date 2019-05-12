#!/usr/bin/perl -lw

use strict;
use JSON;
use List::Util qw(min max);

# Fixup the almost-json.
my $data = join '', <>;
$data = "[$data]";
$data =~ s/,\s*(\]|\})/$1/gsm;

my $boards = decode_json $data;
my $sz = 32;
my $hsz = $sz / 2;
my %prev;

sub draw_board {
    my $board = shift;
    my $height = $board->{height} * $sz;
    my $width = $board->{width} * $sz;
    my $hwidth = $width / 2;

    if ($board->{type}) {
        print "<text x='$hwidth' y='18' text-anchor='middle' fill='black' class='text'>$board->{type}</text>";
    }
    if (exists $board->{iter}) {
        print "<text x='5' y='18' text-anchor='left' fill='black' class='text'>Iteration: $board->{iter}</text>";
    }
    if (exists $board->{score}) {
        print "<text x='$hwidth' y='18' text-anchor='middle' fill='black' class='text'>Score: $board->{score}</text>";
    }
    print "<g transform='translate(5, 21)'>";    
    print "<rect x='0' y='0' height='$height' width='$width' stroke='black' stroke-width='2' fill='white'/>";
    for my $line (@{$board->{lines}}) {
        my $x = ($line->{c} + 0.5) * $sz;
        my $y = ($line->{r} + 0.85) * $sz;
        print "<text x='$x' y='$y' text-anchor='middle' fill='black' class='number'>$line->{value}</text>";
        if (exists $line->{hz}) {
            my $minx = $line->{minc} * $sz + $hsz;
            my $miny = $line->{minr} * $sz + $hsz;
            my $maxx = $line->{maxc} * $sz + $hsz;
            my $maxy = $line->{maxr} * $sz + $hsz;
            if ($line->{hz}) {
                $minx -= $sz * 0.4;
                $maxx += $sz * 0.4;
            } else {
                $miny -= $sz * 0.4;
                $maxy += $sz * 0.4;
            }
            my $key = "$x $y";
            my $val = "$minx $miny $maxx $maxy";
            my $width = 5;
            my $color = '#f00';
            my $dash = '';
            if (!defined $prev{$key} or $prev{$key} ne $val) {
                $dash = '3,3';
                $prev{$key} = $val;
            }
            if ($line->{value} == max($line->{maxr} - $line->{minr},
                                      $line->{maxc} - $line->{minc}) + 1) {
                $color = '#4c4';
            }
            print "<line x1='$minx' y1='$miny' x2='$maxx' y2='$maxy' stroke-width='$width' stroke='$color' stroke-dasharray='$dash' stroke-opacity='0.75'/>";
        }
    }
    for my $dot (@{$board->{dots}}) {
        my $x = $dot->{c} * $sz + $hsz;
        my $y = $dot->{r} * $sz + $hsz;
        print "<circle r='3' cx='$x' cy='$y' height='$height' fill='black'/>";
    }
    print "</g>";

    return ($width, $height + 16);
}

print qq(<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
  <style>
    .number { font-size: 28px; }
    .text { font-size: 16px; }
  </style>
  <rect width="100%" height="100%" fill="white"></rect>
);

my $x_offset = 5;
my $y_offset = 5;
for my $board (@{$boards}) {
    print "<g transform='translate($x_offset, $y_offset)'>";
    my ($w, $h) = draw_board $board;
    $x_offset += 5 + $w;
    if ($x_offset > 1600) {
        $x_offset = 5;
        $y_offset += 16 + $h;
    }
    print "</g>";
}
print qq(</svg>);
