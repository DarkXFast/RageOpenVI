#pragma once
#include "inicpp.h"

class config
{
public:
	static ini::IniFile& get_ini()
	{
		static ini::IniFile iniFile;
		return iniFile;
	}

	static void load()
	{
		if (std::filesystem::exists("RageOpenVI.ini"))
		{
			get_ini().load("RageOpenVI.ini");
		}
		else
		{	//defaults
			get_ini()["config"]["enabled"] = true;
			get_ini()["config"]["console"] = false;
			get_ini()["log"]["info"] = false;
			get_ini()["log"]["mods"] = false;
			get_ini()["log"]["rpf"] = false;
			get_ini()["log"]["device"] = false;
			save();
		}
	}

	static void save()
	{
		get_ini().save("RageOpenVI.ini");
	}

	template<typename T>
	static T get_config(const char* key)
	{
		return get_ini()["config"][key].as<T>();
	}

	// Like get_config, but returns `def` when the key is missing (older or hand-edited ini files)
	template<typename T>
	static T get_config_or(const char* key, T def)
	{
		auto& section = get_ini()["config"];
		auto it = section.find(key);
		return it == section.end() ? def : it->second.as<T>();
	}

	static bool get_log(const char* key)
	{
		return get_ini()["log"][key].as<bool>();
	}
};