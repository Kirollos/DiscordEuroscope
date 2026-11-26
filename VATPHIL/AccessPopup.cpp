#include "stdafx.h"
#include "AccessPopup.h"

static const wchar_t* const POPUP_CLASS = L"VATPHILAccessPopup";
static const int ID_TRY_AGAIN = 1001;
static const int ID_CLOSE_ES = 1002;
static const int CONTENT_WIDTH = 900;
static const int BRAND_HEIGHT = 24;
static const int HEADING_HEIGHT = 56;
static const int LINE_HEIGHT = 34;
static const int BUTTON_WIDTH = 190;
static const int BUTTON_HEIGHT = 46;
static const int BUTTON_GAP = 16;

static const COLORREF WHITE = RGB(255, 255, 255);
static const COLORREF GREY = RGB(150, 150, 150);
static const COLORREF DIM = RGB(90, 90, 90);
static const COLORREF RED = RGB(240, 80, 80);
static const COLORREF BLACK = RGB(0, 0, 0);

static std::wstring Wide(const std::string& in)
{
	if (in.empty())
		return std::wstring();
	int wlen = MultiByteToWideChar(CP_UTF8, 0, in.c_str(), (int)in.size(), NULL, 0);
	std::wstring out(wlen, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, in.c_str(), (int)in.size(), &out[0], wlen);
	return out;
}

static HINSTANCE ThisModule()
{
	HMODULE self = NULL;
	GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCWSTR)&ThisModule, &self);
	return (HINSTANCE)self;
}

static HFONT MakeFont(int px, int weight)
{
	return CreateFontW(-px, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
}

static BOOL CALLBACK FindMain(HWND w, LPARAM out)
{
	DWORD pid = 0;
	GetWindowThreadProcessId(w, &pid);
	if (pid != GetCurrentProcessId() || !IsWindowVisible(w) || GetWindow(w, GW_OWNER) != NULL)
		return TRUE;
	wchar_t cls[64] = { 0 };
	GetClassNameW(w, cls, 64);
	if (wcscmp(cls, POPUP_CLASS) == 0)
		return TRUE;
	HWND* best = (HWND*)out;
	RECT a, b;
	GetWindowRect(w, &a);
	if (*best == NULL)
		*best = w;
	else
	{
		GetWindowRect(*best, &b);
		if ((a.right - a.left) * (a.bottom - a.top) > (b.right - b.left) * (b.bottom - b.top))
			*best = w;
	}
	return TRUE;
}

HWND AccessPopup::EuroScopeWindow()
{
	HWND best = NULL;
	EnumWindows(FindMain, (LPARAM)&best);
	return best;
}

bool AccessPopup::RegisterClassOnce()
{
	static bool done = false;
	if (done)
		return true;
	WNDCLASSEXW wc;
	memset(&wc, 0, sizeof(wc));
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = ThisModule();
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = POPUP_CLASS;
	done = RegisterClassExW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
	return done;
}

AccessPopup::~AccessPopup()
{
	Close();
	HGDIOBJ objects[] = { brandFont, headingFont, textFont, buttonFont, black };
	for (HGDIOBJ o : objects)
		if (o != nullptr)
			DeleteObject(o);
}

void AccessPopup::Show(const std::string& headingText, const std::vector<std::string>& lines)
{
	if (!RegisterClassOnce())
		return;
	if (headingFont == nullptr)
	{
		brandFont = MakeFont(16, FW_SEMIBOLD);
		headingFont = MakeFont(40, FW_BOLD);
		textFont = MakeFont(22, FW_NORMAL);
		buttonFont = MakeFont(17, FW_SEMIBOLD);
		black = CreateSolidBrush(BLACK);
	}

	std::wstring body;
	for (size_t i = 0; i < lines.size(); i++)
	{
		if (i > 0)
			body += L"\r\n";
		body += Wide(lines[i]);
	}
	lineCount = lines.empty() ? 1 : (int)lines.size();
	std::wstring title = Wide(headingText);

	if (hwnd == nullptr)
	{
		HINSTANCE inst = ThisModule();
		// A plain popup: no title bar, so nothing to close or drag. Owned by EuroScope so it goes with it.
		hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, POPUP_CLASS, L"VATPHIL", WS_POPUP,
			0, 0, 100, 100, EuroScopeWindow(), NULL, inst, this);
		if (hwnd == nullptr)
			return;
		brand = CreateWindowExW(0, L"STATIC", L"VATPHIL", WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 0, 0, 0, hwnd, NULL, inst, NULL);
		heading = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 0, 0, 0, hwnd, NULL, inst, NULL);
		text = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 0, 0, 0, hwnd, NULL, inst, NULL);
		tryAgain = CreateWindowExW(0, L"BUTTON", L"Try again", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
			0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)ID_TRY_AGAIN, inst, NULL);
		closeEs = CreateWindowExW(0, L"BUTTON", L"Close EuroScope", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
			0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)ID_CLOSE_ES, inst, NULL);
		SendMessageW(brand, WM_SETFONT, (WPARAM)brandFont, TRUE);
		SendMessageW(heading, WM_SETFONT, (WPARAM)headingFont, TRUE);
		SendMessageW(text, WM_SETFONT, (WPARAM)textFont, TRUE);
		SendMessageW(tryAgain, WM_SETFONT, (WPARAM)buttonFont, TRUE);
		SendMessageW(closeEs, WM_SETFONT, (WPARAM)buttonFont, TRUE);
	}
	SetWindowTextW(heading, title.c_str());
	SetWindowTextW(text, body.c_str());
	SetChecking(false);
	CoverScreen();
	Layout();
	ShowWindow(hwnd, SW_SHOWNORMAL);
	SetForegroundWindow(hwnd);
	SetFocus(tryAgain);
}

