module main;

reg      a;
reg      b;
reg      c;

initial begin
        $dumpfile( "simple.vcd" );
        $dumpvars( 0, main );
        a = 1'b0;
        b = ~a;
        #5;
        b = a;
	#10;
end

endmodule
