/* verilator lint_off UNUSED */
/* verilator lint_off UNDRIVEN */
/* verilator lint_off MULTIDRIVEN */
/* verilator lint_off BLKSEQ */ 
/* verilator lint_off COMBDLY */

module gpioemu(n_reset, saddress[15:0], srd, swr, sdata_in[31:0], sdata_out[31:0], gpio_in[31:0], gpio_latch, gpio_out[31:0], clk, gpio_in_s_insp[31:0]);
    
    input         n_reset;
    input  [15:0] saddress;
    input         srd;
    input         swr;
    input  [31:0] sdata_in;
    output [31:0] sdata_out;
    input  [31:0] gpio_in;
    input         gpio_latch;
    output [31:0] gpio_out;
    reg    [31:0] gpio_in_s;
    reg    [31:0] gpio_out_s;
    reg    [31:0] sdata_out_s;
    output [31:0] gpio_in_s_insp;
    input         clk;
	
	//Lokalne rejestry A,S,W z polecenia
    reg	[31:0] A;
	reg	[31:0] S;
    reg	[31:0] W;
	
	
	//Parametry ułatwiające ewentualne zmiany w kodzie

	localparam MAX_PRIME = 7920; //1000-na liczba pierwsza to 7919

	//Adresy rejestrów A,S,W
	localparam ADDRESS_A = 16'hD4;
	localparam ADDRESS_S = 16'hEC;
	localparam ADDRESS_W = 16'hE4;
	
	//Lokalne rejestry wykorzystywane w algorytmie
	reg [31:0] sito [0:MAX_PRIME]; // tablica na liczby pierwsze (sito Erastotenesa)
	reg [31:0] current_prime; //obecnie liczona liczba pierwsza
    
	reg [31:0] i, j;
    reg [31:0] n; 
	
    reg [31:0] system_A;  // rejestr na wartość A
	
	//stany automatu
	localparam IDLE         = 32'haa,
			   INIT         = 32'hbb,
			   FIND_PRIMES  = 32'hcc,
			   DONE         = 32'hdd;

	always @(posedge gpio_latch)
    begin
        gpio_in_s <= gpio_in;
    end
	
	//Zapis do GPIO
	always@(posedge swr) 
	begin
		if (saddress == ADDRESS_A) begin
			A <= sdata_in;
		end
	end
	
	//Odczyt z GPIO
	always@(posedge srd) 
	begin
		if (saddress == ADDRESS_W) begin
			sdata_out_s <= W; //Wyliczona liczba pierwsza
		end
		//odczyt stanu automatu
		else if (saddress == ADDRESS_S) begin
			sdata_out_s <= S; //Aktualny stan automatu
		end
		else begin
			sdata_out_s <= 32'b0;
		end
	end
	 
	//Reset (aktywowane przejściem 1->0)
	always @(posedge clk, negedge n_reset) 
	begin
		if(!n_reset) begin
			//Przypisanie bazowego stanu - IDLE
			S             <= IDLE;
			
			//Zerowanie rejestrów
			gpio_in_s     <= 0;
			gpio_out_s    <= 0;
			sdata_out_s	  <= 0;
			current_prime <= 0;
			W             <= 0;
			i             <= 0;
			system_A      <= 0;

		end 
		else begin
			case (S)
				IDLE: begin
					/* pojawia się nowy input - uruchamiamy system liczący (jeżeli nowa wartość będzie taka sama, 
					to algorytm nie rusza - w pliku jest już wyliczona poprzednia taka sama wartość)*/
					if (A != system_A) begin 
						/*Jeśli zadana liczba wynosi 0 lub jest większa niż zadana w zadaniu projektowym (1000), 
						pozostań w stanie IDLE*/
						if(A == 0 || A > 1000) begin
							S <= IDLE;
						end
						else begin
							system_A <= A;
							S        <= INIT;
						end
					end
				end
				
				INIT: begin
					for (n = 0; n < MAX_PRIME; n = n + 1) begin
						sito[n] = 1;
					end
					//Usunięcie 0 i 1 (nie są to liczby pierwsze)
					sito[0]       <= 0;
					sito[1]       <= 0; 
					i             <= 1; 
					current_prime <= 0;
					S             <= FIND_PRIMES;
				end
				
				FIND_PRIMES: begin
					i <= i+1;
					if (i < MAX_PRIME) begin
						if (sito[i] == 1) begin
							current_prime <= current_prime + 1;
							if(current_prime + 1 == system_A) begin
								W <= i;
								S <= DONE;
							end
							//Usuwanie kolejnych wielokrotności obecnej liczby pierwszej (algorytm Erastotenesa)
							for (j = i*i; j < MAX_PRIME; j = j + i) begin
								sito[j] = 0;
							end
						end
					end
					else begin
						S <= DONE;
					end
				end
				
				DONE: begin
					gpio_out_s <= gpio_out_s + 1; //Dodanie kolejnej wyliczonej wartości na wyprowadzenie GPIO
					W <= W;
					S <= IDLE;
				end
			endcase
		end
	end

	assign gpio_in_s_insp = gpio_in_s;
	assign gpio_out = gpio_out_s;
	assign sdata_out = sdata_out_s; 
	
endmodule