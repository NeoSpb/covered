module main;

localparam off = 0;

reg [3:0] a;

generate
  if( off < 1 )
    initial begin
            a = 4'h0;
            #10;	
            a = 4'ha;
    end

endgenerate

initial begin
        $dumpfile( "generate5.vcd" );
        $dumpvars( 0, main );
        #20;
        $finish;
end

endmodule
