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
#include "ESEHandler.h"
#include "ConfigData.h"
#include <filesystem>
#include <fstream>

std::vector<std::string> ESEHandler::path_to_ese;
std::vector<ESEPositionItem> ESEHandler::positions;

bool ESEHandler::LocateESEFile(std::string relative_path_to_ese)
{
	path_to_ese.clear();
	for (auto& file : std::filesystem::directory_iterator(relative_path_to_ese))
	{
		if (file.is_regular_file() && stricmp(file.path().filename().extension().string().c_str(), ".ese") == 0)
		{
			path_to_ese.push_back(file.path().string());
		}
	}
	return path_to_ese.size() > 0;
}

int ESEHandler::ParsePositions(void)
{
	if (path_to_ese.size() > 0)
	{
		for (auto& file : path_to_ese)
		{
			bool start_pos = false;
			std::ifstream fs(file);
			std::string line;
			while (std::getline(fs, line))
			{
				if (line == "[POSITIONS]")
				{
					start_pos = true;
					continue;
				}
				if (!start_pos) continue;

				if (!(line[0] >= 'A' && line[0] <= 'Z'))
				{
					if (line[0] == '[') break;
					continue;
				}

				ESEPositionItem pos;
				int idx1 = line.find_first_of(':');
				pos.callsign = line.substr(0, idx1);
				int idx2 = line.find_first_of(':', idx1 + 1);
				pos.radio_callsign = line.substr(idx1 + 1, idx2 - idx1 - 1);
				idx1 = line.find_first_of(':', idx2 + 1);
				pos.frequency = line.substr(idx2 + 1, idx1 - idx2 - 1);
				positions.push_back(pos);
			}
			fs.close();
		}
	}
	return positions.size();
}

void ESEHandler::GetRadioCallsigns(RadioCallsigns_t& rcs)
{
	using namespace DiscordEuroScope_Configuration;
	rcs.clear();
	for (auto& pos : positions)
	{
		DiscordEuroScope_Configuration::RadioCallsignElement_t element;
		element.callsign = pos.callsign;
		element.frequency = pos.frequency;
		element.icao = pos.callsign.substr(0, pos.callsign.find_first_of('_'));
		element.radio_callsign = pos.radio_callsign;
		bool flag = false;
		for (auto& it : rcs)
		{
			if (it.callsign == element.callsign /* TEMP DISABLE || (it.icao == element.icao && it.frequency == element.frequency)*/)
			{
				flag = true;
				break;
			}
		}
		if(!flag)
			rcs.push_back(element);
	}
}
