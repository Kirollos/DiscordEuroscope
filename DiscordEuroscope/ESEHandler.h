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
#include <string>
#include <vector>
#include <map>


struct ESEPositionItem
{
	std::string callsign;
	std::string radio_callsign;
	std::string frequency;
};

class ESEHandler
{
private:
	static std::vector<std::string> path_to_ese;
	static std::vector<ESEPositionItem> positions;
public:
	static bool LocateESEFile(std::string relative_path_to_ese);
	static int ParsePositions(void);
	static void GetRadioCallsigns(std::map<std::string, std::string>& rcs);
};
