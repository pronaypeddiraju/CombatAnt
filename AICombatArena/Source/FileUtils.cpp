//------------------------------------------------------------------------------------------------------------------------------
#include "FileUtils.hpp"
#include "ErrorWarningAssert.hpp"
#include <fstream>

typedef unsigned int uint;

//------------------------------------------------------------------------------------------------------------------------------
unsigned long CreateFileReadBuffer(const std::string& fileName, char **outData )
{
	std::ifstream fileStream;

	//Open the file in binary and at the end
	//This tells us where we are in the file and. We open in binary so it doesn't fuck with carriage returns
	//We open at end of file so it's easier to get the file size
	fileStream.open(fileName, std::ios::binary | std::ios::ate);

	if(fileStream.is_open())
	{
		//The file is open

		//tellg is an int telling us how far into the file we are. Since we are in the end we can assume this returns the file size
		unsigned long fileSize = (unsigned long)fileStream.tellg();
		//Get the cursor back to the start of the file so we read the whole thing as opposed to reading nothing lol
		fileStream.seekg(std::ios::beg);

		*outData = new char[fileSize];
		fileStream.read(*outData, fileSize);

		fileStream.close();
		return fileSize;
	}
	else
	{
		//We didn't open the file. Maybe the file doesn't exist
		//Don't assert the user because we could be opening a save file and we may not need to warn the user
		
		//Note: Remove this later when you don't need the assert anymore
		GUARANTEE_RECOVERABLE(true, "The file did not exist when creating File Buffer");
		return 0;
	}

}

//------------------------------------------------------------------------------------------------------------------------------
unsigned long CreateTextFileReadBuffer(const std::string& fileName, char **outData)
{
	std::ifstream fileStream;

	//We open at end of file so it's easier to get the file size
	fileStream.open(fileName, std::ios::ate);

	if (fileStream.is_open())
	{
		//The file is open

		//tellg is an int telling us how far into the file we are. Since we are in the end we can assume this returns the file size
		unsigned long fileSize = (unsigned long)fileStream.tellg();
		//Get the cursor back to the start of the file so we read the whole thing as opposed to reading nothing lol
		fileStream.seekg(std::ios::beg);

		*outData = new char[fileSize];
		fileStream.read(*outData, fileSize);

		fileStream.close();
		return fileSize;
	}
	else
	{
		//We didn't open the file. Maybe the file doesn't exist
		//Don't assert the user because we could be opening a save file and we may not need to warn the user

		//Note: Remove this later when you don't need the assert anymore
		GUARANTEE_RECOVERABLE(true, "The file did not exist when creating File Buffer");
		return 0;
	}

}

//------------------------------------------------------------------------------------------------------------------------------
std::ofstream* CreateFileWriteBuffer(const std::string& fileName)
{
	std::ofstream* fileStream = new std::ofstream();

	//Open the file in binary and at the end
	//This tells us where we are in the file and. We open in binary so it doesn't fuck with carriage returns
	//We open at end of file so it's easier to get the file size
	fileStream->open(fileName, std::ios::binary | std::ios::ate);

	if (!fileStream->is_open())
	{
		//The file didn't exist so lets make one
		delete fileStream;
		fileStream = new std::ofstream(fileName, std::ofstream::out, std::ofstream::trunc);
	}

	return fileStream;
}

//------------------------------------------------------------------------------------------------------------------------------
std::ofstream* CreateTextFileWriteBuffer(const std::string& fileName)
{
	std::ofstream* fileStream = new std::ofstream();

	//We open at end of file so it's easier to get the file size
	fileStream->open(fileName, std::ios::ate);

	if (!fileStream->is_open())
	{
		//The file didn't exist so lets make one
		delete fileStream;
		fileStream = new std::ofstream(fileName, std::ofstream::out, std::ofstream::trunc);
	}

	return fileStream;
}

//------------------------------------------------------------------------------------------------------------------------------
std::string GetDirectoryFromFilePath(const std::string& filePath)
{
	//Iterate from the back and see if you can find a "/"
	for (uint i = (uint)filePath.size(); i > 0; --i)
	{
		if (filePath[i] == '/')
		{
			return std::string(filePath.begin(), filePath.begin() + i);
		}
	}
	if (filePath[0] == '/')
	{
		return "/";
	}
	else
	{
		return ".";
	}
}