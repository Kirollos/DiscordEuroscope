/*
	Copyright(C) 2023-2024 Kirollos Nashaat

	This program is free software : you can redistribute it and /or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.If not, see < https://www.gnu.org/licenses/>.
*/

#pragma once
#ifndef _CONFIGMANAGER_H_
#define _CONFIGMANAGER_H_

#include "config.h"
#include <string>
#include <vector>
#include <map>
#include "ConfigData.h"
#include "./inc/rapidjson/rapidjson.h"
#include "./inc/rapidjson/document.h"
#include "./inc/rapidjson/ostreamwrapper.h"
#include "./inc/rapidjson/writer.h"

namespace DiscordEuroScope_Configuration
{
	class ConfigManager
	{
	private:
		rapidjson::Document json_document;
		ConfigData data;
		std::string file_path;
		bool _ready = false;
	public:
		inline ConfigData Data() const
		{
			return data;
		};

		inline bool isReady()
		{
			return _ready;
		};

		ConfigManager();
		//ConfigManager(std::string filepath);
		~ConfigManager();
		void Init(void);
		void Init(std::string filepath);
		void Cleanup(void);
		void LoadRadioCallsigns(void);
		void GenerateConfigFile(std::string filepath);
		void FindRadioCallsign(std::string callsign, std::string& radio_callsign);
	};
}

#endif