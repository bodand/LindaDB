#!/usr/bin/env sh

rm -f ./test/default.profraw # file is broken for some reason?
profraws=$(find . -type f -name "*.profraw")
raws_count=$(echo "$profraws" | wc -l)
echo "Collected $raws_count raw datafiles:"
echo "$profraws"
llvm-profdata merge $profraws -o collected.profdata

test_exes=$(find . -type f -iname "*test" | sed -e 's/^/--object /' | tr "\n" ' ' | cut -f'2-' -d' ')
test_count=$(echo "$test_exes" |tr ' ' "\n"| wc -l)
echo "Collected $test_count executables:"
echo "$test_exes" | tr ' ' "\n"
llvm-cov export --format=lcov --instr-profile collected.profdata $test_exes >lcov.info

curl https://keybase.io/codecovsecurity/pgp_keys.asc | gpg --no-default-keyring --import
curl -Os https://uploader.codecov.io/latest/linux/codecov
curl -Os https://uploader.codecov.io/latest/linux/codecov.SHA256SUM
curl -Os https://uploader.codecov.io/latest/linux/codecov.SHA256SUM.sig
gpg --verify codecov.SHA256SUM.sig codecov.SHA256SUM
shasum -a 256 -c codecov.SHA256SUM

chmod +x codecov
./codecov
