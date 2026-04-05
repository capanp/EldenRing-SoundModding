// ============================================================
//  ER TR Dublaj Mod Başlatıcı - main.cpp
//  Dear ImGui + Win32 + DirectX11 | UTF-8 Desteği
// ============================================================

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <windows.h>
#include <string>
#include <vector>
#include <shlobj.h>
#include <shellapi.h>
#include <windowsx.h>
#include <thread>
#include <mutex>
#include <fstream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

// ---- Pencere boyutlari ----
static const int   APP_W = 740;
static const int   APP_H = 650;
static const float TB_H = 30.0f;

// ---- D3D globalleri ----
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                    g_SwapChainOccluded = false;
static UINT                    g_ResizeWidth = 0;
static UINT                    g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRTV = nullptr;

// ---- Uygulama globalleri ----
static HWND g_hWnd = nullptr;

// ---- Oyun yolu ----
static char g_GamePath[MAX_PATH] = {};

// ---- Başlatma Seçenekleri ----
static bool g_IsCrackVersion = false;
static bool g_UseSeamlessCoop = false;

// ---- Log sistemi ----
struct LogEntry
{
    std::string msg;
    ImVec4      col;
    std::string ts;
};
static std::vector<LogEntry> g_Log;
static bool                  g_LogScrollToBottom = false;
static std::mutex            g_LogMutex;

static void Log(const char* text, ImVec4 col = { 0.84f, 0.84f, 0.84f, 1.0f })
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    char ts[12];
    sprintf_s(ts, "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);

    std::lock_guard<std::mutex> lock(g_LogMutex);
    g_Log.push_back({ text, col, ts });
    g_LogScrollToBottom = true;
}

// ============================================================
//  Yardımcı fonksiyonlar
// ============================================================

static bool FileExists(const std::string& path)
{
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

static bool IsValidERPath(const std::string& path)
{
    return !path.empty() && FileExists(path + "\\eldenring.exe");
}

// Steam registry, VDF taraması ve varsayılan crack yolları
static std::string AutoDetectERPath()
{
    std::vector<std::string> possiblePaths;

    // 1. HKEY_CURRENT_USER altındaki Steam Apps kaydı
    HKEY hk_cu;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\Apps\\1245620", 0, KEY_READ, &hk_cu) == ERROR_SUCCESS)
    {
        char  val[MAX_PATH];
        DWORD sz = sizeof(val);
        if (RegQueryValueExA(hk_cu, "InstallLocation", nullptr, nullptr, (LPBYTE)val, &sz) == ERROR_SUCCESS)
        {
            std::string p = val;
            possiblePaths.push_back(p);
            possiblePaths.push_back(p + "\\Game");
        }
        RegCloseKey(hk_cu);
    }

    // 2. Windows kaldırıcı
    const char* hklm_keys[] = {
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 1245620",
        "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 1245620"
    };
    for (auto& k : hklm_keys)
    {
        HKEY hk;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, k, 0, KEY_READ, &hk) == ERROR_SUCCESS)
        {
            char  val[MAX_PATH];
            DWORD sz = sizeof(val);
            if (RegQueryValueExA(hk, "InstallLocation", nullptr, nullptr, (LPBYTE)val, &sz) == ERROR_SUCCESS)
            {
                std::string p = val;
                possiblePaths.push_back(p);
                possiblePaths.push_back(p + "\\Game");
            }
            RegCloseKey(hk);
        }
    }

    // 3. Steam'in ana dizininden diğer kütüphaneleri
    HKEY hk_steam;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_READ, &hk_steam) == ERROR_SUCCESS)
    {
        char val[MAX_PATH];
        DWORD sz = sizeof(val);
        if (RegQueryValueExA(hk_steam, "SteamPath", nullptr, nullptr, (LPBYTE)val, &sz) == ERROR_SUCCESS)
        {
            std::string steamPath = val;
            possiblePaths.push_back(steamPath + "\\steamapps\\common\\ELDEN RING\\Game");

            std::string vdfPath = steamPath + "\\steamapps\\libraryfolders.vdf";
            std::ifstream vdfFile(vdfPath);
            if (vdfFile.is_open())
            {
                std::string line;
                while (std::getline(vdfFile, line))
                {
                    if (line.find("\"path\"") != std::string::npos)
                    {
                        size_t firstQuote = line.find("\"", line.find("\"path\"") + 6);
                        if (firstQuote != std::string::npos)
                        {
                            size_t secondQuote = line.find("\"", firstQuote + 1);
                            if (secondQuote != std::string::npos)
                            {
                                std::string libPath = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                                size_t pos = 0;
                                while ((pos = libPath.find("\\\\", pos)) != std::string::npos) {
                                    libPath.replace(pos, 2, "\\");
                                    pos += 1;
                                }
                                possiblePaths.push_back(libPath + "\\steamapps\\common\\ELDEN RING\\Game");
                            }
                        }
                    }
                }
                vdfFile.close();
            }
        }
        RegCloseKey(hk_steam);
    }

    std::vector<std::string> drives = { "C:", "D:", "E:", "F:", "G:" };
    std::vector<std::string> commonFolders = {
        "\\Games\\ELDEN RING\\Game",
        "\\Games\\Elden Ring\\Game",
        "\\Program Files (x86)\\ELDEN RING\\Game",
        "\\Program Files\\ELDEN RING\\Game",
        "\\Oyunlar\\ELDEN RING\\Game",
        "\\Oyunlar\\Elden Ring\\Game",
        "\\ELDEN RING\\Game"
    };

    for (const auto& d : drives) {
        for (const auto& f : commonFolders) {
            possiblePaths.push_back(d + f);
        }
    }

    for (const auto& p : possiblePaths)
    {
        if (IsValidERPath(p))
            return p;
    }

    return "";
}

