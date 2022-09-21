#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#define BUILDER

#include "Include/nlohmann/json.hpp"
#include <iostream>
#include <experimental/filesystem>
#include <format>
#include "cryptopp/cryptlib.h"
#include "cryptopp/channels.h"
#include "cryptopp/filters.h"
#include "cryptopp/files.h"
#include "cryptopp/sha.h"
#include "cryptopp/hex.h"

namespace fs = std::experimental::filesystem;

nlohmann::json known;

nlohmann::json unknown;

//Config options for hash json generation

//Paths to check, will check all files and directories from this point
std::vector<std::string> paths = { "\\paks", "\\vpk", "\\media" , "\\audio", "\\stbsp", "\\cfg" , "\\bin", "\\materials", "\\platform\\shaders" };

//Specific files to check
std::vector<std::string> files = { "\\amd_ags_x64.dll", "\\bink2w64.dll", "\\binkawin64.dll", "\\r5apexdata.bin", "\\mileswin64.dll" };

#ifdef BUILDER
//Ignore specific files by name
std::vector<std::string> excluded_files = { "client_frontend.bsp.pak000_000.vpk", "englishclient_frontend.bsp.pak000_dir.vpk" };
#endif // 


std::string logo = R"(+-----------------------------------------------+
|   ___ ___ ___     _              _        _   |
|  | _ \ __| _ \___| |___  __ _ __| |___ __| |	|
|  |   /__ \   / -_) / _ \/ _` / _` / -_) _` |  |
|  |_|_\___/_|_\___|_\___/\__,_\__,_\___\__,_|  |
|                                               |
+-----------------------------------------------+)";


//Main hashing function
void HashFile(const fs::path &path_in, const bool& gen_hash)
{

	using namespace CryptoPP;
	
	SHA1 hash;

	std::string file_hash;
	HashFilter hash_filter(hash, new HexEncoder(new StringSink(file_hash)));

	ChannelSwitch channel_switch;
	channel_switch.AddDefaultRoute(hash_filter);
	
	
	//Try catch to stop directories breaking the FileSource, should use a better method to filter them out
	try
	{
		FileSource(path_in.c_str(), true, new Redirector(channel_switch));
	}
	catch (const std::exception&)
	{
	}

	if (file_hash != "")
	{
		std::string path_str = path_in.u8string();
		std::size_t ind = path_str.find(fs::current_path().u8string());
		path_str.erase(ind, fs::current_path().u8string().length());

		#ifdef BUILDER
		if (!gen_hash)
		{
			unknown[path_str] = file_hash;

		}
		else
		{
			known[path_str] = file_hash;
			std::cout << "Hash: " << file_hash << std::endl;
		}
		#endif

		#ifndef BUILDER

		unknown[path_str] = file_hash;

		#endif
	}

}


void main()
{
	bool bad_files{ false };
	std::cout << logo << std::endl;
	std::cout << "R5R file hash check" << std::endl;
	
	fs::path hashes_path = fs::current_path() /= "hashes.json";
	std::ifstream hashes_file_in(hashes_path, std::ios::in);

	if (hashes_file_in.good() && hashes_file_in)
	{
		hashes_file_in >> known;
		hashes_file_in.close();
	}
	
#ifndef BUILDER
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
		//Hash individual files
		for (std::string& ittr : files)
		{
			fs::path a = fs::current_path() += ittr;

			std::cout << "Hashing: " << a << std::endl;
			HashFile(a, true);
		}

		//I should think of a better way to do this but idk, this is only a builder function, no need to be fast
		bool reject_file = false;


		//Hash Directories
		for (std::string& ittr : paths)
		{

			fs::path dir = fs::current_path() += ittr;

			for (auto& file : fs::recursive_directory_iterator(dir))
			{
				//Clear file rejection status
				reject_file = false;
				
				//Set reject_file true if file is in excluded_files vector
				for (std::string& ittr : excluded_files)
				{
					if (ittr == file.path().filename())
					{
						reject_file = true;
					}
				}


				//Sanity checks on files and filter files that are in excluded_files
				if (reject_file == false && (file.path().has_filename()) && (file.path().has_extension()))
				{
						std::cout << "Hashing: " << file << std::endl;
						std::cout << "Hashing: " << file.path().filename() << std::endl;
						HashFile(file, true);	
				}
				
			}
		}

		//Write hashes.json file
		std::ofstream hashes_file(hashes_path, std::ios::out | std::ios::trunc);
		hashes_file << known.dump(4);
		hashes_file.close();

}

	else
	{
#endif

		//Check individual files
		for (std::string& ittr : files)
		{
			fs::path a = fs::current_path() += ittr;
			std::cout << "Verifying: " << a << std::endl;
			HashFile(a, false);
		}

		//Check whole directories
		for (std::string& ittr : paths)
		{
			fs::path a = fs::current_path() += ittr;
			std::cout << "Verifying: " << a << std::endl;

			for (auto& file : fs::recursive_directory_iterator(a))
			{
				if ((file.path().has_filename()) && (file.path().has_extension()))
				{
					HashFile(file, false);
				}
			}
		}

		//Check hashes vs hash file
		//unknown = hashes generated
		//known = known good hashes from file
		for (auto& ittr : known.items())
		{
			if (unknown.contains(ittr.key()))
			{
				if (unknown[ittr.key()].get<std::string>() != (ittr.value()))
				{
					bad_files = true;
					std::cout << "\nInvalid File found: " << ittr.key() << '\n' << std::endl;
				}
			}
			else
			{
				bad_files = true;
				std::cout << "\nFiles missing: " << ittr.key() << '\n' << std::endl;
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

