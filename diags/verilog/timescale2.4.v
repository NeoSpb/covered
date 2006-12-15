/*
 Name:     timescale2.4.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     11/25/2006
 Purpose:  Verifies multiple timescale specifications within the design when
           top module timescale is 1 s / 1 us.
*/

`timescale 1 s / 1 us

module main;

ts_module tsm();

initial begin
`ifndef VPI
        $dumpfile( "timescale2.4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule