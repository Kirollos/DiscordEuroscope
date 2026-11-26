#pragma once
#ifndef _PORTALCHECK_H_
#define _PORTALCHECK_H_

#include <string>
#include <vector>
#include <thread>
#include <atomic>

// Asks the VATPHIL portal whether the connected controller may control their position
// (GET /api/plugin/check?callsign=...&sct=...). The request runs on a background
// thread so EuroScope never waits on the network; Poll() picks up the answer on the
// main thread.
class PortalCheck
{
public:
	enum class Status
	{
		Ok,       // nothing wrong, or the check is off
		Blocked,  // show the popup with Messages
		Waiting,  // the portal can't see them in the VATSIM feed yet; ask again shortly
		Failed    // network or portal error; never blocks anyone
	};

	struct Result
	{
		Status status = Status::Failed;
		std::vector<std::string> messages; // UTF-8, one line each
	};

	~PortalCheck();

	// Starts a check unless one is already running. Returns false when one is running.
	bool Start(const std::string& url, const std::string& callsign, const std::string& sectorFiles);
	bool Busy() const { return running; }
	// True once, when a check has finished; fills out.
	bool Poll(Result& out);

	// The names of the .sct files in the sector package, comma separated ("2612,2611"), looked up from
	// the folder the DLL is in and the folders above it. Empty when none can be found.
	static std::string FindSectorFiles();

private:
	static Result Fetch(const std::string& url);
	void Join();

	std::thread worker;
	std::atomic<bool> running{ false };
	std::atomic<bool> done{ false };
	Result result;
};

#endif
