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

namespace DiscordEuroScope_Configuration
{
	class MessageFormatter
	{
	public:
		/*
		Description: Format the message with a given array vector where the message contains the matching tokens as $i
		Paramters:
			std::string& message: a reference to the message string object
			std::vector<std::string>& Dictionary: a reference to the dictionary vector object that enlists the elements to replace
		*/
		static void formatvector(std::string& message, const std::vector<std::string>& Dictionary);
		/*
		Description: Format the message with a given mapping where the message contains the matching tokens.
		Example message: Controlling {callsign} on {frequency}MHz
		Example map: { std::pair("callsign", "HECA_APP") , std::pair("frequency", "119.050") }
		Paramters:
			std::string& message: a reference to the message string object
			std::map<std::string, std::string>& Dictionary: a reference to the dictionary map object that enlists the elements to replace
		*/
		static void formatmap(std::string& message, const std::map<std::string, std::string>& Dictionary);
	};
}