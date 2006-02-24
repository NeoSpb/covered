module main;

wire [1:0] a = return_two( 1'b0 );

initial begin
        $dumpfile( "func3.3.vcd" );
        $dumpvars( 0, main );
        #100;
        $finish;
end

function [1:0] return_two;
input a;
begin
  @(a);
  return_two = 2'b10;
end

endfunction

endmodule

/* HEADER
GROUPS func3.3 all iv vcd lxt
SIM    func3.3 all iv vcd  : iverilog func3.3.v; ./a.out                             : func3.3.vcd
SIM    func3.3 all iv lxt  : iverilog func3.3.v; ./a.out -lxt2; mv func3.3.vcd func3.3.lxt : func3.3.lxt
SCORE  func3.3.vcd     : -t main -vcd func3.3.vcd -o func3.3.cdd -v func3.3.v >& func3.3.err : func3.3.err : 1
SCORE  func3.3.lxt     : -t main -lxt func3.3.lxt -o func3.3.cdd -v func3.3.v >& func3.3.err : func3.3.err : 1
*/

/* OUTPUT func3.3.err
ERROR!  Attempting to use a delay, task call, non-blocking assign or event controls in function main.return_two, file func3.3.v, line 15
*/
