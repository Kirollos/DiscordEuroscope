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
#include "inc/discord_rpc.h"
#include "ConfigData.h"

namespace DiscordEuroScope_Configuration
{
	ConfigData::ConfigData()
	{
		this->Cleanup();
	}

	void ConfigData::Cleanup()
	{
		this->discord_appid = "";
		this->discord_presence_large_image_key = "";
		this->discord_presence_small_image_key = "";
		this->sweatbox_bypass = false;
		this->loaded_from_ese = false;
		this->buttons[0] = Button();
		this->buttons[1] = Button();
		this->RadioCallsigns.clear();
		for (int i = 0; i < 7; i++)
		{
			this->states[i].details = "";
			this->states[i].presence_large_image_key = "";
			this->states[i].presence_large_image_text = "";
			this->states[i].presence_small_image_key = "";
			this->states[i].presence_small_image_text = "";
			this->states[i].buttons[0] = Button();
			this->states[i].buttons[1] = Button();
			this->states[i].used = false;
		}
	}

	void Button::FillStruct(DiscordButton* refButton) const
	{
		refButton->label = label.c_str();
		refButton->url = url.c_str();
	}
}