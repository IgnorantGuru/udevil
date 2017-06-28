#!/bin/bash


# Update version reported in manpage
version="$(grep --color=never 'AM_INIT_AUTOMAKE(udevil,' ../configure.ac)";
if [[ $version =~ ([[:digit:]\.\+]+) ]]
then
    version="${BASH_REMATCH[0]}"
else
    echo "Unable to parse udevil version out of configure.ac!" >&2
    exit 1
fi
sed -i "s/:man version: .*/:man version: $version/" udevil.1.txt

# Generate translated asciidoc files
po4a po4a/po4a.cfg

# Generate manpages from the asciidoc files - they end up in translated/<locale> directories
find translated -iname '*.txt' -exec a2x --doctype manpage --format manpage {} \;
