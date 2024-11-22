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
#ifndef _CONFIGDATA_H_
#define _CONFIGDATA_H_

#include <string>
#include <vector>
#include <map>

struct DiscordButton; // discord_rpc.h

namespace DiscordEuroScope_Configuration
{
	typedef struct
	{
		std::string callsign;
		std::string icao;
		std::string frequency;
		std::string radio_callsign;
	} RadioCallsignElement_t;
	typedef std::vector<RadioCallsignElement_t> RadioCallsigns_t;

	struct Button
	{
		std::string label;
		std::string url;

		Button()
		{
		}

		Button(std::string label, std::string url) : label{label}, url{url}
		{
		}
		
		inline bool IsValid() const
		{
			return !label.empty() && !url.empty();
		}

		void FillStruct(DiscordButton* refButton) const;
	};

	enum States_Enum
	{
		State_Idle,
		State_Playback,
		State_Sweatbox,
		State_Direct,
		State_Direct_OBS,
		State_Direct_CON,
		State_Direct_SUP,
	};
	struct State
	{
		bool used = false;
		std::string presence_small_image_key;
		std::string presence_large_image_key;
		std::string state;
		std::string details;
		std::string presence_small_image_text;
		std::string presence_large_image_text;
		Button buttons[2];
	};

	class ConfigData
	{
	public:
		std::string discord_appid;
		std::string discord_presence_large_image_key;
		std::string discord_presence_small_image_key;
		bool sweatbox_bypass;
		Button buttons[2];
		State states[7];
		RadioCallsigns_t RadioCallsigns;
		bool loaded_from_ese = false;

		ConfigData();

		void Cleanup();

		inline static const std::string& LocalOrGlobal(const std::string& local, const std::string& global)
		{
			return local.empty() ? global : local;
		}
	};
}

#endif