// The whole monitor EuroScope is on, taskbar included.
void AccessPopup::CoverScreen()
{
	if (hwnd == nullptr)
		return;
	HWND owner = GetWindow(hwnd, GW_OWNER);
	HMONITOR mon = MonitorFromWindow(owner != NULL ? owner : hwnd, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	RECT area = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
	if (GetMonitorInfoW(mon, &mi))
		area = mi.rcMonitor;
	RECT now;
	GetWindowRect(hwnd, &now);
	const bool moved = !EqualRect(&now, &area);
	SetWindowPos(hwnd, HWND_TOPMOST, area.left, area.top, area.right - area.left, area.bottom - area.top, SWP_NOACTIVATE);
	if (moved)
		Layout();
}

void AccessPopup::Layout()
{
	if (hwnd == nullptr)
		return;
	RECT c;
	GetClientRect(hwnd, &c);
	const int w = c.right - c.left, h = c.bottom - c.top;
	const int width = w - 80 < CONTENT_WIDTH ? w - 80 : CONTENT_WIDTH;
	// Allow two rows per message, in case a long one wraps
	const int textHeight = lineCount * LINE_HEIGHT * 2;
	const int total = BRAND_HEIGHT + 18 + HEADING_HEIGHT + 24 + textHeight + 28 + BUTTON_HEIGHT;
	const int left = (w - width) / 2;
	int y = (h - total) / 2;
	MoveWindow(brand, left, y, width, BRAND_HEIGHT, TRUE);
	y += BRAND_HEIGHT + 18;
	MoveWindow(heading, left, y, width, HEADING_HEIGHT, TRUE);
	y += HEADING_HEIGHT + 24;
	MoveWindow(text, left, y, width, textHeight, TRUE);
	y += textHeight + 28;
	const int buttonsLeft = (w - (2 * BUTTON_WIDTH + BUTTON_GAP)) / 2;
	MoveWindow(tryAgain, buttonsLeft, y, BUTTON_WIDTH, BUTTON_HEIGHT, TRUE);
	MoveWindow(closeEs, buttonsLeft + BUTTON_WIDTH + BUTTON_GAP, y, BUTTON_WIDTH, BUTTON_HEIGHT, TRUE);
}

void AccessPopup::KeepInFront()
{
	if (hwnd == nullptr)
		return;
	CoverScreen();
	if (!IsWindowVisible(hwnd))
		ShowWindow(hwnd, SW_SHOWNOACTIVATE);
}

void AccessPopup::SetChecking(bool now)
{
	checking = now;
	if (tryAgain == nullptr)
		return;
	EnableWindow(tryAgain, now ? FALSE : TRUE);
	SetWindowTextW(tryAgain, now ? L"Checking..." : L"Try again");
	InvalidateRect(tryAgain, NULL, TRUE);
}

void AccessPopup::Close()
{
	if (hwnd == nullptr)
		return;
	HWND w = hwnd;
	hwnd = nullptr;
	brand = heading = text = tryAgain = closeEs = nullptr;
	SetWindowLongPtrW(w, GWLP_USERDATA, 0);
	DestroyWindow(w);
}

// Try again: white, filled. Close EuroScope: outlined. Disabled (checking): dim.
void AccessPopup::DrawButton(const DRAWITEMSTRUCT* item)
{
	const bool primary = item->hwndItem == tryAgain;
	const bool disabled = (item->itemState & ODS_DISABLED) != 0;
	const bool pressed = (item->itemState & ODS_SELECTED) != 0;
	RECT r = item->rcItem;
	FillRect(item->hDC, &r, black);

	COLORREF fill = primary ? (disabled ? DIM : (pressed ? RGB(210, 210, 210) : WHITE)) : (pressed ? RGB(30, 30, 30) : BLACK);
	COLORREF edge = primary ? fill : (disabled ? DIM : GREY);
	COLORREF ink = primary ? BLACK : WHITE;
	HBRUSH brush = CreateSolidBrush(fill);
	HPEN pen = CreatePen(PS_SOLID, 1, edge);
	HGDIOBJ oldBrush = SelectObject(item->hDC, brush);
	HGDIOBJ oldPen = SelectObject(item->hDC, pen);
	RoundRect(item->hDC, r.left, r.top, r.right, r.bottom, 8, 8);
	SelectObject(item->hDC, oldBrush);
	SelectObject(item->hDC, oldPen);
	DeleteObject(brush);
	DeleteObject(pen);

	wchar_t label[64] = { 0 };
	GetWindowTextW(item->hwndItem, label, 64);
	SetBkMode(item->hDC, TRANSPARENT);
	SetTextColor(item->hDC, ink);
	HGDIOBJ oldFont = SelectObject(item->hDC, buttonFont);
	DrawTextW(item->hDC, label, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	SelectObject(item->hDC, oldFont);
	if ((item->itemState & ODS_FOCUS) != 0 && !primary)
	{
		RECT f = r;
		InflateRect(&f, -4, -4);
		DrawFocusRect(item->hDC, &f);
	}
}

LRESULT CALLBACK AccessPopup::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE)
	{
		CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
	}
	AccessPopup* self = (AccessPopup*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	switch (msg)
	{
	case WM_CLOSE:
		return 0; // no Alt+F4
	case WM_SYSCOMMAND:
		switch (wParam & 0xFFF0)
		{
		case SC_CLOSE:
		case SC_MOVE:
		case SC_MINIMIZE:
		case SC_SIZE:
			return 0;
		}
		break;
	case WM_CTLCOLORSTATIC:
		if (self != nullptr)
		{
			HWND which = (HWND)lParam;
			SetBkColor((HDC)wParam, BLACK);
			SetTextColor((HDC)wParam, which == self->heading ? RED : which == self->brand ? GREY : WHITE);
			return (LRESULT)self->black;
		}
		break;
	case WM_DRAWITEM:
		if (self != nullptr)
		{
			self->DrawButton((const DRAWITEMSTRUCT*)lParam);
			return TRUE;
		}
		break;
	case WM_COMMAND:
		if (self != nullptr && HIWORD(wParam) == BN_CLICKED)
		{
			if (LOWORD(wParam) == ID_TRY_AGAIN && self->onTryAgain)
			{
				self->onTryAgain();
				return 0;
			}
			if (LOWORD(wParam) == ID_CLOSE_ES && self->onCloseEuroScope)
			{
				self->onCloseEuroScope();
				return 0;
			}
		}
		break;
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}
