#include <iostream>
#include <Windows.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <shlobj.h>

const char* usXmlContent = R"(
<gs:GlobalizationServices xmlns:gs="urn:longhornGlobalizationUnattend">
    <gs:UserList><gs:User UserID="Current"/></gs:UserList>
    <gs:InputPreferences>
        <!--add en-US keyboard input-->
        <gs:InputLanguageID Action="add" ID="0409:00000409"/>
        <!--remove en-US keyboard input-->
        <gs:InputLanguageID Action="remove" ID="0409:00000409"/>
    </gs:InputPreferences>
</gs:GlobalizationServices>
)";

bool isEnUSLayoutActive() {
    int numLayouts = GetKeyboardLayoutList(0, nullptr);
    if (numLayouts == 0) {
        std::cerr << "Cannot retrieve keyboard languages.";
        return false;
    }

    HKL* layouts = new HKL[numLayouts];
    GetKeyboardLayoutList(numLayouts, layouts);

    bool isEnUS = false;
    for (int i = 0; i < numLayouts; ++i) {
        wchar_t layoutName[KL_NAMELENGTH];
        LCIDToLocaleName(MAKELCID((DWORD_PTR)layouts[i], SORT_DEFAULT), layoutName, KL_NAMELENGTH, 0);
        if (wcscmp(layoutName, L"en-US") == 0) {
            isEnUS = true;
            break;
        }
    }

    delete[] layouts;

    return isEnUS;
}

void applyUSLayout() {
    std::ofstream file("US.xml");
    file << usXmlContent;
    file.close();

    system("control intl.cpl,, /f:\"US.xml\"");

    std::remove("US.xml");
}

void backgroundProcess() {
    while (true) {
        if (isEnUSLayoutActive()) {
            applyUSLayout();
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            {
                NOTIFYICONDATA nid;
                nid.cbSize = sizeof(NOTIFYICONDATA);
                nid.hWnd = hwnd;
                nid.uID = 1;
                nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
                nid.uCallbackMessage = WM_USER + 1;
                nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
                lstrcpy(nid.szTip, TEXT("US Layout Remover"));
                Shell_NotifyIcon(NIM_ADD, &nid);
            }
            break;
        case WM_DESTROY:
            {
                NOTIFYICONDATA nid;
                nid.cbSize = sizeof(NOTIFYICONDATA);
                nid.hWnd = hwnd;
                nid.uID = 1;
                Shell_NotifyIcon(NIM_DELETE, &nid);
            }
            PostQuitMessage(0);
            break;
        case WM_USER + 1:
            if (LOWORD(lParam) == WM_RBUTTONUP) {
                HMENU hPopupMenu = CreatePopupMenu();
                SetForegroundWindow(hwnd);
                POINT pt;
                GetCursorPos(&pt);
                TrackPopupMenu(hPopupMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
            }
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("USRemover");
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        TEXT("USRemover"),
        TEXT("USRemover"),
        0,
        0,
        0,
        0,
        0,
        HWND_MESSAGE,
        NULL,
        hInstance,
        NULL);

    if (hwnd == NULL) {
        return 0;
    }

    std::thread backgroundThread(backgroundProcess);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    backgroundThread.join();

    return 0;
}