#!/usr/bin/perl -lw

use File::Slurp qw(read_file);
use JSON;
use List::Util qw(shuffle);

sub build {
    my ($h, $w, $accept) = @_;
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
            $record->{classification}{all}{depth};
    }

    my @output = ();
    my @sorted = sort { $b->{score} <=> $a->{score} }
                 grep { defined $_->{score} } @records;

    for my $record (@sorted) {
        push @output, { "puzzle" => $record->{puzzle} };
        # print join " ", $record->{score},
        #     $record->{classification}{all}{max_width},
        #     $record->{classification}{all}{depth},
        #     $record->{file};
    }

    return [shuffle @output[0..99]];
}

# Easy mode, must be solvable with just the rote rule.
print encode_json {
    easy => build 10, 7, sub { my $r = shift; $r->{classification}{basic}{solved} },
};

