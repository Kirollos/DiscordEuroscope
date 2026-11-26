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
#include "config.h"
#include "ConfigManager.h"
#include "MessageFormatter.h"
#include "ESEHandler.h"
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <regex>
#include <stdexcept>

const char default_file_content[] = DEFAULT_FILE_CONTENT;

// caching so we don't regex every time
static std::string last_fetch_callsign = "";
static std::string last_returned_rcallsign = "";
static std::string last_fetch_frequency = "";

namespace
{
	std::string JoinPath(const std::string& context, const char* key)
	{
		return context.empty() ? std::string(key) : context + "." + key;
	}

	const rapidjson::Value& RequireObject(const rapidjson::Value& parent, const char* key, const std::string& context)
	{
		if (!parent.IsObject() || !parent.HasMember(key) || !parent[key].IsObject())
			throw std::runtime_error("Config error: " + JoinPath(context, key) + " must be an object.");
		return parent[key];
	}

	std::string RequireString(const rapidjson::Value& parent, const char* key, const std::string& context)
	{
		if (!parent.IsObject() || !parent.HasMember(key) || !parent[key].IsString())
			throw std::runtime_error("Config error: " + JoinPath(context, key) + " must be a string.");
		return std::string(parent[key].GetString());
	}

	bool RequireBool(const rapidjson::Value& parent, const char* key, const std::string& context)
	{
		if (!parent.IsObject() || !parent.HasMember(key) || !parent[key].IsBool())
			throw std::runtime_error("Config error: " + JoinPath(context, key) + " must be true or false.");
		return parent[key].GetBool();
	}

	void FillButtons(VATPHIL_Configuration::Button(&buttons)[2], const rapidjson::Value& object)
	{
		if (!object.IsObject() || !object.HasMember("buttons") || !object["buttons"].IsArray())
			return;

		const auto& arr = object["buttons"].GetArray();
		rapidjson::SizeType len = arr.Size();
		if (len > 2)
			len = 2;
		for (rapidjson::SizeType i = 0; i < len; i++)
		{
			if (!arr[i].IsObject() || !arr[i].HasMember("label") || !arr[i]["label"].IsString())
				continue;
			if (!arr[i].HasMember("url") || !arr[i]["url"].IsString())
				continue;
			buttons[i] = VATPHIL_Configuration::Button(arr[i]["label"].GetString(), arr[i]["url"].GetString());
		}
	}

	void FillState(VATPHIL_Configuration::State& state, const rapidjson::Value& object, const std::string& context)
	{
		state.used = true;
		state.presence_small_image_key = RequireString(object, "presence_small_image_key", context);
		state.presence_large_image_key = RequireString(object, "presence_large_image_key", context);
		state.state = RequireString(object, "state", context);
		state.details = RequireString(object, "details", context);
		state.presence_small_image_text = RequireString(object, "presence_small_image_text", context);
		state.presence_large_image_text = RequireString(object, "presence_large_image_text", context);
		FillButtons(state.buttons, object);
	}
}

