#pragma once
#include "config.h"

#define DEFAULT_FILE_CONTENT  ("\
{\r\
    \"discord_appid\": \""DISCORD_APPID"\",\r\
    \"discord_presence_large_image_key\" : \""PRESENCE_LARGE_IMAGE_KEY"\",\r\
    \"discord_presence_small_image_key\" : \""PRESENCE_SMALL_IMAGE_KEY"\",\r\
    \"sweatbox_bypass\" : false,\r\
    \"states\" : {\r\
        \"idle\": {\r\
            \"presence_small_image_key\": \"\",\r\
            \"presence_large_image_key\" : \"\",\r\
            \"state\" : \"\",\r\
            \"details\" : \"Idle\",\r\
            \"presence_small_image_text\" : \"\",\r\
            \"presence_large_image_text\" : \"\"\r\
        },\r\
        \"playback\": {\r\
            \"presence_small_image_key\": \"\",\r\
            \"presence_large_image_key\" : \"\",\r\
            \"state\" : \"\",\r\
            \"details\" : \"Playback\",\r\
            \"presence_small_image_text\" : \"\",\r\
            \"presence_large_image_text\" : \"\"\r\
        },\r\
        \"sweatbox\": {\r\
            \"presence_small_image_key\": \"\",\r\
            \"presence_large_image_key\" : \"\",\r\
            \"state\" : \"sweating someone\",\r\
            \"details\" : \"Sweatbox\",\r\
            \"presence_small_image_text\" : \"\",\r\
            \"presence_large_image_text\" : \"\"\r\
        },\r\
        \"direct\": {\r\
            \"presence_small_image_key\": \"\",\r\
            \"presence_large_image_key\" : \"\",\r\
            \"state\" : \"\",\r\
            \"details\" : \"\",\r\
            \"presence_small_image_text\" : \"\",\r\
            \"presence_large_image_text\" : \"\",\r\
            \r\
            \"if_observer\" : {\r\
                \"presence_small_image_key\": \"\",\r\
                \"presence_large_image_key\" : \"\",\r\
                \"state\" : \"Aircraft in range: {total_inrange}\",\r\
                \"details\" : \"Observing as {callsign}\",\r\
                \"presence_small_image_text\" : \"\",\r\
                \"presence_large_image_text\" : \"\"\r\
            },\r\
            \r\
            \"if_controller\": {\r\
                \"presence_small_image_key\": \"\",\r\
                \"presence_large_image_key\" : \"\",\r\
                \"state\" : \"Controlling ({current_tracked} of {total_inrange})\",\r\
                \"details\" : \"{rcallsign} {frequency}MHz\",\r\
                \"presence_small_image_text\" : \"Total tracks: {total_tracked}\",\r\
                \"presence_large_image_text\" : \"{callsign}\",\r\
                \"buttons\": [\r\
                    {\r\
                        \"label\": \"Callsign stats\",\r\
                        \"url\" : \"https://stats.vatsim.net/search/{callsign}\"\r\
                    }\r\
                ]\r\
            },\r\
            \r\
            \"if_supervisor\": {\r\
                \"presence_small_image_key\": \"\",\r\
                \"presence_large_image_key\" : \"\",\r\
                \"state\" : \"\",\r\
                \"details\" : \"\",\r\
                \"presence_small_image_text\" : \"\",\r\
                \"presence_large_image_text\" : \"\"\r\
            }\r\
        }\r\
    },\r\
    \"buttons\": [\r\
        {\r\
            \"label\": \"Feedback\",\r\
            \"url\" : \"mailto:info@vateg.net\"\r\
        },\r\
        {\r\
            \"label\": \"Download Plugin\",\r\
            \"url\" : \"https://github.com/Kirollos/DiscordEuroscope/\"\r\
        }\r\
    ],\r\
    \"radio_callsigns\": {\r\
        \"config\": {\r\
            \"load_from_ese\": false,\r\
            \"path_to_ese\" : \"..\\\\..\\\\\"\r\
        },\r\
        \"custom_callsigns\": {\r\
            \"^HECC_CTR$\": \"Cairo Control Bandbox\",\r\
            \"^HECC_(\\\\d+)(?:_)CTR$\" : \"Cairo Control ACC $0\"\r\
        }\r\
    }\r\
}\r\
")