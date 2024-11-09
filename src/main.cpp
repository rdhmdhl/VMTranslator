#include <stdexcept>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <iostream>
#include <string>
class Parser{
private:
	std::ifstream VMCode;
	std::vector<std::string> instructions;
	size_t currentInstructionIndex;

public:
	// VM commands
	enum CommandType {
		C_ARITHMETIC,
		C_PUSH, C_POP,
		LABEL, C_GOTO,
		C_IF, C_FUNCTION, 
		C_RETURN, C_CALL,
	};
	
	std::unordered_map <std::string, std::string> commandMap = {
		{"push", "C_PUSH"},
      		{"pop", "C_POP"},
		{"lt", "C_ARITHMETIC"},
		{"gt", "C_ARITHMETIC"},
		{"eq", "C_ARITHMETIC"},
      		{"add", "C_ARITHMETIC"},
		{"sub", "C_ARITHMETIC"},
		{"neg", "C_ARITHMETIC"},
		{"or", "C_ARITHMETIC"}, 
      		{"not", "C_ARITHMETIC"},
		{"and", "C_ARITHMETIC"},
      };
      
	// constructor -- open files, gets ready to parse it 
	Parser(const std::string& inputFilename) : currentInstructionIndex(0) {
		VMCode.open(inputFilename);
		if(!VMCode.is_open()){
			throw std::runtime_error("Could not open the input file: " + inputFilename);
		}
		loadInstructions();
	}
	
	~Parser(){
		if (VMCode.is_open()) VMCode.close();
	}

	void loadInstructions() {
		std::string line;
		while(std::getline(VMCode, line)){
			// remove trailing carriage return character (r)
			if (!line.empty() && line.back() == '\r'){
				line.pop_back();
			}

			// removing trailing whitespace
			line.erase(0, line.find_first_not_of(" \t"));
			line.erase(line.find_last_not_of(" \t") + 1);

			// remove any inline comments starting with '//'
			if (line.find("//") == 0 || line.empty()) {
				continue;
			}

			if (!line.empty()){
				instructions.push_back(line);
			}
		}
	}

	void advance(){
		if (hasMoreCommands()){
			currentInstructionIndex++;
		}
	}

	bool hasMoreCommands() {
		return currentInstructionIndex < instructions.size();
	} 

	Parser::CommandType commandType(){
		std::string instruction = getCurrentInstruction();
		if (instruction != "EOF"){
			// get first word of instruction
			std::string instructionArg1 = instruction.substr(0, instruction.find(" "));
      			auto command = commandMap.find(instructionArg1);
			
			if (command != commandMap.end()) {
				if(command->second == "C_PUSH") return C_PUSH;
				if(command->second == "C_POP") return C_POP;
				return C_ARITHMETIC;
			}
      		}
		throw std::runtime_error("Unexpected EOF or unsupported command type: " + instruction);
	}	

	std::string getCurrentInstruction(){
		if (currentInstructionIndex < instructions.size()){
			return instructions[currentInstructionIndex];
		}
		return "EOF";
	}

	std::string arg1(){
		std::string instruction = getCurrentInstruction();

		if (commandType() == C_ARITHMETIC) {
			return instruction;
		}

		size_t firstSpace = instruction.find(" ");
		size_t secondSpace = instruction.find(" ", firstSpace + 1);
		if (firstSpace != std::string::npos && secondSpace != std::string::npos){
			return instruction.substr(firstSpace + 1, secondSpace - firstSpace - 1);
		}

		throw std::runtime_error("Invalid or unsupported command type: " + instruction);
	}	

	int arg2(){
		std::string instruction = getCurrentInstruction();
		size_t firstSpace = instruction.find(" ");
		// find(" ", startPosition)
		size_t secondSpace = instruction.find(" ", firstSpace + 1);

		if (firstSpace != std::string::npos && secondSpace != std::string::npos){
			// extract the third argument as a substring
			std::string arg2str = instruction.substr(secondSpace + 1);
			// convert arg2str into an integer
			return std::stoi(arg2str);
		}
		throw std::runtime_error("arg2 called on a command without segment argument: " + instruction);
	}

	void printLine(){
		std::string instruction = getCurrentInstruction();
		std::cout << instruction << std::endl;
	}
};	


class CodeWriter{
private:
	std::ofstream outputFile;

public:
	// constructor 
	CodeWriter(const std::string&outputFilename){
		outputFile.open(outputFilename);
		
		if(!outputFile.is_open()) throw std::runtime_error("Could not open the output file: " + outputFilename);
	}

	// deconstructor
	~CodeWriter(){
		if (outputFile.is_open()) outputFile.close();
	}

