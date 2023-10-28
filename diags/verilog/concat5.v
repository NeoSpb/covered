module main;

reg  [127:0]  a;
reg  [127:0]  b;

always @(b)
  a = {b[123:120], b[127:124],
       b[115:112], b[119:116],
       b[107:104], b[111:108],
       b[ 99: 96], b[103:100],
       b[ 91: 88], b[ 95: 92],
       b[ 83: 80], b[ 87: 84],
       b[ 75: 72], b[ 79: 76],
       b[ 67: 64], b[ 71: 68],
       b[ 59: 56], b[ 63: 60],
       b[ 51: 48], b[ 55: 52],
       b[ 43: 40], b[ 47: 44],
       b[ 35: 32], b[ 39: 36],
       b[ 27: 24], b[ 31: 28],
       b[ 19: 16], b[ 23: 20],
       b[ 11:  8], b[ 15: 12],
       b[  3:  0], b[  7:  4]};

initial begin
`ifdef DUMP
	$dumpfile( "concat5.vcd" );
	$dumpvars( 0, main );
`endif
	b = 128'h0;
	#5;
	b = 128'h1;
	#5;
	$finish;
end

endmodule
