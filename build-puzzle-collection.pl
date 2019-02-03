#!/usr/bin/perl -lw

use File::Slurp qw(read_file);
use JSON;
use List::Util qw(shuffle);

sub build {
    my ($h, $w, $accept, $score, $add_easy_first) = @_;
    my @files = glob("puzzledb/h=${h}_w=${w}_*");
    my @records = ();
    
    for my $file (@files) {
        for my $row (read_file $file) {
            $row =~ s/,$//;
            my $record = decode_json $row;
            $record->{file} = $file;
            push @records, $record;
        }
    }

    for my $record (@records) {
        next if !$accept->($record);
        my $cls = $record->{classification};
        $record->{score} =
            ($score ? $score->($cls) : 0) +
            $cls->{cover}{depth} +
            $cls->{cant_fit}{depth} +
            $cls->{square}{depth} * 10 +
            $cls->{dep}{depth} * 50 +
            ($cls->{one_of}{depth} // 0) * 20 -
            $cls->{all}{max_width} * 2;
    }

    my @output = ();
    my @sorted = sort { $b->{score} <=> $a->{score} }
                 grep { defined $_->{score} } @records;

    for my $record (@sorted) {
        push @output, $record;
        my $cls = $record->{classification};
        print STDERR join " ", $record->{score},
            $cls->{all}{max_width},
            "|",
            $cls->{cover}{depth},
            $cls->{cant_fit}{depth},
            $cls->{square}{depth},
            $cls->{one_of}{depth} // 0,
            $cls->{dep}{depth},
            $record->{file};
        last if @output >= 99;
    }

    my @shuffled = shuffle @output;
    if ($add_easy_first) {
        for (0..4) {
            my $record = $sorted[$#sorted * (1 - $_ / 10.0)];
            $shuffled[$_] = $record;
        }
    }

    [@shuffled];
}

sub tutorial {
    ({ puzzle => ["    ",
                  "    ",
                  ".  4",
                  "    "],
       message => "Drag the 4 over the dot. Then click 'Done' to check the solution."
     },
     { puzzle => ["    ",
                  "    ",
                  ".3. ",
                  "    "],
       message => "Lines can extend in both directions. Cover both of the dots by drawing a line from the 3 to the left, and then again to the right. Then click 'Done'."
     },
     { puzzle => [" 4  ",
                  "    ",
                  ". 3 ",
                  "    "],
       message => "Drag the 3 over the dot. That means the 4 will only fit in horizontally. Draw that line too, and click 'Done'."
     },
     { puzzle => undef,
       message => "You should be good to go now! But take care: the actual puzzles might require types of logical deduction that weren't part of this tutorial."
     })
}

sub add_all_done {
    [@{$_[0]},
     { puzzle => undef,
       message => "Congratulations, you've solved all " .
           "available puzzles for this difficulty level."
     }]
}

print encode_json {
    # Hand-built examples with explanatory text
    tutorial => [tutorial],
    # Easy mode, must be solvable with just the rote rule.
    easy => add_all_done(
        build 9, 6, sub {
            my $r = shift;
            ($r->{classification}{cover}{depth} > 1 &&
             !$r->{classification}{square}{depth} &&
             !$r->{classification}{dep}{depth} &&
             !$r->{classification}{one_of}{depth})
        }, sub {
            my $cls = shift;
            $cls->{cant_fit}{depth} * 1.25
        }, 1),
    # Like easy, but a little larger levels.
    medium => add_all_done(
        build 10, 7, sub {
            my $r = shift;
            ($r->{classification}{cover}{depth} > 5 &&
             !$r->{classification}{square}{depth} &&
             !$r->{classification}{dep}{depth} &&
             !$r->{classification}{one_of}{depth})
        }, sub {
            my $cls = shift;
            $cls->{cant_fit}{depth} * 1.1
        }, 1),
    # Must include some dedeuction based on corners of a
    # rectangle.
    hard => add_all_done(
        build 11, 8, sub {
            my $r = shift;
            ($r->{classification}{square}{depth} &&
             !$r->{classification}{dep}{depth} &&
             !$r->{classification}{one_of}{depth})
        }),
    # Everything.
    expert => add_all_done(
        build 13, 9, sub {
            1
        }, sub {
            1
        }),
};


