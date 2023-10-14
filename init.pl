use v5.32;
use warnings;
use strict;
use utf8;

use Cwd qw/getcwd/;
use File::Basename qw/basename/;
use File::Find;
use File::Path qw/remove_tree/;
use Getopt::Long;
use HTTP::Tiny;
use Tie::File;

my $pwd = getcwd;
die "error: cannot figure out current working directory: Sorry, this is fatal."
    unless defined $pwd;

my %defaults = (
    license     => "BSD-3-Clause",
    keep        => !!0,
    description => '',
    summary     => '',
    project     => basename $pwd,
        hungarian => !!0,
);

sub opt;
sub read_opt;
sub read_yn;

GetOptions(
    "keep|k!"         => \&opt,
    "license|l=s"     => \&opt,
    "project|P=s"     => \&opt,
    "summary|s=s"     => \&opt,
    "description|d=s" => \&opt,
    "name|n=s"        => \&opt,
    "email|e=s"       => \&opt,
    "hungarian|H"     => \&opt,
);

say "Hello! I'm here to help you configure this new C++ project!";

# steal data from git, if possible
my $havegit = !!0;
if (-d '.git') {
    $havegit = !!1;
    opt name => qx/git config user.name/;
    opt email => qx/git config user.email/;
}

# project input
if (not opt '?' => 'project') {
    say "What is the name of the project? [$defaults{project}]";
    read_opt 'project'
}

# name
if (not opt '?' => 'name') {
    say "What is your name? (Needed for the license) [@{[ opt 'name' // '' ]}]";
    read_opt 'name'
}

# email
if (not opt '?' => 'name') {
    say "What is your email? (Needed for the license) [@{[ opt 'email' // '' ]}]";
    read_opt 'email'
}

# summary
if (not opt '?' => 'summary') {
    say "What's your project's summary (shorter than descroiption, a sentence)? [$defaults{summary}]";
    read_opt 'summary'
}

# description
if (not opt '?' => 'description') {
    say "What's your project's description (longer than summary)? [$defaults{description}]";
    read_opt 'description'
}

# license input
my $license_text;
my (undef, undef, undef, undef, undef, $year) = localtime(time);
$year += 1900;
opt year => $year;
if (not opt '?' => 'license') {
    while (1) {
        say "What license should the project be under? [$defaults{license}]";
        my $license = <STDIN>;
        $license =~ s/\A\s+|\s+\z//g;
        last if $license eq '';
        opt license => $license;
        my $res = HTTP::Tiny->new->get(
            "https://raw.githubusercontent.com/spdx/license-list-data/main/text/$license.txt");
        next unless $res->{success};
        $license_text = $res->{content};
    }
}

unless (defined $license_text) {
    my $license = opt 'license';
    my $res = HTTP::Tiny->new->get(
        "https://raw.githubusercontent.com/spdx/license-list-data/main/text/$license.txt");
    die <<"EOF" unless $res->{success};
error: invalid license specified: $license: Sorry, this is fatal even if it could be not
EOF
    $license_text = $res->{content};
}

$license_text =~ s/<year>/$year/g;
$license_text =~ s/<owner>/@{[ opt 'name' ]} <@{[ opt 'email' ]}>/g;
opt license_text => $license_text;

say "Please note to check the license file and the CMakeLists.txt header, \
for the template fields are not deterministic in the SPDX db.";

if (open my $lf, '>', 'LICENSE.adoc') {
    print $lf '</project/> license';
    print $lf $license_text;
}
else {
    say "warning: cannot open license file: generation will be skipped";
}

# templating
find sub {
    return if -d;
    return if $File::Find::name eq $0;

    my @lines;
    if (tie @lines, 'Tie::File', "$pwd/$_") {
        for (@lines) {
            s,</([^/]+)/>,opt $1,ge;
        }
    }
    else {
        say "warning: cannot edit $_, which may or may not stay in an invalid state now. Take care."
    }

}, $pwd;

# git 
unless ($havegit) {
    say "You don't seem to have a git repository. Would you like one? [yes]";
    if (read_yn()) {
        say "Certainly.";
        system "git init";
        system "git branch -M trunk";
    }
    else {
        say "If you wish so..."
    }
}

# hooks
if ($havegit) {
    say "Installing hooks for git";
    system "hooks/install.pl";
}
else {
    say "Deleting hooks for you don't have a git repository.";
    remove_tree 'hooks';
}


# epilogue
unless (opt 'keep') {
    if (opt 'hungarian') {
        say <<'EOF';
Be van fejezve a nagy mű, igen.
A gép forog, az alkotó pihen.
EOF
    }
    else {
        say <<'EOF';
The mighty masterpiece at last is done;
The wheel revolves; the master on his throne
Doth rest.
EOF
    }
    unlink $0
}

sub opt {
    state %values = ();
    my $valname = shift;
    if (@_ == 0) {
        die "error: invalid use of opt: Sorry this is a bug, but fatal nontheless"
            if $valname eq '?';
        $values{$valname} // $defaults{$valname}
    }
    else {
        if ($valname eq '?') {
            exists $values{$valname} and defined $values{$valname}
        }
        else {
            $values{$valname} = shift
        }
    }
}

sub read_opt {
    my ($opt) = @_;
    my $data = <STDIN>;
    $data =~ s/\A\s+|\s+\z//g;
    opt $opt, $data if $data ne '';
}

sub read_yn {
    my $default = shift // 'yes';

    my $data = <STDIN>;
    $data = $default if $data =~ /\A\s*\z/;
    return $data =~ /\A\s*y(?:e(?:s)?)?\s*\z/
}
