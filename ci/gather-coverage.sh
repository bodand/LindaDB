#!/usr/bin/env sh

profraws=$(find . -type f -name "*.profraw")
raws_count=$(echo "$profraws" | wc -l)
echo "Collected $raws_count raw datafiles:"
echo "$profraws"
llvm-profdata merge $profraws -o collected.profdata

test_exes=$(find . -type f -name "cg3-*-test" | sed -e 's/^/--object /' | tr "\n" ' ' | cut -f'2-' -d' ')
test_count=$(echo "$test_exes" |tr ' ' "\n"| wc -l)
echo "Collected $test_count executables:"
echo "$test_exes" | tr ' ' "\n"
llvm-cov export --format=lcov --instr-profile collected.profdata $test_exes >lcov.info

echo "Uploading to Coveralls ${CIRRUS_BRANCH}/${CIRRUS_BUILD_ID}"
coveralls-lcov -t "${COVERALLS_TOKEN}" -b "${CIRRUS_BRANCH}" -s "cirrus-ci" --service-job-id="${CIRRUS_BUILD_ID%}" lcov.info
