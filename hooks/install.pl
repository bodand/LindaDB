#!/usr/bin/perl
use v5.32;
use utf8;
use strict;
use warnings;
no warnings qw/experimental::signatures/;
use experimental qw/signatures/;

sub install_hook :prototype($@);
sub create_driver :prototype($$@);

use Cwd;
use Config;
use File::Find qw/find/;
use File::Copy qw/cp/;
use File::Path qw/rmtree/;
use File::Spec::Functions qw/abs2rel rel2abs splitdir catdir/;

my %hook_scripts = ();

my $root = qx/git rev-parse --show-toplevel/;
chomp $root;
my $githooks = qx/git rev-parse --git-path hooks/;
chomp $githooks;
$githooks = rel2abs($githooks);

my $hooks = $root . "/hooks";
say "Hooks directory is $hooks, installing to $githooks";
chdir $hooks;

find {
    wanted     => \&wanted,
    preprocess => \&preprocess_dir,
    no_chdir   => !!1
}, Cwd::getcwd();

while ((local $_ = <"$githooks/*">)) {
    next if -d;
    next if /\.sample$/;

    my @dirs = splitdir $_;
    my $hook = $dirs[-1];
    unless (exists $hook_scripts{$hook}) {
        say "stale hook: $dirs[-1] -- deleting";

        rmtree("$githooks/$hook.d");
        unlink
    }
}
for my $hook (keys %hook_scripts) {
    install_hook($hook, $hook_scripts{$hook}->@*);
}

sub wanted() {
    return if -d; # ignore directories

    local $_ = abs2rel($_);
    return if /^(my|\.)$/; # ignore my directory

    my @dirs = splitdir $_;
    shift @dirs if $dirs[0] eq 'my'; # handle my/ as it wasn't even a thing

    my $hook = $dirs[0];
    $hook_scripts{$hook} = [] unless exists $hook_scripts{$hook};

    push $hook_scripts{$hook}->@*, { from => $_, to => $dirs[-1] };
}

sub preprocess_dir(@files) {
    if ($File::Find::dir eq $hooks) {
        # hooks root only check directories
        grep {-d} @files
    }
    else {
        @files
    }
}

sub install_hook :prototype($@) ($hook, @locals) {
    mkdir "$githooks/$hook.d";
    my $need_new_driver = !(-x "$githooks/$hook") || (-M "$githooks/$hook") < (-M $0);
    my @scripts = ();

    for my $script (@locals) {
        my $from = rel2abs($script->{from}, $hooks);
        my $to = rel2abs("$hook.d/" . $script->{to}, $githooks);
        print "copying $from -> $to...";
        push @scripts, $to;
        if (!(-e $to) || -M $from > -M $to) {
            cp $from, $to;
            $need_new_driver = !!1;
            say "done";
        }
        else {
            say "cached";
        }
    }

    create_driver $hook, "$githooks/$hook", @scripts if $need_new_driver;
}

sub create_driver :prototype($$@) ($hook, $file, @scripts) {
    open my $driver_fh, '>', $file
        or die "Cannot generate driver for hook '$hook': $!";
    say $driver_fh "#!$Config{perlpath}";
    say $driver_fh "";
    say $driver_fh "# WARNING #";
    say $driver_fh "# This file is automagically generated using hooks/install.pl";
    say $driver_fh "# Using the files in the hooks/ directory.";
    say $driver_fh "# For information see hooks/README.adoc about the repository's hook mgmt system";
    say $driver_fh "";
    for my $script (@scripts) {
        $script =~ s,\\,\\\\\\\\,g; # handle Windows-paths
        # TODO: handle Windows not executing arbitrary things
        say $driver_fh "system(q!$script!, \@ARGV)";
    }
    chmod 755, $file;
}
