/*
 Name:        race5.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/25/2008
 Purpose:     Verifies that embedded racecheck pragmas can be renamed by the user.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [3:0] a, b;
reg       clk;
integer   i;

// rcc off
always @(posedge clk) begin
  for( i=0; i<4; i=i+1 )
    a[i] <= ~b[i];
end
// rcc on

initial begin
	b   = 4'b0000;
	clk = 1'b0;
	#5;
	clk = 1'b1;
	#5;
	b   = 4'b1001;
	clk = 1'b0;
	#5;
	clk = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "race5.2.vcd" );
        $dumpvars( 0, main );
`endif
        #50;
        $finish;
end

endmodule
