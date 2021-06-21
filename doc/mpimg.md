# NAME
mpimg - mpd albumart client

# SYNOPSIS
**trx** \[options\] \[file|baudrate|timeout|count\] \[cmd1\] ...

# DESCRIPTION
Mpimg is a command-line utility to download albumartwork from a remote or local mpd server using the builtin tcp protocol\
Files named cover.jpg/png available in the same folder as the selected audio file is available.\
Embedded artwork is not yet supported.\
By default, the cover for the currently playing track is downloaded or select a specific tag by supplying the --song option.
Mpd server settings can be read from environment variables MPD\_HOST and MPD\_PORT or parameter options --host and --port.\
The image can be written to file by setting the --output \<file\>] option or environment variable MPD\_ALBUMART

# OPTIONS

**-H**, **\--HOST** **\<host-address\>**
: set the mpd server ip address or hostname

**-p**, **\--PORT** **\<port\>**
: set the mpd server tcp port - likely 6600

**-o**, **\--output** **\<filename\>**
: write response to file, use '-' if you want to write to stdout

**-v**, **\--verbose**
: shows feedback and prints options set by arguments or environment vaiables

**-h**, **\--help**
: print help menu

**-s**, **\--song** **\<uri\>**
: fetch coverart for this song instead of currently playing song

# COMMANDS

**idle**
: wait for mpd to change player status before fetching cover

**idleloop**
: wait for mpd to change player status before fetching cover and start over, loops forever

# EXAMPLES
**todo**
: tooodooo

# EXIT VALUES
**0**
: Succes

**1**
: Fail, invalid options, error is printed to stdout

# BUGS
yes

# COPYRIGHT
Copyright © 2021 Arno Lievens. License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.<br>
This is free software: you are free to change and redistribute it.  There  is  NO
WARRANTY, to the extent permitted by law.
