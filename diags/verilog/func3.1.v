module main;

wire [1:0] a = return_two( 1'b0 );

initial begin
        $dumpfile( "func3.1.vcd" );
        $dumpvars( 0, main );
        #100;
        $finish;
end

function [1:0] return_two;
input a;
begin
  @(posedge a);
  return_two = 2'b10;
end

endfunction

endmodule
