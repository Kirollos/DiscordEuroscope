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

#define PLUGIN_NAME					"VATPHIL"
#define PLUGIN_VERSION				("1.4.0")

#define UPDATE_TIMER				(1000) // milliseconds
#define CONFIG_FILENAME				"VATPHIL_config.json"
// Used when the config has no webhook.line_format (or it is empty).
#define WEBHOOK_DEFAULT_LINE_FORMAT	"**{sender}:** {message}"
// The webhook URL is compiled into the DLL. tools/bake_webhook.py replaces this
// placeholder in the built DLL; the public release build keeps the placeholder.
#define WEBHOOK_BAKED_URL_PLACEHOLDER "https://bake.invalid/00000000-0000-0000-0000-000000000000"
// The VATPHIL portal endpoint that says whether the connected controller may control their position.
#define PORTAL_CHECK_URL			"https://portal.vatphil.com/api/plugin/check"
// No longer used by any logic in the code. Only used to be dumped in default .json config file.
#define DISCORD_APPID				"1563163358042259537"
#define PRESENCE_LARGE_IMAGE_KEY	"vatphil_banner"
#define PRESENCE_SMALL_IMAGE_KEY	"vatphil_logo"


#include "DefaultFileContent.h"