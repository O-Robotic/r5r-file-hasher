#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#define _CRT_SECURE_NO_WARNINGS
#define BUILDER
#include "Include/nlohmann/json.hpp"
#include <iostream>
#include <experimental/filesystem>
#include <format>
#include <fstream>
#include "Sha1.h"


namespace fs = std::experimental::filesystem;

nlohmann::json known;

nlohmann::json unknown;
bool shouldAddSDK = false;

//Config options for hash json generation

//Paths to check, will check all files and directories from this point
const char* paths[]{ "\\paks", "\\vpk", "\\media" , "\\audio", "\\stbsp", "\\cfg" , "\\bin", "\\materials", "\\platform\\shaders" };

const char* excluded_files[]{ "r5r-file-hasher.exe", "build.txt", "gameinfo.txt", "gameversion.txt", "hashes.json" };

const char* logo = R"(+-----------------------------------------------+
|   ___ ___ ___     _              _        _   |
|  | _ \ __| _ \___| |___  __ _ __| |___ __| |	|
|  |   /__ \   / -_) / _ \/ _` / _` / -_) _` |  |
|  |_|_\___/_|_\___|_\___/\__,_\__,_\___\__,_|  |
|                                               |
+-----------------------------------------------+)";
const int ReadSize = 1048576;

//Main hashing function
void HashFile(const fs::path& path_in, const bool gen_hash)
{

	CSha1* sha = new CSha1();
	Sha1_Init(sha);

	unsigned char* buf = (unsigned char*)malloc(ReadSize);

	FILE* file = fopen(path_in.u8string().c_str(), "rb");

	size_t filePos = 0;

	bool didHash = false;

	if (file)
	{	
		while (filePos = fread(buf, 1, ReadSize, file))
			{
				Sha1_Update(sha, buf, filePos);
			}
		didHash = true;
		fclose(file);
	}
	else
	{
		std::cout << "Failed to open: " << path_in.u8string().c_str() << std::endl;
	}
	
	unsigned char result[20];
	Sha1_Final(sha, result);

	std::stringstream shastr;
	shastr << std::hex << std::setfill('0');
	for (const auto& byte : result)
	{
		shastr << std::setw(2) << (int)byte;
	}

	free(buf);
	delete sha;

	std::string file_hash = shastr.str();

	if (didHash)
	{
		std::string path_str = path_in.u8string();
		std::size_t ind = path_str.find(fs::current_path().u8string());
		if (shouldAddSDK)
		{
			path_str.erase(ind, (fs::current_path() += "\\SDK").u8string().length());
		}
		else
		{
			path_str.erase(ind, fs::current_path().u8string().length());
		}
#ifdef BUILDER
		if (!gen_hash)
		{
			unknown[path_str] = file_hash;
		}
		else
		{

			if (shouldAddSDK)
			{
				if (known[path_str].is_object())
				{
					known[path_str] += {"Default", file_hash };
				}
				else
				{
					known[path_str] = { {"SDK", file_hash }};
				}
			}
			else if (known[path_str].is_object())
			{
				known[path_str] += {"Default", file_hash};
			}
			else
			{
				known[path_str] = file_hash;
			}

			std::cout << "Hashed: " << path_str << "\nHash: " << file_hash << "\n" << std::endl;
		}
#endif

#ifndef BUILDER
		unknown[path_str] = file_hash;
#endif
	}
}


void main()
{

	Sha1Prepare();

	bool bad_files{ false };
	std::cout << logo << std::endl;
	std::cout << "R5R file hash check" << std::endl;
	
	fs::path hashes_path = fs::current_path() /= "hashes.json";

#ifndef BUILDER
	std::ifstream hashes_file_in(hashes_path, std::ios::in);

	if (hashes_file_in.good() && hashes_file_in)
	{
		hashes_file_in >> known;
		hashes_file_in.close();
	}
	else
	{
		std::cout << "\nFailed to read hashes.json, make sure you put it and this exe in the folder with r5apex" << std::endl;
		system("pause");
		return;
	}
#endif // BUILDER

	
	
		
#ifdef BUILDER
		
	//Add select menu if builder is defined
	
	int i;

	std::cout << "\nPress 1 to generate hashes from current directory, Press 2 to verify files in current directory: ";

	std::cin >> i;
	if (i == 1)
	{
		const fs::path sdkPath = fs::current_path() += "\\SDK";

		if (fs::exists(sdkPath))
		{
			shouldAddSDK = true;

			std::cout << "Hashing SDK files" << std::endl;

			for (const char* ittr : paths)
			{

				fs::path dir = sdkPath;
				dir += ittr;

				
				for (auto& file : fs::recursive_directory_iterator(dir))
				{
					if (std::find(std::begin(excluded_files), std::end(excluded_files), file.path().filename().u8string()) != std::end(excluded_files))
					{
						continue;
					}

					if (file.path().has_filename() && file.path().has_extension())
					{
						HashFile(file, true);
					}
				}
			}

			for (auto& file : fs::directory_iterator(sdkPath))
			{
				//Check if file is in excluded files list
				if (std::find(std::begin(excluded_files), std::end(excluded_files), file.path().filename().u8string()) != std::end(excluded_files))
				{
					continue;
				}

				if (file.path().has_filename() && file.path().has_extension())
				{
					HashFile(file, true);
				}
			}
		}

		shouldAddSDK = false;

		//Hash files in base dir
		for (auto& file : fs::directory_iterator(fs::current_path()))
		{
			if (std::find(std::begin(excluded_files), std::end(excluded_files), file.path().filename().u8string()) != std::end(excluded_files))
			{
				continue;
			}

			if ((file.path().has_filename()) && (file.path().has_extension()))
			{
				HashFile(file, true);
			}
		}

		//Hash Directories
		for (const char* ittr : paths)
		{

			fs::path dir = fs::current_path() += ittr;

			for (auto& file : fs::recursive_directory_iterator(dir))
			{
				if (std::find(std::begin(excluded_files), std::end(excluded_files), file.path().filename().u8string()) != std::end(excluded_files))
				{
					continue;
				}

				if ((file.path().has_filename()) && (file.path().has_extension()))
				{
					HashFile(file, true);
				}
			}
		}

		//Write hashes.json file
		std::ofstream hashes_file(hashes_path, std::ios::out | std::ios::trunc);
		hashes_file << known.dump(1);
		hashes_file.close();

	}
	else
	{
		std::ifstream hashes_file_in(hashes_path, std::ios::in);

		if (hashes_file_in.good() && hashes_file_in)
		{
			hashes_file_in >> known;
			hashes_file_in.close();
		}
#endif
		//If user has sdk installed use different set of hashes for sdk modified files
		bool bHasSDK = false;

		//Check files in the base directory and check if user has the sdk installed
		for (auto& file : fs::directory_iterator(fs::current_path()))
		{
			if (file.path().has_filename() && file.path().has_extension())
			{
				if (file.path().filename().u8string() == "gamesdk.dll")
				{
					bHasSDK = true;
				}
				HashFile(file, false);
			}
		}

		//Check whole directories
		for (const char* ittr : paths)
		{
			fs::path a = fs::current_path() += ittr;
			std::cout << "Verifying: " << a << std::endl;

			for (auto & file : fs::recursive_directory_iterator(a))
			{
				if (file.path().has_filename() && file.path().has_extension())
				{
					HashFile(file, false);
				}
			}
		}

		std::cout << std::endl;

		//Check hashes vs hash file
		//unknown = hashes generated
		//known = known good hashes from file
		for (auto& ittr : known.items())
		{

			//If ittr is an object we should expect it may or may not exist depending on if the user has the sdk
			if (ittr.value().is_object())
			{
				if (bHasSDK) //Do we have the sdk installed
				{
					
					//If we have the sdk installed every file from the json should exist
					if (!unknown.contains(ittr.key()))
					{
						bad_files = true;
						std::cout << "File missing: " << ittr.key() << std::endl;
						continue;
					}
					
					//If we do then check the value against the "SDK" hash
					if (unknown[ittr.key()] != ittr.value()["SDK"])
					{
						bad_files = true;
						std::cout << "Invalid File found: " << ittr.key() << std::endl;
					}
				}
				else
				{

					//Does the current known hash object have a value for "Default"
					if (ittr.value().contains("Default"))
					{
						//If it does have a "Default" value then we should have the file no matter what
						if (!unknown.contains(ittr.key()))
						{
							bad_files = true;
							std::cout << "File missing: " << ittr.key() << std::endl;
							continue;
						}

						//If the object has a "Default" hash and the file exists then check the value against the "Default" hash
						if (unknown[ittr.key()] != ittr.value()["Default"])
						{
							bad_files = true;
							std::cout << "Invalid File found: " << ittr.key() << std::endl;
						}

					}
					else if (!ittr.value().contains("Default")) //If we have no default value this file only exists in the sdk and we should skip it since we dont have the sdk installed
					{
						continue;
					}
				}
			}
			else //If ittr is not an object this file should exist no matter if user has the sdk
			{
				if (unknown.contains(ittr.key())) //Do we have the ittr file
				{
					if (unknown[ittr.key()] != ittr.value()) //Since the file exists check that its valid
					{
						bad_files = true;
						std::cout << "Invalid File found: " << ittr.key() << std::endl;
					}
				}
				else
				{
					bad_files = true;
					std::cout << "File missing: " << ittr.key() << std::endl;
				}
			}
		}
		
		//Message
		if (!bad_files)
		{
			std::cout << "\nFile Integrity check complete, No damaged/missing files found :)\n" << std::endl;
		}
		else
		{
			std::cout << "\nFile Integrity check failed, damaged/missing files found :(\n" << std::endl;
		}

#ifdef BUILDER
	}
#endif // BUILDER
	system("pause");
}