	void writeArithmetic(std::string& command){
		// unique label counter
		static int labelCounter = 0;
		std::string assemblyCode;
		
		if (command == "neg") {
			assemblyCode = 
				"// " + command + "\n"
				"@SP\n"
				"A=M-1\n" // select top stack value
				"M=-M\n"; // negate stack value
		} else if (command == "add") {
			assemblyCode = 
				"// " + command + "\n"
				"@SP\n"
				"M=M-1\n" // decrement SP
				"A=M\n" // access top stack value (y) 
				"D=M\n" // store y in D register
				"A=A-1\n" // move to next value in stack 
				"M=M+D\n"; // add x + y, store in M register
		} else if (command == "sub") {
			assemblyCode = 
				"// " + command + "\n"
				"@SP\n"
				"M=M-1\n" // decrement SP
				"A=M\n" // access top stack value (y)
				"D=M\n" // store y in D register	
				"A=A-1\n" // move to next value in stack 
				"M=M-D\n"; // subtract x - y, store in M register
		} else if (command == "eq" || command == "lt" || command == "gt") {
			std::string trueLabel = "TRUE_" + std::to_string(labelCounter);
			std::string endLabel = "END_" + std::to_string(labelCounter);
			labelCounter++;

			std::string jumpCondition;
			if(command == "eq") jumpCondition = "JEQ";
			else if (command == "lt") jumpCondition = "JLT";
			else if (command == "gt") jumpCondition = "JGT";

			assemblyCode = 
				"// " + command + "\n"
				"@SP\n"
				"M=M-1\n" // decrement SP
				"A=M\n"
				"D=M\n" // store y in D register
				"A=A-1\n" // move to next value in stack (x)
				"D=M-D\n" // compute x - y
				"@" + trueLabel + "\n" 
				"D;" + jumpCondition + "\n" // jump if condition is true 
				"@SP\n"
				"A=M-1\n" // set pointer to top of stack 
				"M=0\n" // set false (0) to top of stack
				"@" + endLabel + "\n"
				"0;JMP\n"
				"(" + trueLabel + ")\n"
				"@SP\n" 
				"A=M-1\n"
				"M=-1\n" // set true (-1) to top of stack
				"(" + endLabel + ")\n";
		}
		outputFile << assemblyCode;	
	}

	void writePushPop(std::string instruction, Parser::CommandType commandType, std::string segment, int index) {
		std::string memorySegment;
		std::string assemblyCode;

		if (segment == "local") memorySegment = "LCL";
		else if (segment == "argument") memorySegment = "ARG";
		else if (segment == "this") memorySegment = "THIS";
		else if (segment == "that") memorySegment = "THAT";
		else if (segment == "pointer") memorySegment = (index == 0) ? "THIS" : "THAT";
		else if (segment == "temp") memorySegment = "R" + std::to_string(5 + index);
		else if (segment == "constant") memorySegment = "";
		else if (segment == "static") memorySegment = "STATIC_" + std::to_string(index);

		if (commandType == Parser::C_POP) {
			if (segment == "pointer" || segment == "temp" || segment == "static"){
				// pop into fixed segment
				assemblyCode = 
					"// " + instruction + "\n"
					"@SP\n"
					"M=M-1\n"
					"A=M\n"
					"D=M\n" // load value from top of stack into D register
					"@" + memorySegment + "\n"
					"M=D\n"; // load value from stack into memory segment 
			} else {
				// pop into memory segment (local, argument, this, that)
				assemblyCode = 
					"// " + instruction + "\n"
					"@" + std::to_string(index) + "\n"
					"D=A\n"
					"@" + memorySegment + "\n"
					"D=M+D\n" // compute target address (base + index)
					"@R13\n" // temp storage 
					"M=D\n" // store target address in R13
					"@SP\n"
					"M=M-1\n" // decrement SP (pop)
					"A=M\n"
					"D=M\n" // get top stack value
					"@R13\n"
					"A=M\n"
					"M=D\n"; // store value at (memorySegment + index)
			}
		} else if (commandType == Parser::C_PUSH) {
			if (segment == "constant") {
				// push constant value onto stack
				assemblyCode = 
					"// " + instruction + "\n"
					"@" + std::to_string(index) + "\n"
					"D=A\n"
					"@SP\n"
					"A=M\n" // get address of stack pointer
					"M=D\n" // load value into stack pointer address
					"@SP\n"
					"M=M+1\n";// increment stack pointer
			} else if (segment == "pointer" || segment == "temp" || segment == "static") {
				assemblyCode = 
					"// " + instruction + "\n"
					"@" + memorySegment + "\n"
					"D=M\n"
					"@SP\n"
					"A=M\n" // get address of stack pointer
					"M=D\n" // load value into stack 
					"@SP\n"
					"M=M+1\n"; // increment stack pointer 
			} else {
				// push from memory segment (local, argument, this, that)
				assemblyCode =
					"// " + instruction + "\n"
					"@" + std::to_string(index) + "\n"
					"D=A\n"
					"@" + memorySegment + "\n"
					"D=M+D\n" // compute target address (base + index)
					"A=D\n"
					"D=M\n" // load data from target address
					"@SP\n"
					"A=M\n"
					"M=D\n" // push data onto stack
					"@SP\n"
					"M=M+1\n"; // increment stack pointer
			}
		}
		outputFile << assemblyCode;	
	}
};
	
int main(){
	Parser parser("SimpleTest.vm");
	CodeWriter codeWriter("output.txt");

	while(parser.hasMoreCommands()){
		// get instruction line	from input file
		std::string instruction = parser.getCurrentInstruction();

		// get instruction type 
		Parser::CommandType commandType = parser.commandType();

		if (commandType == Parser::C_ARITHMETIC) {
			codeWriter.writeArithmetic(instruction);
		} else if (commandType == Parser::C_PUSH || commandType == Parser::C_POP){
			std::cout << "push or pull command type met.. " << std::endl;
			// find memory segment (arg1), and index (arg2)
			std::string segment = parser.arg1();
			std::cout << "segment " << segment << std::endl;
			int index = parser.arg2();
			std::cout << "index " << index << std::endl;
			codeWriter.writePushPop(instruction, commandType, segment, index);
		}
		// advance to next instruction
		parser.advance();
	}

	return 0;
} 
