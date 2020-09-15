#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_instruction();
	CURRENT_STATE = NEXT_STATE;
	INSTRUCTION_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			runAll(); 
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* decode and execute instruction                                                                     */ 
/************************************************************/
void handle_instruction()
{
	/*IMPLEMENT THIS*/
	/* execute one instruction at a time. Use/update CURRENT_STATE and and NEXT_STATE, as necessary.*/
	/*
	CODE OUTLINE		
	1. Load instruction using mem_read_32 function	
	2. Increment instruction to separate op, rs, rt, rd, shamt, funct, address, immediate	
	3. Use switch-case function with opcode to determine which instruction we have	
	4. Perform operations to correctly execute instruction	
	5. Update Current/Next State	
	*/
	uint32_t addr;
	addr = mem_read_32(CURRENT_STATE.PC);	//Gets instruction value from current state of PC
	uint8_t op, rs, rt, rd, shamt, funct;
	op = addr >> 26;	//opcode is located in bits 26-31 so increment to bit 26 of instruction
	
	if(op == 0)//R TYPE
	{
		rs = addr >> 21;	//rs is located in bits 21-25
		rs = rs & 0b00011111;	//Bit mask 3 leftmost bits
		rt = addr >> 16;	//rt is located in bits 16-20
		rt = rt & 0b00011111;	//Bit mask 3 leftmost bits
		rd = addr >> 11;	//rd is located in bits 11-15
		rd = rd & 0b00011111;	//Bit mask 3 leftmost bits
		shamt = addr >> 6;	//shamt is located in bits 6-10
		shamt = shamt & 0b00011111;	//Bit mask 3 leftmost bits
		funct = addr;		//function code is located in bits 0-5
		funct = funct & 0b00111111;	//Bit mask 2 leftmost bits
		
		switch(funct) 
		{
			case 0b100000 : //ADD
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];

			case 0b100001 : //ADDU
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
			
			case 0b100100 : //AND
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] & CURRENT_STATE.REGS[rt];
					
			case 0b011010 : //DIV
				
			case 0b011011 : //DIVU
				
			case 0b001001 : //JALR
				uint8_t temp = CURRENT_STATE.REGS[rs];
				*CURRENT_STATE.REGS[rd] = (CURRENT_STATE.PC + 8);
				//figure out how to do it after T+1
				NEXT_STATE.PC = temp;
			case 0b001000 : //JR
				uint8_t temp = CURRENT_STATE.REGS[rs];
				//after T=1??
				CURRENT_STATE.PC = temp;
			case 0b010000 : //MFHI
				CURRENT_STATE.REGS[rd] = CURRENT_STATE.HI;
			case 0b010010 : // MFLO
				CURRENT_STATE.REGS[rd] = CURRENT_STATE.LO;
			case 0b010001 : //MTHI
				//figure out T-2 and T-1
				CURRENT_STATE.HI = CURRENT_STATE.REGS[rs];
			case 0b010011 : //MTLO
				//^^
				CURRENT_STATE.LO = CURRENT_STATE.REGS[rs];
			case 0b011000 : //MULT
				
			case 0b011001 : //MULTU
				
			case 0b100111 : //NOR
				NEXT_STATE.REGS[rd] = ~(CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt]);
			case 0b100101 : //OR
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt];
			case 0b000000 : //SLL
				//ask about operation
			case 0b101010 : //SLT
				if (CURRENT_STATE.REGS[rs] < CURRENT_STATE.REGS[rt])
				{}
				else {}
			case 0b000011 : //SRA
				
			case 0b000010 : //SRL
				
			case 0b100010 : //SUB
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
			case 0b100011 : //SUBU
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
			case 0b001100 : //SYSCALL
				CURRENT_STATE.REGS[2] = 0xA;
			case 0b100110 : //XOR
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] ^ CURRENT_STATE.REGS[rt];
			
			default : break;	
		}
		
	}
	
	else if(op == 2 || op == 3)//J TYPE
	{
		if(op == 2)//J
		{
			
		}
		else if(op == 3)//JAL
		{
			
		}
	}
	
	else //I TYPE
	{
		switch(op)
		{
			
			//ADDI
			//ADDIU
			//ANDI
			//BEQ
			//BGEZ
			//BGTZ
			//BLEZ
			//BLTZ
			//BNE
			//LB
			//LH
			//LUI
			//LW
			//ORI
			//SB
			//SH
			//SLTI
			//SW
			//XORI
				
		}
	}
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){
	int i;
	uint32_t addr;
	
	for(i=0; i<PROGRAM_SIZE; i++){
		addr = MEM_TEXT_BEGIN + (i*4);
		printf("[0x%x]\t", addr);
		print_instruction(addr);
	}
}

