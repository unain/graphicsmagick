#!/usr/local/bin/perl
#
# Test writing FPX images
#
# Contributed by Bob Friesenhahn <bfriesen@simple.dallas.tx.us>
#
BEGIN { $| = 1; $test=1; print "1..6\n"; }
END {print "not ok $test\n" unless $loaded;}

use Image::Magick;
$loaded=1;

require 't/subroutines.pl';

chdir 't/fpx' || die 'Cd failed';

#
# 1) Test Black-and-white, bit_depth=1 FPX
# 
print( "1-bit grayscale FPX ...\n" );
testReadWrite( 'input_bw.fpx', 'output_bw.fpx', q/quality=>95/,
               '00706f70fcdc65b7e1d81db2641c28fc',
	       '3b69a5385ee461dee7b6faf5c6c0b922');

#
# 2) Test monochrome image
#
++$test;
print( "8-bit grayscale FPX ...\n" );
testReadWrite( 'input_mono.fpx',
	       'output_mono.fpx', '',
	       '4812f6adde914192a77b356c41bf6b99',
	       '3a25beb452133a9b53e812ac409e44ee');
#
# 3) Test pseudocolor image
#
++$test;
print( "8-bit indexed-color FPX ...\n" );
testReadWrite( 'input_256.fpx',
	       'output_256.fpx',
	       q/quality=>54/,
	       '8778165f74f3e11619724c613ea3d8e7' );
#
# 4) Test truecolor image
#
++$test;
print( "24-bit Truecolor FPX ...\n" );
testReadWrite( 'input_truecolor.fpx',
	       'output_truecolor.fpx',
	       q/quality=>55/,
	       'eb4d6e084afe2835a1ad28ad7fc12ced' );
