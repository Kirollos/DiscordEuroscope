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

#include "stdafx.h"
#include "ConfigManager.h"
#include "MessageFormatter.h"
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <regex>

const char default_file_content[] = DEFAULT_FILE_CONTENT;

namespace DiscordEuroScope_Configuration
{
	ConfigManager::ConfigManager()
	{
	}
	
	void ConfigManager::Init(std::string filepath)
	{
		file_path = filepath;
		this->Init();
	}

	void ConfigManager::Init(void)
	{
		std::ifstream file_stream(file_path, std::ios_base::in);
		if (file_stream.fail())
		{
			throw std::exception("Failed to open JSON file.");
		}
		std::stringstream file_sstream;
		file_sstream << file_stream.rdbuf();

		if (json_document.Parse(static_cast<const char*>(file_sstream.str().c_str())).HasParseError())
		{
			auto error = json_document.GetParseError();
			std::string error_str = "Failed to parse the JSON file. Error code: ";
			error_str += std::to_string(error);
			throw std::exception(error_str.c_str());
		}

		assert(json_document.IsObject());

		assert(json_document["discord_appid"].IsString());
		data.discord_appid = json_document["discord_appid"].GetString();
		assert(json_document["discord_presence_large_image_key"].IsString());
		data.discord_presence_large_image_key = json_document["discord_presence_large_image_key"].GetString();
		assert(json_document["discord_presence_small_image_key"].IsString());
		data.discord_presence_small_image_key = json_document["discord_presence_small_image_key"].GetString();
		assert(json_document["sweatbox_bypass"].IsBool());
		data.sweatbox_bypass = json_document["sweatbox_bypass"].GetBool();
		
		assert(json_document["states"].IsObject());
		assert(json_document["states"]["idle"].IsObject());
		assert(json_document["states"]["direct"].IsObject());
		assert(json_document["states"]["playback"].IsObject());
		assert(json_document["states"]["sweatbox"].IsObject());

		std::vector<rapidjson::GenericObject<false, rapidjson::Value>> objects;

		objects.push_back(json_document["states"]["idle"].GetObject());
		objects.push_back(json_document["states"]["playback"].GetObject());
		objects.push_back(json_document["states"]["sweatbox"].GetObject());
		objects.push_back(json_document["states"]["direct"].GetObject());


		for (int i = 0; i < 4; i++)
		{
			data.states[i].used = true;
			data.states[i].presence_small_image_key = objects[i]["presence_small_image_key"].GetString();
			data.states[i].presence_large_image_key = objects[i]["presence_large_image_key"].GetString();
			data.states[i].state = objects[i]["state"].GetString();
			data.states[i].details = objects[i]["details"].GetString();
			data.states[i].presence_small_image_text = objects[i]["presence_small_image_text"].GetString();
			data.states[i].presence_large_image_text = objects[i]["presence_large_image_text"].GetString();
		}

		for (int i = 4; i < 7; i++)
		{
			rapidjson::GenericObject<false, rapidjson::Value>* objj;
			if (i == 4 && json_document["states"]["direct"]["if_observer"].IsObject())
			{
				objj = &json_document["states"]["direct"]["if_observer"].GetObject();
			}
			else if (i == 5 && json_document["states"]["direct"]["if_controller"].IsObject())
			{
				objj = &json_document["states"]["direct"]["if_controller"].GetObject();
			}
			else if (i == 6 && json_document["states"]["direct"]["if_supervisor"].IsObject())
			{
				objj = &json_document["states"]["direct"]["if_supervisor"].GetObject();
			}
			else
			{
				continue;
			}
			auto& obj = *objj;
			data.states[i].used = true;
			data.states[i].presence_small_image_key = obj["presence_small_image_key"].GetString();
			data.states[i].presence_large_image_key = obj["presence_large_image_key"].GetString();
			data.states[i].state = obj["state"].GetString();
			data.states[i].details = obj["details"].GetString();
			data.states[i].presence_small_image_text = obj["presence_small_image_text"].GetString();
			data.states[i].presence_large_image_text = obj["presence_large_image_text"].GetString();
		}
		_ready = true;
	}

	void ConfigManager::Cleanup(void)
	{
		_ready = false;
		json_document.GetAllocator().Clear();
		data.Cleanup();
	}

	void ConfigManager::LoadRadioCallsigns()
	{
		assert(json_document["radio_callsigns"].IsObject());
		data.RadioCallsigns.clear();
		for (auto& it : json_document["radio_callsigns"].GetObject())
		{
			if (it.name.GetType() == rapidjson::kStringType && it.value.GetType() == rapidjson::kStringType)
			{
				data.RadioCallsigns.insert(std::pair<std::string, std::string>(it.name.GetString(), it.value.GetString()));
			}
		}
	}

	void ConfigManager::FindRadioCallsign(std::string callsign, std::string& radio_callsign)
	{
		// caching so we don't regex every time
		static std::string last_fetch_callsign = "";
		static std::string last_returned_rcallsign = "";
		if (callsign == last_fetch_callsign)
		{
			radio_callsign = last_returned_rcallsign; // return cached result
			return;
		}
		last_fetch_callsign = callsign;
		for (auto& it : data.RadioCallsigns)
		{
			std::regex rgx(std::string(it.first));
			std::smatch rgx_match;
			if(!std::regex_search(callsign, rgx_match, rgx)) continue;
			radio_callsign = it.second;

			std::vector<std::string> dict;
			for (int rgx_it = 1; rgx_it < rgx_match.size(); rgx_it++)
				dict.push_back((rgx_match[rgx_it]).str());
			MessageFormatter::formatvector(radio_callsign, dict);
			last_returned_rcallsign = radio_callsign;
			return;
		}
		last_returned_rcallsign = radio_callsign = callsign;
		return;
	}

	void ConfigManager::GenerateConfigFile(std::string filepath)
	{
		std::ofstream o_fs(filepath);
		o_fs << default_file_content;
		o_fs.close();
	}

	ConfigManager::~ConfigManager()
	{
		this->Cleanup();
	}

}