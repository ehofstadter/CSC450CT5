#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <limits>


namespace fs = std::filesystem;


//Establish security constants to prevent resource exhaustion
constexpr std::size_t MAX_INPUT_LENGTH = 1024;
constexpr std::uintmax_t MAX_FILE_SIZE = 10*1024*1024;


//Validation chain to ensure file path is safe for use
void validateFilePath(const fs::path& filePath, bool mustExist){

	if (fs::exists(filePath)){

		if(fs::is_symlink(filePath)){
			throw std::runtime_error("SEC ERROR: Symlinks not permitted! (" +filePath.string() + ").");
		}

		if (!fs::is_regular_file(filePath)){
			throw std::runtime_error("SEC ERROR: Target is not a regular file (" + filePath.string() + ").");
		}

		if (fs::file_size(filePath) > MAX_FILE_SIZE){
			throw std::runtime_error("SEC ERROR: File size exceeds maximum limit! (" + filePath.string()+ ")");
		}

	} else if(mustExist){
		throw std::runtime_error("FILE ERROR: Input file does not exist (" + filePath.string()+ ").");
	}
}

//writeToFile method takes user input and appends it to the target file, without deleting existing content
void writeToFile(const fs::path& filePath, const std::string& input){
	validateFilePath(filePath, false);

	std::ofstream outFile;
	outFile.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	try{
		outFile.open(filePath, std::ios::out | std::ios::app);
		outFile << input << '\n';
		outFile.flush();
		outFile.close();
	} catch(const std::ios_base::failure& e){
		throw std::runtime_error("I/O error while writing to '" + filePath.string() + "': " + e.what());
	}
}

//reverseFile method reverses all characters in the source file and writes them to a new destination file (in this case CSC450-mod5-reverse.txt)
void reverseFile(const fs::path& sourceFile, const fs::path& destFile){
	validateFilePath(sourceFile, true);
	validateFilePath(destFile, false);

	//badbit will throw an error on hardware or read corruption
	std::ifstream inFile;
	inFile.exceptions(std::ifstream::badbit);

	std::string content;

	try{
		//Opening the file in binary mode so line endings (\r, \n) are not silently translated or corrupted
		inFile.open(sourceFile, std::ios::in | std::ios::binary);
		if(!inFile.is_open()){
			throw std::runtime_error("Source file could not be read: " + sourceFile.string());
		}

		//allocating memory pre-emptively based on the file size avoids repeated reallocations
		const auto fileSize = fs::file_size(sourceFile);
		content.resize(static_cast<std::size_t>(fileSize));

		inFile.read(content.data(), static_cast<std::streamsize>(fileSize));
		//Adjusting after the fact to match the exact bytes read
		content.resize(static_cast<std::size_t>(inFile.gcount()));
		inFile.close();

	} catch(const std::bad_alloc&){
		throw std::runtime_error("MEM ERROR: Insufficient memory to read file!");

	} catch(const std::ios_base::failure& e) {
		throw std::runtime_error("I/O error while reading '" + sourceFile.string() + "': " + e.what());

	}

	//Reverse all of the characters in place
	std::reverse(content.begin(), content.end());

	//Write the reversed content to our target file
	std::ofstream outFile;
	outFile.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	try{
		outFile.open(destFile, std::ios::out | std::ios::trunc | std::ios::binary);
		outFile.write(content.data(), static_cast<std::streamsize>(content.size()));
		outFile.flush();
		outFile.close();
	} catch(const std::ios_base::failure& e) {
		throw std::runtime_error("I/O error while writing to '" + destFile.string() + "': " + e.what());
	}
}

int main() {
	const fs::path targetFile = "CSC450_CT5_mod5.txt";
	const fs::path reversedFile = "CSC450-mod5-reverse.txt";

	try{
		std::cout << "Enter the text you would like to add to " << targetFile.string()
				<< " (maximum of " << MAX_INPUT_LENGTH << " characters): ";

		std::string input;
		if (!std::getline(std::cin, input)){
			if (std::cin.eof()){
				throw std::runtime_error("INPUT ERROR: end-of-file reached before input was provided.");
			}
			throw std::runtime_error("INPUT ERROR: failed to read from standard input.");
		}

		if (input.length() > MAX_INPUT_LENGTH){
			std::cerr <<"WARNING: maximum input length exceeded. Your message was truncated for safety.";
			input.resize(MAX_INPUT_LENGTH);
		}

		writeToFile(targetFile, input);
		std::cout << "Your input has been added to " << targetFile.string() << ".\n";

		reverseFile(targetFile, reversedFile);
		std::cout<< "Contents of " <<targetFile.string() << " have been reversed and saved as " <<reversedFile.string() << ".\n";
	} catch(const std::exception& e) {
		std::cerr << "\n[CRITICAL FAILURE] " << e.what() << std::endl;
		return 1;
	} catch(...){
		std::cerr <<"\n[CRITICAL FAILURE] An unknown exception has occurred." << std::endl;
		return 1;
	}

	return 0;
}
