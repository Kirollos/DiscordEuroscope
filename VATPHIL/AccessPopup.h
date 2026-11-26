#pragma once
#ifndef _ACCESSPOPUP_H_
#define _ACCESSPOPUP_H_

#include <string>
#include <vector>
#include <functional>

// The "you may not control this position" screen: a black window over the whole screen EuroScope is
// on, always on top. It has no close button and ignores Alt+F4. It goes when the check passes
// (Try again) or, once connected, the controller disconnects; Close EuroScope is the way out, since EuroScope itself
// is covered.
class AccessPopup
{
public:
	~AccessPopup();

	// Shows the screen (or updates it when it's already up). Heading and lines are UTF-8.
	void Show(const std::string& heading, const std::vector<std::string>& lines);
	// While a check runs the Try again button is disabled and says so.
	void SetChecking(bool checking);
	void Close();
	bool IsOpen() const { return hwnd != nullptr; }
	// Keeps it covering EuroScope's screen and on top (called every second).
	void KeepInFront();

	std::function<void()> onTryAgain;
	std::function<void()> onCloseEuroScope;

	// EuroScope's main window: the biggest visible top-level window of this process.
	static HWND EuroScopeWindow();

private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static bool RegisterClassOnce();
	void Layout();
	void CoverScreen();
	void DrawButton(const DRAWITEMSTRUCT* item);

	HWND hwnd = nullptr;
	HWND brand = nullptr;
	HWND heading = nullptr;
	HWND text = nullptr;
	HWND tryAgain = nullptr;
	HWND closeEs = nullptr;
	HFONT brandFont = nullptr;
	HFONT headingFont = nullptr;
	HFONT textFont = nullptr;
	HFONT buttonFont = nullptr;
	HBRUSH black = nullptr;
	int lineCount = 1;
	bool checking = false;
};

#endif
