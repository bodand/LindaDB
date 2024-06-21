use v5.26;
use strict;
use warnings;
use feature qw/signatures/;
no warnings qw/experimental::signatures/;

my ($min, $max) = @ARGV;
unless (defined $max) {
    $max = $min;
    $min = 10;
}
$max = 500 unless defined $max;
say "$min..$max";

my @src_range = $min..$max;

open my $make, '>', 'Makefile'
    or die "Cannot create Makefile: $!";
mk_makefile($make, @src_range);

open my $ninja, '>', 'build.ninja'
    or die "Cannot create build.ninja: $!";
mk_ninja($ninja, @src_range);

open my $lb, '>', 'build.lb'
    or die "Cannot create build.lb: $!";
mk_lb($lb, @src_range);

open my $fib_common, '>', 'fib_common.hxx'
    or die "Cannot create fib_common.hxx: $!";
mk_fib_common($fib_common);
for my $lim (@src_range) {
    open my $fh, '>', "fib$lim.hxx"
        or die "cannot create fib$lim.hxx: $!";
    mk_fib_head($fh, $lim);

    open $fh, '>', "fib$lim.cxx"
        or die "cannot create fib$lim.cxx: $!";
    mk_fib_src($fh, $lim);

    open $fh, '>', "get-fib$lim.cxx"
        or die "cannot create get-fib$lim.cxx: $!";
    mk_entry_point($fh, $lim);
}

sub mk_entry_point($fh, $lim) {
    say {$fh} <<~"EOF";
    // entry-point of get-fib$lim.exe
    #include <iostream>
    #include "fib$lim.hxx"

    int
    main() {
        int x;
        if (!std::cin >> x) {
            std::cout << "Invalid!\\n";
            return 1;
        }

        for (int i = 0; i < x; ++i) {
            std::cout << "fib($lim) = " << fib$lim() << "\\n";
        }
    }
    EOF
}

sub mk_fib_common($fh) {
    say {$fh} <<~"EOF";
    template<unsigned long long X>
    struct fib {
        constexpr const static unsigned long long value =
            fib<X - 1>::value
            + fib<X - 2>::value;
    };

    template<>
    struct fib<0ull> {
        constexpr const static auto value = 0ull;
    };
    template<>
    struct fib<1ull> {
        constexpr const static auto value = 1ull;
    };
    template<>
    struct fib<2ull> {
        constexpr const static auto value = 1ull;
    };
    EOF
}

sub mk_fib_head($fh, $lim) {
    say {$fh} <<~"EOF";
    // header for fib$lim()
    long long
    fib$lim();
    EOF
}

sub mk_fib_src($fh, $lim) {
    say {$fh} <<~"EOF";
    // source for fib$lim.hxx's declarations
    #include "fib$lim.hxx"
    #include "fib_common.hxx"

    long long
    fib$lim() {
        return fib<$lim>::value;
    }
    EOF
}

sub mk_makefile($fh, @range) {
    state $exe;
    unless (defined $exe) {
        $exe = '';
        $exe = '.exe' if $^O =~ /Win/
    }

    say {$fh} <<~"EOF";
    .POSIX:
    .SUFFIXES:
    .SUFFIXES: .cxx .o

    CXX = g++

    .cxx.o: \${<:.cxx=.hxx}
    \t\${CXX} -c -ftemplate-depth=4000 -o \$\@ \$<

    all: @{[ map { "get-fib$_$exe" } @range ]}
    \techo Built!
    EOF

    for my $target (@range) {
        say {$fh} <<~"EOF";
        get-fib$target$exe: get-fib$target.o fib$target.o
        \t\${CXX} -o \$\@ get-fib$target.o fib$target.o
        EOF
    }
}

sub mk_ninja($fh, @range) {
    state $exe;
    unless (defined $exe) {
        $exe = '';
        $exe = '.exe' if $^O =~ /Win/
    }

    say {$fh} <<~"EOF";
    rule cc
        command = g++ -c -ftemplate-depth=4000 -o \$out \$in

    rule link
        command = g++ -o \$out \$in

    EOF

    for my $target (@range) {
        say {$fh} <<~"EOF";
        build get-fib$target.o: cc get-fib$target.cxx
        build fib$target.o: cc fib$target.cxx
        build get-fib$target$exe: link get-fib$target.o fib$target.o
        EOF
    }
}

sub mk_lb($fh, @range) {
    state $exe;
    unless (defined $exe) {
        $exe = '';
        $exe = '.exe' if $^O =~ /Win/
    }

    for my $target (@range) {
        say {$fh} <<~"EOF";
        CC get-fib$target.o get-fib$target.cxx
        CC fib$target.o fib$target.cxx
        LINK get-fib$target$exe get-fib$target.o fib$target.o
        EOF
    }
}
