/*
 Name:     timescale1.9.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     11/25/2006
 Purpose:  Verifies 1 s / 10 ms timescale setting.
*/

`timescale 1 s / 10 ms

module main;

reg a;

initial begin
	a = 1'b0;
	#10;
	a = 1'b1;
end

initial begin
`ifndef VPI
        $dumpfile( "timescale1.9.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

endmodule
