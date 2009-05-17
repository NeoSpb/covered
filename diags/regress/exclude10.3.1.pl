# Name:     exclude10.3.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/15/2008
# Purpose:  Runs the exclude command to exclude a multi-expression for coverage, providing a reason for
#           the exclusion

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude10.3.1", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP exclude10.3.1.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP exclude10.3.1.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP exclude10.3.1.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP exclude10.3.1.v" ) && die;
}

# Perform diagnostic running code here
&runScoreCommand( "-t main -vcd exclude10.3.1.vcd -v exclude10.3.1.v -o exclude10.3.1.cdd" );

# Create temporary file that will contain an exclusion message
&runCommand( "echo This logic is not needed > exclude10.3.1.excl" );

# Perform exclusion
&runExcludeCommand( "-m E05 exclude10.3.1.cdd < exclude10.3.1.excl" );

# Remove temporary exclusion reason file
system( "rm -f exclude10.3.1.excl" ) && die;

# Generate reports
&runReportCommand( "-d v -e -x -o exclude10.3.1.rptM exclude10.3.1.cdd" );
&runReportCommand( "-d v -e -x -i -o exclude10.3.1.rptI exclude10.3.1.cdd" );

# Perform the file comparison checks
&checkTest( "exclude10.3.1", 1, 0 );

exit 0;