// Klasör seç dialogu
static std::string BrowseFolder()
{
    char buf[MAX_PATH] = {};
    BROWSEINFOA bi = {};
    bi.hwndOwner = g_hWnd;
    bi.pszDisplayName = buf;
    bi.lpszTitle = "Elden Ring oyun klasorunu secin (eldenring.exe iceren klasor)";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl)
    {
        SHGetPathFromIDListA(pidl, buf);
        CoTaskMemFree(pidl);
        return std::string(buf);
    }
    return {};
}

// Uygulama klasörünü al
static std::string GetAppDir()
{
    char exe[MAX_PATH];
    GetModuleFileNameA(nullptr, exe, MAX_PATH);
    std::string s(exe);
    size_t pos = s.rfind('\\');
    return (pos != std::string::npos) ? s.substr(0, pos) : s;
}

// Modu başlat
static void LaunchMod()
{
    if (!strlen(g_GamePath))
    {
        Log("[HATA] Oyun dizini belirlenmedi!", { 1.0f, 0.32f, 0.32f, 1.0f });
        return;
    }
    if (!IsValidERPath(g_GamePath))
    {
        Log("[HATA] eldenring.exe bu dizinde bulunamadı!", { 1.0f, 0.32f, 0.32f, 1.0f });
        return;
    }

    std::string appDir = GetAppDir();
    std::string me3Exe = appDir + "\\kaynaklar\\me3\\bin\\me3.exe";

    if (!FileExists(me3Exe))
    {
        Log("[HATA] me3.exe bulunamadı: kaynaklar\\me3\\bin\\me3.exe", { 1.0f, 0.32f, 0.32f, 1.0f });
        return;
    }

    std::string profilePath = g_UseSeamlessCoop
        ? (appDir + "\\kaynaklar\\me3\\config\\eldenring-tr-seamlesscoop.me3")
        : (appDir + "\\kaynaklar\\me3\\config\\eldenring-tr.me3");

    if (!FileExists(profilePath))
    {
        std::string errMsg = "[HATA] Profil dosyası bulunamadı: " + profilePath;
        Log(errMsg.c_str(), { 1.0f, 0.32f, 0.32f, 1.0f });
        return;
    }

    //me3.exe launch -p <profil> -g eldenring
    std::string cmd = "\"" + me3Exe + "\" launch -p \"" + profilePath + "\" -g eldenring";

    // EKSTRA PARAMETRE
    if (g_IsCrackVersion)
    {
        std::string gameExe = std::string(g_GamePath) + "\\eldenring.exe";
        cmd += " -e \"" + gameExe + "\" --skip-steam-init";
    }

    Log("[BİLGİ] Oyun başlatılıyor...", { 0.50f, 0.90f, 0.50f, 1.0f });
    Log(("[DEBUG] Komut: " + cmd).c_str(), { 0.40f, 0.55f, 0.40f, 1.0f });

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    std::vector<char> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back('\0');

    if (CreateProcessA(nullptr, cmdBuffer.data(), nullptr, nullptr, FALSE, 0,
        nullptr, g_GamePath, &si, &pi))
    {
        Log("[BİLGİ] Görev başarıyla me3.exe'ye iletildi.", { 0.50f, 0.90f, 0.50f, 1.0f });
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else
    {
        char err[128];
        sprintf_s(err, "[HATA] Süreç oluşturulamadı! Hata kodu: %lu", GetLastError());
        Log(err, { 1.0f, 0.32f, 0.32f, 1.0f });
    }
}

// ============================================================
//  Seamless Coop Fonksiyonları
// ============================================================
static std::string GetDLLDir()
{
    return GetAppDir() + "\\kaynaklar\\me3\\config\\dll";
}

static bool IsSeamlessCoopInstalled()
{
    return FileExists(GetDLLDir() + "\\ersc.dll");
}

static bool g_IsInstallingSC = false;

static void InstallSeamlessCoopThread()
{
    g_IsInstallingSC = true;
    Log("[BİLGİ] Seamless Coop son sürüm kontrol ediliyor...", { 0.4f, 0.8f, 0.9f, 1.0f });

    std::string appDir = GetAppDir();
    std::string dllDir = GetDLLDir();
    std::string ps1Path = appDir + "\\install_sc.ps1";
    std::string errLogPath = appDir + "\\sc_error.log";

    if (FileExists(errLogPath)) DeleteFileA(errLogPath.c_str());

    // TEMP POWERSHELL
    std::ofstream psFile(ps1Path);
    if (psFile.is_open())
    {
        psFile << "$ErrorActionPreference = 'Stop'\n"
            << "try {\n"
            // GitHub TLS 1.2
            << "    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12\n"
            << "    $apiUrl = 'https://api.github.com/repos/LukeYui/EldenRingSeamlessCoopRelease/releases/latest'\n"
            << "    $release = Invoke-RestMethod -Uri $apiUrl -UseBasicParsing\n"
            << "    $asset = $release.assets | Where-Object { $_.name -match '\\.zip$' } | Select-Object -First 1\n"
            << "    if ($null -eq $asset) { throw 'ZIP dosyası GitHub üzerinde bulunamadı.' }\n"
            << "    $zipPath = Join-Path $PSScriptRoot 'sc_temp.zip'\n"
            << "    $extPath = Join-Path $PSScriptRoot 'sc_temp_ext'\n"
            << "    Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $zipPath -UseBasicParsing\n"
            << "    Expand-Archive -Path $zipPath -DestinationPath $extPath -Force\n"
            << "    $targetDir = '" << dllDir << "'\n"
            << "    if (-not (Test-Path $targetDir)) { New-Item -ItemType Directory -Force -Path $targetDir | Out-Null }\n"
            << "    $dllFile = Get-ChildItem -Path $extPath -Recurse -Filter 'ersc.dll' | Select-Object -First 1\n"
            << "    if ($null -eq $dllFile) { throw 'İndirilen ZIP içinde ersc.dll bulunamadı!' }\n"
            << "    Copy-Item -Path \"$($dllFile.Directory.FullName)\\*\" -Destination $targetDir -Recurse -Force\n"
            << "    Remove-Item -Path $zipPath -Force\n"
            << "    Remove-Item -Path $extPath -Recurse -Force\n"
            << "    exit 0\n"
            << "} catch {\n"
            << "    $errMessage = $_.Exception.Message\n"
            << "    [System.IO.File]::WriteAllText('" << errLogPath << "', $errMessage)\n"
            << "    exit 1\n"
            << "}\n";
        psFile.close();

        Log("[BİLGİ] İndiriliyor ve ayıklanıyor... Lütfen bekleyin.", { 0.4f, 0.8f, 0.9f, 1.0f });

        // Betiği konsol açmadan gizlice çalıştır
        std::string cmd = "powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File \"" + ps1Path + "\"";
        STARTUPINFOA si = {};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi = {};

        if (CreateProcessA(nullptr, &cmd[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
        {
            WaitForSingleObject(pi.hProcess, INFINITE);
            DWORD exitCode;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            if (exitCode == 0)
            {
                // --- INI DÜZENLEME ---
                std::string iniPath = dllDir + "\\ersc_settings.ini";
                WritePrivateProfileStringA("PASSWORD", "cooppassword", "3131", iniPath.c_str());
                // -----------------------------------------------------
                g_UseSeamlessCoop = true;

                Log("[BİLGİ] Seamless Coop başarıyla kuruldu ve varsayılan şifre (3131) ayarlandı!", { 0.2f, 0.9f, 0.2f, 1.0f });
            }
            else
            {
                std::string realError = "Bilinmeyen bir hata oluştu. (Muhtemel API sınırı)";
                std::ifstream errFile(errLogPath);
                if (errFile.is_open())
                {
                    std::getline(errFile, realError);
                    errFile.close();
                }

                std::string failMsg = "[HATA] İşlemi tamamlayamadı: " + realError;
                Log(failMsg.c_str(), { 1.0f, 0.32f, 0.32f, 1.0f });
            }
        }
        else
        {
            Log("[HATA] Yükleme betiği başlatılamadı.", { 1.0f, 0.32f, 0.32f, 1.0f });
        }

        DeleteFileA(ps1Path.c_str());
        if (FileExists(errLogPath)) DeleteFileA(errLogPath.c_str());
    }
    else
    {
        Log("[HATA] Geçici kurulum betiği oluşturulamadı.", { 1.0f, 0.32f, 0.32f, 1.0f });
    }

    g_IsInstallingSC = false;
}

static void InstallSeamlessCoopAsync()
{
    if (g_IsInstallingSC) return;
    std::thread(InstallSeamlessCoopThread).detach();
}

static void UninstallSeamlessCoop()
{
    std::string dllDir = GetDLLDir();
    bool success = true;

    if (FileExists(dllDir + "\\ersc.dll")) {
        if (!DeleteFileA((dllDir + "\\ersc.dll").c_str())) success = false;
    }
    if (FileExists(dllDir + "\\ersc_settings.ini")) {
        DeleteFileA((dllDir + "\\ersc_settings.ini").c_str());
    }

    std::string localeDir = dllDir + "\\locale";
    if (FileExists(localeDir)) {
        std::string rmCmd = "cmd.exe /c rmdir /s /q \"" + localeDir + "\"";
        WinExec(rmCmd.c_str(), SW_HIDE);
    }

    if (success)
        Log("[BİLGİ] Seamless Coop başarıyla kaldırıldı.", { 0.8f, 0.8f, 0.32f, 1.0f });
    else
        Log("[HATA] ersc.dll silinemedi. Oyun çalışıyor olabilir mi?", { 1.0f, 0.32f, 0.32f, 1.0f });
}

// ============================================================
//  ImGui Tema
// ============================================================
static void ApplyCrackStyle()
{
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 0.0f;
    s.ChildRounding = 0.0f;
    s.FrameRounding = 0.0f;
    s.GrabRounding = 0.0f;
    s.PopupRounding = 0.0f;
    s.ScrollbarRounding = 0.0f;
    s.TabRounding = 0.0f;

    s.WindowBorderSize = 1.0f;
    s.ChildBorderSize = 1.0f;
    s.FrameBorderSize = 1.0f;
    s.PopupBorderSize = 1.0f;

    s.WindowPadding = { 10.0f, 10.0f };
    s.FramePadding = { 6.0f, 4.0f };
    s.ItemSpacing = { 8.0f, 6.0f };
    s.ScrollbarSize = 13.0f;
    s.GrabMinSize = 10.0f;

    ImVec4* c = s.Colors;

    // Arka planlar
    c[ImGuiCol_WindowBg] = { 0.09f, 0.09f, 0.09f, 1.0f };
    c[ImGuiCol_ChildBg] = { 0.065f,0.065f,0.065f,1.0f };
    c[ImGuiCol_PopupBg] = { 0.12f, 0.12f, 0.12f, 1.0f };

    // Çerçeveler
    c[ImGuiCol_FrameBg] = { 0.14f, 0.14f, 0.14f, 1.0f };
    c[ImGuiCol_FrameBgHovered] = { 0.20f, 0.20f, 0.20f, 1.0f };
    c[ImGuiCol_FrameBgActive] = { 0.27f, 0.27f, 0.27f, 1.0f };

    // Sınırlar
    c[ImGuiCol_Border] = { 0.33f, 0.33f, 0.33f, 1.0f };
    c[ImGuiCol_BorderShadow] = { 0.00f, 0.00f, 0.00f, 0.0f };

    // Başlık
    c[ImGuiCol_TitleBg] = { 0.07f, 0.07f, 0.07f, 1.0f };
    c[ImGuiCol_TitleBgActive] = { 0.11f, 0.11f, 0.11f, 1.0f };
    c[ImGuiCol_TitleBgCollapsed] = { 0.07f, 0.07f, 0.07f, 1.0f };

    // Menü çubuğu
    c[ImGuiCol_MenuBarBg] = { 0.11f, 0.11f, 0.11f, 1.0f };

    // Kaydırma çubuğu
    c[ImGuiCol_ScrollbarBg] = { 0.04f, 0.04f, 0.04f, 1.0f };
    c[ImGuiCol_ScrollbarGrab] = { 0.28f, 0.28f, 0.28f, 1.0f };
    c[ImGuiCol_ScrollbarGrabHovered] = { 0.38f, 0.38f, 0.38f, 1.0f };
    c[ImGuiCol_ScrollbarGrabActive] = { 0.50f, 0.50f, 0.50f, 1.0f };

    // Tıklamalar / kaydırıcılar
    c[ImGuiCol_CheckMark] = { 0.88f, 0.88f, 0.88f, 1.0f };
    c[ImGuiCol_SliderGrab] = { 0.44f, 0.44f, 0.44f, 1.0f };
    c[ImGuiCol_SliderGrabActive] = { 0.60f, 0.60f, 0.60f, 1.0f };

    // Butonlar
    c[ImGuiCol_Button] = { 0.20f, 0.20f, 0.20f, 1.0f };
    c[ImGuiCol_ButtonHovered] = { 0.30f, 0.30f, 0.30f, 1.0f };
    c[ImGuiCol_ButtonActive] = { 0.40f, 0.40f, 0.40f, 1.0f };

    // Header
    c[ImGuiCol_Header] = { 0.20f, 0.20f, 0.20f, 1.0f };
    c[ImGuiCol_HeaderHovered] = { 0.28f, 0.28f, 0.28f, 1.0f };
    c[ImGuiCol_HeaderActive] = { 0.36f, 0.36f, 0.36f, 1.0f };

    // Ayraçlar
    c[ImGuiCol_Separator] = { 0.30f, 0.30f, 0.30f, 1.0f };
    c[ImGuiCol_SeparatorHovered] = { 0.42f, 0.42f, 0.42f, 1.0f };
    c[ImGuiCol_SeparatorActive] = { 0.55f, 0.55f, 0.55f, 1.0f };

    // Yeniden boyutlandırma tutacağı
    c[ImGuiCol_ResizeGrip] = { 0.28f, 0.28f, 0.28f, 1.0f };
    c[ImGuiCol_ResizeGripHovered] = { 0.38f, 0.38f, 0.38f, 1.0f };
    c[ImGuiCol_ResizeGripActive] = { 0.50f, 0.50f, 0.50f, 1.0f };

    // Sekmeler
    c[ImGuiCol_Tab] = { 0.12f, 0.12f, 0.12f, 1.0f };
    c[ImGuiCol_TabHovered] = { 0.26f, 0.26f, 0.26f, 1.0f };
    c[ImGuiCol_TabActive] = { 0.21f, 0.21f, 0.21f, 1.0f };
    c[ImGuiCol_TabUnfocused] = { 0.09f, 0.09f, 0.09f, 1.0f };
    c[ImGuiCol_TabUnfocusedActive] = { 0.17f, 0.17f, 0.17f, 1.0f };

    // Metin
    c[ImGuiCol_Text] = { 0.88f, 0.88f, 0.88f, 1.0f };
    c[ImGuiCol_TextDisabled] = { 0.40f, 0.40f, 0.40f, 1.0f };
    c[ImGuiCol_TextSelectedBg] = { 0.28f, 0.28f, 0.28f, 0.70f };

    // Navigasyon
    c[ImGuiCol_NavHighlight] = { 0.50f, 0.50f, 0.50f, 1.0f };
}

// ============================================================
//  D3D yardımcı fonksiyonlar
// ============================================================
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ============================================================
//  MAIN
// ============================================================
int main(int, char**)
{
    CoInitialize(nullptr);
    ImGui_ImplWin32_EnableDpiAwareness();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"ER TR Dublaj Mod Başlatıcı";
    ::RegisterClassExW(&wc);

    // Ekranda ortala
    int sx = GetSystemMetrics(SM_CXSCREEN);
    int sy = GetSystemMetrics(SM_CYSCREEN);
    int posX = (sx - APP_W) / 2;
    int posY = (sy - APP_H) / 2;

    // WS_POPUP
    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName, L"ER TR Dublaj Mod Başlatıcı",
        WS_POPUP | WS_VISIBLE,
        posX, posY, APP_W, APP_H,
        nullptr, nullptr, wc.hInstance, nullptr
    );
    g_hWnd = hwnd;

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // ImGui kurulum
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;   // imgui.ini

    // --- FONT VE TÜRKÇE DESTEĞİ ---
    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 2;

    static const ImWchar turkish_ranges[] =
    {
        0x0020, 0x00FF,
        0x0100, 0x017F,
        0
    };

    // Windows Tahoma fontu
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\tahoma.ttf", 15.0f, &fontConfig, turkish_ranges);
    // ------------------------------------------------

    ImGui::StyleColorsDark();
    ApplyCrackStyle();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // ---- Başlatma: Elden Ring yolunu otomatik bul ----
    {
        std::string found = AutoDetectERPath();
        if (!found.empty() && IsValidERPath(found))
        {
            strncpy_s(g_GamePath, found.c_str(), MAX_PATH - 1);
            Log("[BİLGİ] Oyun dizini otomatik algılandı.", { 0.48f, 0.88f, 0.48f, 1.0f });
            Log(("[BİLGİ] Konum: " + found).c_str(), { 0.68f, 0.68f, 0.68f, 1.0f });
        }
        else
        {
            Log("[UYARI] Oyun kurulumu otomatik bulunamadı. Lütfen dizini elle seçin.", { 1.0f, 0.80f, 0.25f, 1.0f });
        }
    }

    // ---- Ana döngü ----
    const float BG_COL[4] = { 0.09f, 0.09f, 0.09f, 1.0f };
    bool done = false;

    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        // Pencere gizliyse bekle
        if (g_SwapChainOccluded &&
            g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Yeniden boyutlanma
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // --- ImGui Kare ---
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ========================
        //  Tam ekran kök pencere
        // ========================
        ImGui::SetNextWindowPos({ 0.0f, 0.0f });
        ImGui::SetNextWindowSize({ (float)APP_W, (float)APP_H });

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
        ImGui::Begin("##Root", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse
        );
        ImGui::PopStyleVar();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2      wp = ImGui::GetWindowPos();
        float       lh = ImGui::GetTextLineHeight();

        // ============================================
        //  ÖZEL BAŞLIK ÇUBUĞU
        // ============================================
        float tbY1 = wp.y;
        float tbY2 = wp.y + TB_H;

        // Başlık çubuğu arka planı
        dl->AddRectFilled({ wp.x, tbY1 }, { wp.x + APP_W, tbY2 }, IM_COL32(16, 16, 16, 255));

        // Alt sınır çizgisi
        dl->AddLine({ wp.x, tbY2 - 1.0f }, { wp.x + APP_W, tbY2 - 1.0f },
            IM_COL32(52, 52, 52, 255));

        // --- Simge kutusu ---
        float iconX1 = wp.x + 7.0f;
        float iconY1 = tbY1 + (TB_H - 18.0f) * 0.5f;
        float iconX2 = iconX1 + 18.0f;
        float iconY2 = iconY1 + 18.0f;
        dl->AddRectFilled({ iconX1, iconY1 }, { iconX2, iconY2 }, IM_COL32(165, 132, 42, 255));
        // Gölgeli çerçeve
        dl->AddRect({ iconX1, iconY1 }, { iconX2, iconY2 }, IM_COL32(110, 85, 25, 255));
        // İçerideki "ER" harfleri
        float txtX = iconX1 + 1.0f;
        float txtY = iconY1 + (18.0f - lh) * 0.5f;
        dl->AddText({ txtX, txtY }, IM_COL32(18, 14, 6, 255), "ER");

        // --- Başlık metni ---
        const char* titleStr = "TR Dublaj Mod Başlatıcı";
        float       titleTxtY = tbY1 + (TB_H - lh) * 0.5f;
        dl->AddText({ wp.x + 32.0f, titleTxtY }, IM_COL32(192, 192, 192, 255), titleStr);

        // --- Kapat butonu (X) ---
        float  clW = 36.0f;
        float  clX1 = wp.x + APP_W - clW;
        float  clX2 = wp.x + APP_W;
        ImVec2 clMin = { clX1, tbY1 };
        ImVec2 clMax = { clX2, tbY2 };
        bool   clHov = ImGui::IsMouseHoveringRect(clMin, clMax, false);
        dl->AddRectFilled(clMin, clMax, clHov ? IM_COL32(185, 34, 34, 255) : IM_COL32(0, 0, 0, 0));
        dl->AddText({ clX1 + (clW - ImGui::CalcTextSize("x").x) * 0.5f, titleTxtY },
            IM_COL32(205, 205, 205, 255), "x");
        if (clHov && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            done = true;

        // --- Küçült butonu (-) ---
        float  mnW = 36.0f;
        float  mnX1 = clX1 - mnW;
        float  mnX2 = clX1;
        ImVec2 mnMin = { mnX1, tbY1 };
        ImVec2 mnMax = { mnX2, tbY2 };
        bool   mnHov = ImGui::IsMouseHoveringRect(mnMin, mnMax, false);
        dl->AddRectFilled(mnMin, mnMax, mnHov ? IM_COL32(62, 62, 62, 255) : IM_COL32(0, 0, 0, 0));
        dl->AddText({ mnX1 + (mnW - ImGui::CalcTextSize("-").x) * 0.5f, titleTxtY },
            IM_COL32(205, 205, 205, 255), "-");
        if (mnHov && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            ::ShowWindow(hwnd, SW_MINIMIZE);

        // Dış çerçeve
        dl->AddRect({ wp.x, wp.y }, { wp.x + APP_W, wp.y + APP_H }, IM_COL32(48, 48, 48, 255));

        // ============================================
        //  İÇERİK ALANI (başlık çubuğunun altı)
        // ============================================
        ImGui::SetCursorPos({ 0.0f, TB_H });

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 10.0f, 8.0f });
        ImGui::BeginChild("##Content",
            { (float)APP_W, (float)APP_H - TB_H },
            false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        );
        ImGui::PopStyleVar();

        // --- Sekme çubuğu ---
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 6.0f, 7.0f });
        bool tabsOpen = ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_FittingPolicyScroll);
        ImGui::PopStyleVar();

        if (tabsOpen)
        {
            // ==========================================
            //  SEKME 1: BAŞLATMA
            // ==========================================
            if (ImGui::BeginTabItem("  Başlatma  "))
            {
                ImGui::Spacing();

                // -- Oyun dizini satırı --
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                ImGui::Text("Oyun Dizini:");

                float btnW = 60.0f;
                float spacing = 6.0f;
                float inputW = ImGui::GetContentRegionAvail().x - btnW - spacing;

                ImGui::SetNextItemWidth(inputW);
                ImGui::InputText("##gamepath", g_GamePath, MAX_PATH);
                ImGui::SameLine(0.0f, spacing);

                if (ImGui::Button("  Seç  ", { btnW, 0.0f }))
                {
                    std::string p = BrowseFolder();
                    if (!p.empty())
                    {
                        strncpy_s(g_GamePath, p.c_str(), MAX_PATH - 1);
                        if (IsValidERPath(p))
                            Log(("[BİLGİ] Oyun dizini seçildi: " + p).c_str(),
                                { 0.68f, 0.68f, 0.68f, 1.0f });
                        else
                            Log("[UYARI] Seçilen klasörde eldenring.exe bulunamadı!",
                                { 1.0f, 0.80f, 0.25f, 1.0f });
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                bool isSCInstalled = IsSeamlessCoopInstalled();

                // -- Başlatma Seçenekleri --
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                ImGui::Text("Başlatma Seçenekleri:");
                ImGui::Spacing();

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                ImGui::Checkbox("Crack / Korsan Sürüm (Steam Bağlantısını Atla)", &g_IsCrackVersion);

                if (!isSCInstalled) {
                    ImGui::BeginDisabled();
                    g_UseSeamlessCoop = false;
                }

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                ImGui::Checkbox("Seamless Coop Aktif Et", &g_UseSeamlessCoop);

                if (!isSCInstalled) {
                    ImGui::EndDisabled();
                    ImGui::SameLine();
                    ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.0f }, "(Kullanmak için aşağıdan kurmalısınız)");
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // -- Çok Oyunculu Kurulum --
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                ImGui::Text("Çok Oyunculu Mod Kurulumu (Seamless Coop):");
                ImGui::Spacing();

                if (g_IsInstallingSC)
                {
                    // Yükleniyor durumu
                    ImGui::PushStyleColor(ImGuiCol_Button, { 0.15f, 0.15f, 0.15f, 1.0f });
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.15f, 0.15f, 0.15f, 1.0f });
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.15f, 0.15f, 0.15f, 1.0f });

                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);

                    float btnWidth = ImGui::GetContentRegionAvail().x - 10.0f;

                    ImGui::Button("Kuruluyor... Lütfen bekleyin", { btnWidth, 0.0f });
                    ImGui::PopStyleColor(3);
                }
                else if (!isSCInstalled)
                {
                    // Kurma Butonu
                    ImGui::PushStyleColor(ImGuiCol_Button, { 0.12f, 0.22f, 0.12f, 1.0f });
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.18f, 0.32f, 0.18f, 1.0f });
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.24f, 0.42f, 0.24f, 1.0f });
                    ImGui::PushStyleColor(ImGuiCol_Border, { 0.20f, 0.50f, 0.20f, 1.0f });

                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);

                    float btnWidth = ImGui::GetContentRegionAvail().x - 10.0f;
                    if (ImGui::Button("Seamless Coop'u Kur (GitHub'dan Son Sürüm)", { btnWidth, 0.0f }))
                    {
                        InstallSeamlessCoopAsync();
                    }
                    ImGui::PopStyleColor(4);
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, { 0.22f, 0.12f, 0.12f, 1.0f });
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.32f, 0.18f, 0.18f, 1.0f });
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.42f, 0.24f, 0.24f, 1.0f });
                    ImGui::PushStyleColor(ImGuiCol_Border, { 0.50f, 0.20f, 0.20f, 1.0f });

                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);

                    float btnWidth = ImGui::GetContentRegionAvail().x - 10.0f;

                    if (ImGui::Button("Seamless Coop'u Kaldır", { btnWidth, 0.0f }))
                    {
                        UninstallSeamlessCoop();
                    }
                    ImGui::PopStyleColor(4);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // -- Aksiyon butonları --

                // Modu Başlat
                ImGui::PushStyleColor(ImGuiCol_Button, { 0.23f, 0.19f, 0.08f, 1.0f });
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.38f, 0.31f, 0.11f, 1.0f });
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.50f, 0.41f, 0.14f, 1.0f });
                ImGui::PushStyleColor(ImGuiCol_Border, { 0.55f, 0.44f, 0.16f, 1.0f });
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 12.0f, 7.0f });

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                if (ImGui::Button(">> MODU BAŞLAT <<"))
                    LaunchMod();

                ImGui::PopStyleVar();
                ImGui::PopStyleColor(4);

                ImGui::SameLine(0.0f, 10.0f);

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10.0f, 7.0f });
                if (ImGui::Button("Oyun Dizinine Git"))
                {
                    if (strlen(g_GamePath))
                        ShellExecuteA(nullptr, "open", g_GamePath, nullptr, nullptr, SW_SHOWDEFAULT);
                    else
                        Log("[HATA] Oyun dizini belirlenmedi!", { 1.0f, 0.32f, 0.32f, 1.0f });
                }

                // --- SAĞA YASLAMA ---
                float btnPaddingX = 10.0f * 2.0f;
                float githubWidth = ImGui::CalcTextSize("GitHub").x + btnPaddingX;
                float youtubeWidth = ImGui::CalcTextSize("YouTube").x + btnPaddingX;

                float totalRightWidth = githubWidth + 10.0f + youtubeWidth;

                float rightAlignX = ImGui::GetWindowContentRegionMax().x - totalRightWidth - 10.0f;

                if (rightAlignX > ImGui::GetCursorPosX())
                    ImGui::SameLine(rightAlignX);
                else
                    ImGui::SameLine(0.0f, 10.0f);

                // GitHub Butonu
                if (ImGui::Button("GitHub"))
                {
                    ShellExecuteA(nullptr, "open", "https://github.com/capanp", nullptr, nullptr, SW_SHOWDEFAULT);
                }

                // YouTube Butonu
                ImGui::SameLine(0.0f, 10.0f);
                if (ImGui::Button("YouTube"))
                {
                    ShellExecuteA(nullptr, "open", "https://www.youtube.com/@cap8738", nullptr, nullptr, SW_SHOWDEFAULT);
                }

                ImGui::PopStyleVar();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // -- Konsol alanı --
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                ImGui::Text("Konsol Çıktısı:");
                ImGui::Spacing();

                static const float VERSION_H = 22.0f;
                float consoleH = ImGui::GetContentRegionAvail().y - VERSION_H - 4.0f;

                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.04f, 0.04f, 1.0f));
                ImGui::BeginChild("##console", { 0.0f, consoleH }, true);

                {
                    std::lock_guard<std::mutex> lock(g_LogMutex);
                    for (auto& entry : g_Log)
                    {
                        ImGui::TextColored({ 0.40f, 0.40f, 0.40f, 1.0f }, "[%s]", entry.ts.c_str());
                        ImGui::SameLine(0.0f, 4.0f);
                        ImGui::TextColored(entry.col, "%s", entry.msg.c_str());
                    }
                }
                if (g_LogScrollToBottom)
                {
                    ImGui::SetScrollHereY(1.0f);
                    g_LogScrollToBottom = false;
                }

                ImGui::EndChild();
                ImGui::PopStyleColor();

                // Sürüm bilgisi - kayan marquee yazısı
                {
                    const char* marqueeText = "               Elden Ring Türkçe Dublaj Mod Başlatıcı v1.0 - Emeği Geçenler: Capan, Shion, Kabuto - Bu mod tamamen ücretsizdir, bu tarz daha fazla mod için Youtube'da yorum yapıp Nexusmods'da artı oy atın...               ";
                    const float scrollSpeed = 60.0f;   // piksel/saniye
                    const float lineH = ImGui::GetTextLineHeight();
                    const float areaW = ImGui::GetContentRegionAvail().x;

                    // Mevcut imleç pozisyonu
                    ImVec2 cursorScr = ImGui::GetCursorScreenPos();

                    // Metin genişliği
                    float textW = ImGui::CalcTextSize(marqueeText).x;

                    float offset = fmodf((float)ImGui::GetTime() * scrollSpeed, textW);

                    ImDrawList* dl2 = ImGui::GetWindowDrawList();
                    ImVec2      clipMin = { cursorScr.x, cursorScr.y };
                    ImVec2      clipMax = { cursorScr.x + areaW, cursorScr.y + lineH + 2.0f };

                    dl2->PushClipRect(clipMin, clipMax, true);

                    ImU32 textCol = IM_COL32(90, 90, 90, 255);

                    dl2->AddText({ cursorScr.x - offset, cursorScr.y }, textCol, marqueeText);

                    dl2->AddText({ cursorScr.x - offset + textW, cursorScr.y }, textCol, marqueeText);

                    dl2->PopClipRect();

                    ImGui::Dummy({ areaW, lineH });
                }

                ImGui::EndTabItem();
            }

            // ==========================================
            //  SEKME 2: YARDIM
            // ==========================================
            if (ImGui::BeginTabItem("  Yardım  "))
            {
                ImGui::Spacing();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                ImGui::TextColored({ 0.80f, 0.68f, 0.24f, 1.0f }, "Elden Ring Türkçe Dublaj Mod Başlatıcı - Yardım");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                ImGui::TextWrapped(
                    "Bu uygulamanın temel amacı Elden Ring Türkçe Dublaj modunu yürütmek. "
                    "Alt yapıda kullanılan kaynaklar:"
                );

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // -- Linkler --
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                ImGui::TextColored({ 0.65f, 0.65f, 0.65f, 1.0f }, "Kaynaklar:");
                ImGui::Spacing();

                auto LinkButton = [](const char* label, const char* url)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, { 0.38f, 0.62f, 1.0f, 1.0f });
                        ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.18f,0.18f,0.22f, 1.0f });
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.24f,0.24f,0.30f, 1.0f });
                        ImGui::PushStyleColor(ImGuiCol_Border, { 0.0f, 0.0f, 0.0f, 0.0f });
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, 1.0f });
                        if (ImGui::SmallButton(label))
                            ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
                        ImGui::PopStyleVar();
                        ImGui::PopStyleColor(5);
                    };

                ImGui::BulletText("Mod Engine 3:");
                ImGui::SameLine(0, 6);
                LinkButton("github.com/garyttierney/me3",
                    "https://github.com/garyttierney/me3");

                ImGui::BulletText("Dear ImGui:");
                ImGui::SameLine(0, 6);
                LinkButton("github.com/ocornut/imgui",
                    "https://github.com/ocornut/imgui");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // -- Linkler --
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                ImGui::TextColored({ 0.65f, 0.65f, 0.65f, 1.0f }, "İletişim bağlantıları: (Mümkünse Steam'den yazın)");
                ImGui::Spacing();

                ImGui::BulletText("Steam:");
                ImGui::SameLine(0, 6);
                LinkButton("steam.com/capansj",
                    "https://steamcommunity.com/id/capansj/");

                ImGui::BulletText("Discord:");
                ImGui::SameLine(0, 6);
                LinkButton("discord.com/ukzQuMB64g",
                    "https://discord.com/invite/ukzQuMB64g");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // -- Linkler --
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                ImGui::TextColored({ 0.65f, 0.65f, 0.65f, 1.0f }, "Ana mod dağıtım bağlantıları:");
                ImGui::Spacing();

                ImGui::BulletText("Nexus Mods:");
                ImGui::SameLine(0, 6);
                LinkButton("nexusmods.com/8969",
                    "https://www.nexusmods.com/eldenring/mods/8969");

                ImGui::BulletText("Donanım Haber:");
                ImGui::SameLine(0, 6);
                LinkButton("donanımhaber.com/elden-ring-turkce-dublaj",
                    "https://forum.donanimhaber.com/elden-ring-turkce-dublaj-v0-1-deepseek-elevenlabs--161229468");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // -- Kullanım adımları --
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                ImGui::TextColored({ 0.65f, 0.65f, 0.65f, 1.0f }, "Kullanım:");
                ImGui::Spacing();
                ImGui::BulletText("Oyun dizini otomatik algılanmazsa 'Seç' ile belirtin.");
                ImGui::BulletText("Oyununuz korsan ise ilk tiki aktif edin.");
                ImGui::BulletText("İsteğe bağlı seamless modunu kurun tuş ile.");
                ImGui::BulletText("'MODU BAŞLAT' tuşuna basın. Mod Engine oyunu başlatır.");
                ImGui::BulletText("kaynaklar\\me3\\ klasöründe Mod Engine dosyaları olmalıdır.");
                ImGui::Spacing();

                ImGui::Separator();
                ImGui::Spacing();

                // BENIOKU butonu
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                if (ImGui::Button("  BENIOKU.txt Aç  "))
                {
                    std::string readme = GetAppDir() + "\\BENIOKU.txt";
                    if (FileExists(readme))
                        ShellExecuteA(nullptr, "open", readme.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                    else
                        Log("[UYARI] BENIOKU.txt bulunamadı.", { 1.0f, 0.80f, 0.25f, 1.0f });
                }

                // Sürüm
                float availY = ImGui::GetContentRegionAvail().y;
                if (availY > 22.0f)
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + availY - 22.0f);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                ImGui::TextColored({ 0.35f, 0.35f, 0.35f, 1.0f }, "Elden Ring Türkçe Dublaj Mod Başlatıcı v1.0 - Capan");

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::EndChild(); // ##Content
        ImGui::End();      // ##Root

        // ---- DirectX render ----
        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRTV, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRTV, BG_COL);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // ---- Temizlik ----
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    CoUninitialize();
    return 0;
}

// ============================================================
//  WndProc
// ============================================================
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;

    case WM_NCHITTEST:
    {

        LRESULT hit = DefWindowProcW(hWnd, msg, wParam, lParam);
        if (hit == HTCLIENT)
        {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT  rc;
            GetWindowRect(hWnd, &rc);
            int rx = pt.x - rc.left;
            int ry = pt.y - rc.top;

            if (ry < (int)TB_H && rx < APP_W - 72)
                return HTCAPTION;
        }
        return hit;
    }
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ============================================================
//  D3D yardımcı fonksiyonları
// ============================================================
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT              createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0
    };

    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createDeviceFlags, featureLevelArray, 2,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain,
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext
    );
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            createDeviceFlags, featureLevelArray, 2,
            D3D11_SDK_VERSION, &sd, &g_pSwapChain,
            &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext
        );
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release();        g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release();        g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBack = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBack));
    if (pBack)
    {
        g_pd3dDevice->CreateRenderTargetView(pBack, nullptr, &g_mainRTV);
        pBack->Release();
    }
}

void CleanupRenderTarget()
{
    if (g_mainRTV) { g_mainRTV->Release(); g_mainRTV = nullptr; }
}
