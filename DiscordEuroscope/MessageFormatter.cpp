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
#include "MessageFormatter.h"

namespace DiscordEuroScope_Configuration
{
	void MessageFormatter::formatvector(std::string& message, const std::vector<std::string>& Dictionary)
	{
		for (auto it = Dictionary.begin(); it != Dictionary.end(); it++)
		{
			int pos = std::string::npos;
			std::string pos_formatter = std::string("$") + std::to_string(it - Dictionary.begin());
			while ((pos = message.find(pos_formatter)) != std::string::npos)
			{
				message.replace(pos, pos_formatter.length(), *it); // {param}
			}
		}
	}

	void MessageFormatter::formatmap(std::string& message, const std::map<std::string, std::string>& Dictionary)
	{
		for (auto& it : Dictionary)
		{
			int pos = std::string::npos;
			std::string pos_formatter = std::string("{") + it.first + "}";
			while ((pos = message.find(pos_formatter)) != std::string::npos)
			{
				message.replace(pos, pos_formatter.length(), it.second); // {param}
			}
		}
	}
}