#!/usr/bin/perl -lw

use File::Slurp qw(read_file);
use JSON;
use List::Util qw(shuffle);

sub build {
    my ($h, $w, $accept, $score) = @_;
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
        $record->{score} =
            (10 - $record->{classification}{all}{max_width}) + 
            $record->{classification}{all}{depth} +
            (defined $score ? $score->($record) : 0);
    }

    my @output = ();
    my @sorted = sort { $b->{score} <=> $a->{score} }
                 grep { defined $_->{score} } @records;

    for my $record (@sorted) {
        push @output, { "puzzle" => $record->{puzzle} };
        print STDERR join " ", $record->{score},
            $record->{classification}{all}{max_width},
            $record->{classification}{all}{depth},
            $record->{classification}{no_dep}{solved},
            $record->{classification}{no_square}{solved},
            $record->{file};
        last if @output >= 99;
    }

    return [shuffle @output];
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
    # Like easy, but fewer pieces
    tutorial => [tutorial],
    easy => add_all_done(build 9, 6, sub { my $r = shift; $r->{classification}{basic}{solved} }),
    # Easy mode, must be solvable with just the rote rule.
    medium => add_all_done(build 10, 7, sub { my $r = shift; $r->{classification}{basic}{solved} }),
    # Medium. Deduction based on corners of a rectangle, with
    # one pair of opposite corners having numbers, the other pair
    # having dots.
    hard => add_all_done(build 11, 8, sub { my $r = shift; !$r->{classification}{no_square}{solved} && $r->{classification}{no_dep}{solved}}),
    expert => add_all_done(
        build 13, 9, sub {
            my $r = shift;
            # !$r->{classification}{no_dep}{solved}
            1
        }, sub {
            my $r = shift;
            $r->{classification}{no_dep}{solved} * -10 + $r->{classification}{no_square}{solved} * -5
        }),
};


