#!/usr/bin/perl -w

use strict;

use Digest::SHA qw(sha1_hex);
use File::Basename qw(dirname);
use File::Copy;
use File::Path qw(make_path);
use File::Slurp qw(slurp);
use File::Temp qw(tempfile);
use Fatal qw(open chmod rename symlink);

my $target = shift;

die "Usage: $0 target\n" if !$target or @ARGV;

die "Directory $target does not exist" if !-d $target;

my $tag = qx(git rev-parse HEAD);
my $devel = ($target eq 'www-devel');

my %untracked = map {
    (/ (.*)/g, 1)
} grep {
    /^\?/
} qx(git status --porcelain);

if (!$devel) {
    system "git", "diff", "--exit-code";

    if ($?) {
        die "Uncommitted changes, can't deploy";
    }
}

sub copy_with_mode {
    my ($mode, $from, $to) = @_;
    die if -d $to;

    make_path dirname $to;

    die "$from doesn't exist" if (!-f $from);

    if ($devel) {
        if (!-l $to) {
            symlink "$ENV{PWD}/$from", $to;
        }
        return 
    }

    if ($untracked{$from}) {
        die "untracked file '$from'\n"
    }

    copy $from, "$to.tmp" or die "Error copying $from to $to: $!";
    chmod $mode, "$to.tmp";
    rename "$to.tmp", $to;
}

sub mangle_with_mode {
    my ($mode, $from, $to, $mangle) = @_;
    my $data = $mangle->(scalar slurp "$from");

    my ($fh, $filename) = tempfile("tmpfileXXXXXXX",
                                   DIR=>"$target");
    print $fh $data;
    close $fh;
    chmod $mode, $filename;
    rename $filename, $to;
}

sub deploy_stc {
    mkdir "$target/";
    mkdir "$target/data/";
    for my $f (qw(data/puzzles.json
                  stc/game.css
                  stc/game.js
                  stc/third-party/Coiny-Regular.ttf
                  stc/third-party/jquery-3.3.1.min.js)) {
        copy_with_mode 0444, "www/$f", "$target/$f";
    }
#    copy_with_mode 0444, "robots.txt", "$target/robots.txt";
#    copy_with_mode 0444, "stc/favicon.ico", "$target/favicon.ico";
#    copy_with_mode 0444, "stc/favicon-inactive.ico", "$target/favicon-inactive.ico";
}

sub deploy_pages {
    mangle_with_mode 0444, "www/index.html", "$target/index.html", sub {
        my $contents = shift;
        while ($contents =~ m{"(stc/[^?"]*)"}) {
            my $filename = $1;
            my $hash = sha1_hex slurp "www/$filename";
            $contents =~ s{"$filename"}{"$filename?tag=$hash"};
        }
        $contents;
    };
}

deploy_stc;
deploy_pages;

$target =~ s/www-//;
system qq{(echo -n "$target: "; git rev-parse HEAD) >> deploy.log};
system qq{git tag -f $target};

