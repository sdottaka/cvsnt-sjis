# $Id: commit_prep.pl,v 1.4.2.1 2003/07/02 13:51:48 tmh Exp $
# -*-Perl-*-
#
#
# Perl filter to handle pre-commit checking of files.  This program
# records the last directory where commits will be taking place for
# use by the log_accum.pl script.  For new files, it forces the
# existence of a RCS "Id" keyword in the first ten lines of the file.
# For existing files, it checks version number in the "Id" line to
# prevent losing changes because an old version of a file was copied
# into the direcory.
#
# Possible future enhancements:
#
#    Check for cruft left by unresolved conflicts.  Search for
#    "^<<<<<<<$", "^-------$", and "^>>>>>>>$".
#
#    Look for a copyright and automagically update it to the
#    current year.  [[ bad idea!  -- woods ]]
#
#
# Contributed by David Hampton <hampton@cisco.com>
#
# Hacked on lots by Greg A. Woods <woods@web.net>

#
#	Configurable options
#

# Constants (remember to protect strings from RCS keyword substitution)
#
$LAST_FILE     = "d:/temp/#cvs.lastdir"; # must match name in log_accum.pl
$ENTRIES       = "CVS/Entries";

#
#	Subroutines
#

sub write_line {
    local($filename, $line) = @_;
    open(FILE, ">$filename") || die("Cannot open $filename, stopped");
    print(FILE $line, "\n");
    close(FILE);
}

#
#	Main Body	
#

# Record the directory for later use by the log_accumulate stript.
#
$record_directory = 0;

# parse command line arguments
#
while (@ARGV) {
    $arg = shift @ARGV;

    if ($arg eq '-d') {
	$debug = 1;
	print STDERR "Debug turned on...\n";
    } elsif ($arg eq '-r') {
	$record_directory = 1;
    } elsif ($arg eq '-i') {
	$id = shift @ARGV;
    } else {
	push(@files, $arg);
    }
}

($id) || die "Must supply parent ID\n";

$directory = shift @files;

# For new-style commitinfo, read stdin for list of files
while (<STDIN>) {
  chop;			# Drop the newline
  push(@files, $_);
}


if ($debug != 0) {
    print STDERR "dir   - ", $directory, "\n";
    print STDERR "files - ", join(":", @files), "\n";
    print STDERR "id    - ", $id, "\n";
}

# Suck in the CVS/Entries file
#
open(ENTRIES, $ENTRIES) || die("Cannot open $ENTRIES.\n");
while (<ENTRIES>) {
    local($filename, $version) = split('/', substr($_, 1));
    $cvsversion{$filename} = $version;
}


# Record this directory as the last one checked.  This will be used
# by the log_accumulate script to determine when it is processing
# the final directory of a multi-directory commit.
#
if ($record_directory != 0) {
    &write_line("$LAST_FILE.$id", $directory);
}
exit(0);
