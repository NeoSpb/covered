/*
 Name:        lshift4.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/29/2008
 Purpose:     Verifies left-shift operations on values larger than 32.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [79:0] a;

initial begin
	a = 80'b1xz01xz01xz01xz01xz01xz01xz01xz0_1xz01xz01xz01xz01xz01xz01xz01xz0;
	#5;
	a = a << 17;
end

initial begin
`ifdef DUMP
        $dumpfile( "lshift4.2.vcd" );
        $dumpvars( 0, main );
`endif
        #400;
        $finish;
end

endmodule