/************************************************************/
/* Print the instruction at given memory address (in MIPS assembly format)    */
/************************************************************/
void print_instruction(uint32_t addr){
	/*IMPLEMENT THIS*/
	/*
	1. Same as HandleInstruction
	2. Same as HandleInstruction
	3. Same as HandleInstruction
	4. Instead of performing operation, simply print the instructions inputs/outputs
	*/
	uint32_t addr;
	addr = mem_read_32(CURRENT_STATE.PC);	//Gets instruction value from current state of PC
	uint8_t op, rs, rt, rd, shamt, funct;
	op = addr >> 26;	//opcode is located in bits 26-31 so increment to bit 26 of instruction
	
	if(op == 0)//R TYPE
	{
		rs = addr >> 21;	//rs is located in bits 21-25
		rs = rs & 0b00011111;	//Bit mask 3 leftmost bits
		rt = addr >> 16;	//rt is located in bits 16-20
		rt = rt & 0b00011111;	//Bit mask 3 leftmost bits
		rd = addr >> 11;	//rd is located in bits 11-15
		rd = rd & 0b00011111;	//Bit mask 3 leftmost bits
		shamt = addr >> 6;	//shamt is located in bits 6-10
		shamt = shamt & 0b00011111;	//Bit mask 3 leftmost bits
		funct = addr;		//function code is located in bits 0-5
		funct = funct & 0b00111111;	//Bit mask 2 leftmost bits
		
		switch(funct)//print out each function by op code
		{
			case 0b100000 : //ADD
				printf("\nADD $%d, $%d, $%d", rd, rs, rt);
			case 0b100001 : //ADDU	
				printf("\nADDU $%d, $%d, $%d", rd, rs, rt);
			case 0b100100 : //AND
				printf("\nAND $%d, $%d, $%d", rd, rs, rt);
			case 0b011010 : //DIV
				printf("\nDIV $%d, $%d", rs, rt);
			case 0b011011 : //DIVU
				printf("\nDIV $%d, $%d", rs, rt);
			case 0b001001 : //JALR
				printf("\nJALR $%d, rs);
				printf("\nJALR $%d, $%d", rd, rs);
			case 0b001000 : //JR
				printf("\nJR $%d", rs);
			case 0b010000 : //MFHI
				printf("\nMFHI $%d", rd);
			case 0b010010 : // MFLO
				printf("\nMFLO $%d", rd);
			case 0b010001 : //MTHI
				printf("\nMTHI $%d", rs);
			case 0b010011 : //MTLO
				printf("\nMTLO $%d", rs);
			case 0b011000 : //MULT
				printf("\nMULT $%d, $%d", rs, rt);
			case 0b011001 : //MULTU
				printf("\nMULTU $%d, $%d", rs, rt);
			case 0b100111 : //NOR
				printf("\nNOR $%d, $%d, $%d", rd, rs, rt);
			case 0b100101 : //OR
				printf("\nOR $%d, $%d, $%d", rd, rs, rt);
			case 0b000000 : //SLL
				printf("\nSLL $%d, $%d, $%d", rd, rt, shamt);
			case 0b101010 : //SLT
				printf("\nSLT $%d, $%d, $%d", rd, rs, rt);
			case 0b000011 : //SRA
				printf("\nSRA $%d, $%d, $%d", rd, rt, shamt);
			case 0b000010 : //SRL
				printf("\nSRL $%d, $%d, $%d", rd, rt, shamt);
			case 0b100010 : //SUB
				printf("\nSUB $%d, $%d, $%d", rd, rs, rt);
			case 0b100011 : //SUBU
				printf("\nSUBU $%d, $%d, $%d", rd, rs, rt);
			case 0b001100 : //SYSCALL
				printf("\nSYSCALL");
			case 0b100110 : //XOR
				printf("\nXOR $%d, $%d, $%d", rd, rs, rt);
			default : break;	
		}
		
	}
	
	else if(op == 2 || op == 3)//J TYPE
	{
		if(op == 2)//J
		{
			
		}
		else if(op == 3)//JAL
		{
			
		}
	}
	
	else //I TYPE
	{
		switch(op)
		{
			//ADDI
			//ADDIU
			//ANDI
			//BEQ
			//BGEZ
			//BGTZ
			//BLEZ
			//BLTZ
			//BNE
			//LB
			//LH
			//LUI
			//LW
			//ORI
			//SB
			//SH
			//SLTI
			//SW
			//XORI
		}
	}
	
	
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
