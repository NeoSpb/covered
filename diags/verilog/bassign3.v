module main;

reg [1:0] a, b;

initial begin
	a = 2'b0;
        b = 2'b0;
	{a,b} = 4'b0110;
end

initial begin
        $dumpfile( "bassign3.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
