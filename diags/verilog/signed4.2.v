/*
 Name:     signed4.2.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     11/06/2006
 Purpose:  
*/

module main;

wire signed [1:0] a;

initial begin
`ifndef VPI
        $dumpfile( "signed4.2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
