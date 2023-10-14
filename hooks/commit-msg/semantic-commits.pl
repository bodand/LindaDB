#!/usr/bin/perl
use v5.32;
use utf8;
use strict;
use warnings;
no warnings qw/experimental::signatures/;
use experimental qw/signatures/;

=pod

=head1 semantic-commits.pl

The following code runs as a B<git> commit-msg hook, and validates it against the conventional commit spec.
In case the spec may change, each point of the spec's implementation is commented, for easier conversion.
This script implements v1.0.0, which is 16 points long.

The following points are those not implemented exactly as written in the specification.

=over 2

=item 2

The type C<feat> MUST be used when a commit adds a new feature to your application or library.

Behavior: Not implemented

Reason: Semantic meaning, non-trivial to script.

=item 3

The type C<fix> MUST be used when a commit represents a bug fix for your application.

Behavior: Not implemented

Reason: Semantic meaning, non-trivial to script.

=item 4

A scope MAY be provided after a type. A scope MUST consist of a noun describing a section of the codebase
surrounded by parenthesis, e.g., C<fix(parser)>:

Behavior: Whether a scope is noun is not checked.

Reason: Feels needless to check if scope is a noun, meaninglessly expensive calculation as well.

=item 11

Breaking changes MUST be indicated in the type/scope prefix of a commit, or as an entry in the footer.

Behavior: Not implemented

Reason: Semantic meaning, non-trivial to script.

=item 13

If included in the type/scope prefix, breaking changes MUST be indicated by a C<!> immediately before the C<:>.
If C<!> is used, C<BREAKING CHANGE:> MAY be omitted from the footer section, and the commit description SHALL
be used to describe the breaking change.

Behavior: Partially implemented

Reason: Partially semantic meaning, non-trivial to script.

=back

=cut

sub parse_commit_file :prototype($);
sub try_parse_footers :prototype(_);
sub breaking_foot :prototype($);
sub status :prototype(@);

local $SIG{__DIE__} = sub($sig) {
    status $sig;
    say "checking semantic commits... died";
    die
};
my %valid_types = map {fc, 0} 'feat', 'fix',                                # R#15 (fc)
    'build', 'chore', 'ci', 'docs', 'perf', 'refr', 'rev', 'style', 'test'; # R#14

print "checking semantic commits...";

die "usage: $0 <commit-file>"
    unless defined $ARGV[0];
open my $commit_fh, '<', $ARGV[0]
    or die "failure: $!";

my ($break, $type, $scope, $subject, $body, $footers) = parse_commit_file($commit_fh);

die qq,invalid type: $type (not in @{[ map {qq/"$_"/} sort keys %valid_types ]}),
    unless exists $valid_types{$type};

say "done";

sub parse_commit_file :prototype($) ($commit) {
    my ($break, $type, $scope, $subject, $body, $footers) = (!!0, undef);

    # state machine variables
    my $expected = 'subject';

    local $/ = "";
    while (local $_ = <$commit>) {
        if ($expected eq 'subject') {
            # R#1, R#4, R#5
            die "invalid subject line; not in the form of <type>['('<scope>')']['!']': '<description>['\\n']"
                unless /(?<type>\w+)(?:\((?<scope>[^)]+)\))?(?<breaking>!?): (?<subject>[^\n]+)(?<allow_body>\n)/;

            $type = fc $+{type};
            $subject = $+{subject};
            $scope = $+{scope} if exists $+{scope};
            $break = !!1 if $+{breaking} eq '!'; # R#13.1

            # move state machine to expect body
            $expected = 'FINAL';
            $expected = 'body' if defined $+{allow_body}; # R#6
        }
        elsif ($expected eq 'body') {
            my $foots = try_parse_footers;
            if (defined $foots) {
                $break = !!1 if breaking_foot($foots);
                $footers->@{keys $foots->%*} = $foots->@{keys $foots->%*};
                $expected = 'footers';
                next;
            }
            $body .= $_; # R#7
        }
        elsif ($expected eq 'footers') {
            my $foots = try_parse_footers;
            die "only footers can follow footers"
                unless defined $foots;
            $footers->@{keys $foots->%*} = $foots->@{keys $foots->%*};
        }
        else {
            die "encountered parseable text: '$_'; missing newline?"
        }
    }

    chomp $body if defined $body;
    return $break, $type, $scope, $subject, $body, $footers
}

sub breaking_foot :prototype($) ($foot_refs) {
    return exists $foot_refs->{'BREAKING CHANGE'} # R#12
        || exists $foot_refs->{'BREAKING-CHANGE'} # R#16
}

# a paragraph is a footer if all its lines are valid footers
sub try_parse_footers :prototype(_) ($maybe_footer) {
    my $foots = {};
    # R#8, R#9, R#10
    # According to #10, the value MUST terminate when the next footer is encountered
    # I'd say this is ambiguous, as it does not specify whether an empty footer value is allowed
    # e.g.:
    #   Name: Another: asd
    #   is this parsed as ("Name",""), ("Another","asd") or some other way?
    # As of now, we implement the first behavior, as nothing specifies it is not valid, but smh, bad spec
    # Also the exact format of the footer's name is unspecified. is 234 a valid footer? or .? or \x00?
    # a word token probably disqualifies the latter two, but what a word is is not specified... smh my head again
    # as of now we only accept ASCII alphanumerics and '-' as specified by R#9
    while ($maybe_footer =~ # this became uglier that expected...
        /(?<footer>\b(?:BREAKING CHANGE|[a-zA-Z0-9-]++))(?:: | #)(?<value>(?:[^: ]+?(?::(?! )| (?!#)|$))*+)/smcg) {
        my $foot = $+{footer};
        $foot = fc $foot unless $foot =~ /BREAKING[- ]CHANGE/; # R#15

        my $val = $+{value};
        chomp $val; # valid? not specified...

        $foots->{$foot} = $val;
    }
    return unless defined pos($maybe_footer); # mismatch occurred
    return $foots
}

sub status :prototype(@) (@params) {
    state $newline = !!0;
    if (!$newline) {
        $newline = !!1;
        print "\n";
    }
    say @params
}