namespace VATPHIL_Configuration
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
			throw std::runtime_error("Failed to open JSON file.");
		}
		std::stringstream file_sstream;
		file_sstream << file_stream.rdbuf();

		const std::string file_contents = file_sstream.str();
		if (json_document.Parse(file_contents.c_str()).HasParseError())
		{
			auto error = json_document.GetParseError();
			throw std::runtime_error("Failed to parse the JSON file. Error code: " + std::to_string(error));
		}
		if (!json_document.IsObject())
		{
			throw std::runtime_error("Config error: the JSON root must be an object.");
		}

		data.discord_appid = RequireString(json_document, "discord_appid", "");
		data.discord_presence_large_image_key = RequireString(json_document, "discord_presence_large_image_key", "");
		data.discord_presence_small_image_key = RequireString(json_document, "discord_presence_small_image_key", "");
		data.sweatbox_bypass = RequireBool(json_document, "sweatbox_bypass", "");

		// The webhook URL itself is baked into the DLL; only the line format is configurable.
		data.webhook.line_format = WEBHOOK_DEFAULT_LINE_FORMAT;
		if (json_document.HasMember("webhook") && json_document["webhook"].IsObject())
		{
			const rapidjson::Value& webhook = json_document["webhook"];
			if (webhook.HasMember("line_format"))
			{
				if (!webhook["line_format"].IsString())
					throw std::runtime_error("Config error: webhook.line_format must be a string.");
				data.webhook.line_format = webhook["line_format"].GetString();
			}
			if (data.webhook.line_format.empty())
				data.webhook.line_format = WEBHOOK_DEFAULT_LINE_FORMAT;
		}

		FillButtons(data.buttons, json_document);

		const rapidjson::Value& states = RequireObject(json_document, "states", "");
		const char* state_names[4] = { "idle", "playback", "sweatbox", "direct" };
		for (int i = 0; i < 4; i++)
		{
			const rapidjson::Value& object = RequireObject(states, state_names[i], "states");
			FillState(data.states[i], object, std::string("states.") + state_names[i]);
		}

		const rapidjson::Value& direct = states["direct"];
		struct NestedState
		{
			int index;
			const char* key;
		};
		const NestedState nested_states[3] = {
			{ 4, "if_observer" },
			{ 5, "if_controller" },
			{ 6, "if_supervisor" }
		};
		for (int i = 0; i < 3; i++)
		{
			if (!direct.HasMember(nested_states[i].key) || !direct[nested_states[i].key].IsObject())
				continue;
			FillState(
				data.states[nested_states[i].index],
				direct[nested_states[i].key],
				std::string("states.direct.") + nested_states[i].key
			);
		}

		_ready = true;
	}

	void ConfigManager::Cleanup(void)
	{
		_ready = false;
		json_document.GetAllocator().Clear();
		data.Cleanup();
		last_fetch_callsign = "";
		last_returned_rcallsign = "";
		last_fetch_frequency = "";
	}

	void ConfigManager::LoadRadioCallsigns()
	{
		if (!json_document.HasMember("radio_callsigns") || !json_document["radio_callsigns"].IsObject())
			return;

		auto& radio_callsigns = json_document["radio_callsigns"];
		if (radio_callsigns.HasMember("config") && radio_callsigns["config"].IsObject())
		{
			auto& radio_config = radio_callsigns["config"];
			if (radio_config.HasMember("load_from_ese") && radio_config["load_from_ese"].IsBool() && radio_config["load_from_ese"].GetBool())
			{
				std::string relative_path = ".\\";
				if (radio_config.HasMember("path_to_ese") && radio_config["path_to_ese"].IsString())
					relative_path = radio_config["path_to_ese"].GetString();

				std::string relative_to_absolute_path;
				if (relative_path.size() > 1 && relative_path[1] == ':')
					relative_to_absolute_path = relative_path;
				else
					relative_to_absolute_path = this->file_path.substr(0, this->file_path.find_last_of('\\') + 1) + relative_path;

				try
				{
					if (ESEHandler::LocateESEFile(relative_to_absolute_path))
					{
						if (ESEHandler::ParsePositions() > 0)
						{
							data.loaded_from_ese = true;
							ESEHandler::GetRadioCallsigns(data.RadioCallsigns);
							return;
						}
					}
				}
				catch (...)
				{
					// Fall through to the custom callsigns below.
				}
			}
		}

		data.RadioCallsigns.clear();
		if (!radio_callsigns.HasMember("custom_callsigns") || !radio_callsigns["custom_callsigns"].IsObject())
			return;

		auto obj = radio_callsigns["custom_callsigns"].GetObject();
		for (auto& it : obj)
		{
			if (!it.name.IsString() || !it.value.IsString())
				continue;
			RadioCallsignElement_t element;
			element.callsign = it.name.GetString();
			element.radio_callsign = it.value.GetString();
			element.icao = element.callsign.substr(0, element.callsign.find_first_of('_'));
			data.RadioCallsigns.push_back(element);
		}
	}

	void ConfigManager::FindRadioCallsign(std::string callsign, std::string frequency, std::string& radio_callsign)
	{
		// caching so we don't regex every time
		if (callsign == last_fetch_callsign && frequency == last_fetch_frequency)
		{
			radio_callsign = last_returned_rcallsign; // return cached result
			return;
		}
		last_fetch_callsign = callsign;
		last_fetch_frequency = frequency;
		std::string icao = callsign.substr(0, callsign.find_first_of('_'));

		if (data.loaded_from_ese)
		{
			for (auto& it : data.RadioCallsigns)
			{
				if (it.icao == icao && it.frequency == frequency)
				{
					radio_callsign = it.radio_callsign;
					last_returned_rcallsign = radio_callsign;
					return;
				}
			}
			last_returned_rcallsign = radio_callsign = callsign;
			return;
		}

		for (auto& it : data.RadioCallsigns)
		{
			std::regex rgx(std::string(it.callsign));
			std::smatch rgx_match;
			if(!std::regex_search(callsign, rgx_match, rgx)) continue;
			if (!it.frequency.empty() && !(it.frequency == frequency && it.icao == icao)) continue;
			radio_callsign = it.radio_callsign;

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
