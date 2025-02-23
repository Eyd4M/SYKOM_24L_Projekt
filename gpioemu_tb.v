`timescale 1 ns/10 ps
module gpioemu_tb;

	reg	n_reset = 1; //reset
	reg	[15:0] saddress = 0;
	reg	[31:0] sdata_out = 0;
	reg	[31:0] sdata_in = 0;
	reg	[31:0] gpio_in = 0;
	reg [31:0] gpio_out = 0;
	reg gpio_latch = 0;
	reg clk = 0;
	reg	srd = 0;
	reg	swr = 0;


initial begin
	$dumpfile("GpioEmu.vcd");
	$dumpvars(0, gpioemu_tb);
end

//Zegar
initial begin
	forever begin
		# 10 clk = ~clk;
	end
end

initial begin

	//RESET
	# 5 n_reset = 0;
	# 5 n_reset = 1;

	//---- TESTY ----
		//Test zapisu wartości A do GPIO
		//Test stosunkowo małej wartości - 0x18 (24 dziesiętnie)
		# 5 sdata_in = 32'h18;
		# 5 saddress = 16'hD4;
		
		# 5 swr = 1;
		# 5 swr = 0;
		
		# 250;
		
		//Sprawdzenie aktualnego stanu automatu (S) - oczekuję 0xcc (FIND_PRIMES)
		# 5 saddress = 16'hEC;
		# 5 srd = 1;
		# 5 srd = 0;
		
		//Sprawdzenie wyliczonej wartości (W) - oczekuję 0, bo jeszcze automat nie doliczył do 24 liczby
		# 5 saddress = 16'hE4;
		# 5 srd = 1;
		# 5 srd = 0;
		
		# 1600;
		
		//Ponowne odczytanie wyliczonej wartości (W) - tutaj oczekuję wyniku 0x59 (89 decymalnie - 24 liczba pierwsza)
		# 5 srd = 1;
		# 5 srd = 0;
		
		#100;
		
		//Wpisanie do rejestru A 1000nej wartości podczas liczenia poprzedniej (0x3E8 heksadecymalnie)
		# 5 sdata_in = 32'h3E8;
		# 5 saddress = 16'hD4;
		# 5 swr = 1;
		# 5 swr = 0;
		
		# 400
 
	//Oczekiwanie na wyliczenie wartości
	# 160000;
	
		//Sprawdzenie wyliczonej wartości dla 1000nej liczby pierwszej - oczekuję wyniku 0x1EEF (7919 decymalnie)
		# 5 saddress = 16'hE4;
		# 5 srd = 1;
		# 5 srd = 0;
		
		# 100;
		
		//Test wpisania kolejnej wartości (0xF - 15 dziesiętnie)
		# 5 sdata_in = 32'hF;
		# 5 saddress = 16'hD4;
		# 5 swr = 1;
		# 5 swr = 0;
		
		//Test wpisania kolejnej wartości w trakcie liczenia (0x8 - 8 dziesiętnie)
		# 5 sdata_in = 32'h8;
		# 5 saddress = 16'hD4;
		# 5 swr = 1;
		# 5 swr = 0;
		
		#2500
		
	//TESTY PODTRZYMANIA
		//Test wpisania wartości do nieistniejącego adresu 0xaa
		# 5 sdata_in = 32'h25;
		# 5 saddress = 16'haa;
		# 5 swr = 1;
		# 5 swr = 0;

		#100

	//TESTY NEUTRALNOŚCI
		//Próba odczytu z nieistniejącego adresu
		# 5 saddress = 16'hbb;
		# 5 srd = 1;
		# 5 srd = 0;
		
#1000 $finish;
end

wire [31:0] gpio_out_test;
wire [31:0] sdata_out_test;
wire [31:0] gpio_in_s_insp_test;

gpioemu gpio_simulation(n_reset, saddress, srd, swr, sdata_in, sdata_out_test, gpio_in, gpio_latch, gpio_out_test, clk, gpio_in_s_insp_test);

endmodule
