#include "stdafx.h"
#include "ESEHandler.h"
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

void ESEHandler::GetRadioCallsigns(std::map<std::string, std::string>& rcs)
{
	rcs.clear();
	for (auto& pos : positions)
	{
		rcs.insert(std::pair<std::string, std::string>(pos.callsign, pos.radio_callsign));
	}
}
