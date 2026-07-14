#pragma once

#include <windows.h>
#include <shlobj.h>
#include <string>

inline std::wstring to_wchar(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

inline HRESULT CreateShortcut(LPCWSTR lpszPathObj, LPCWSTR lpszPathLink, LPCWSTR lpszDesc) {
    HRESULT hres;
    IShellLinkW* psl;

    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&psl);
    if (SUCCEEDED(hres)) {
        IPersistFile* ppf;
        psl->SetPath(lpszPathObj);
        psl->SetDescription(lpszDesc);

        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if (SUCCEEDED(hres)) {
            hres = ppf->Save(lpszPathLink, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    return hres;
}