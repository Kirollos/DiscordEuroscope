#pragma once
#ifndef _WEBHOOKSENDER_H_
#define _WEBHOOKSENDER_H_

#include <string>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>

// Posts chat lines to a Discord webhook on a background thread,
// so EuroScope never waits on the network.
class WebhookSender
{
public:
	WebhookSender();
	~WebhookSender();

	// Lines and username are UTF-8. EuroScope chat strings must be converted with AnsiToUtf8 first.
	void Enqueue(const std::string& url, const std::string& username, const std::string& line);
	// Sends one message immediately and returns the HTTP status (0 = network/URL error).
	// Only used by the explicit ".discord webhooktest" command, never by chat.
	static int TestPost(const std::string& url, const std::string& username, const std::string& content);
	// EuroScope gives chat text in the system ANSI code page.
	static std::string AnsiToUtf8(const std::string& in);
	void Stop();

private:
	struct Item
	{
		std::string url;
		std::string username;
		std::string line; // UTF-8
	};

	void Run();
	static int Post(const std::string& url, const std::string& username, const std::string& content);

	std::deque<Item> queue;
	std::mutex lock;
	std::condition_variable wake;
	bool stopping = false;
	std::thread worker;
};

#endif
