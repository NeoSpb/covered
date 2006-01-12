module main;

foo #(10) bar();

initial begin
        $dumpfile( "localparam1.2.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule


module foo;

localparam SIZEL=5;
parameter  SIZEG=5;

wire [(SIZEL-1):0] a;
wire [(SIZEG-1):0] b;

endmodule
