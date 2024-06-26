#include <windows.h>
#include <gdiplus.h>
#include <wchar.h>
#include <fstream>
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>
#include <vector>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <Shlwapi.h>
#include <CommCtrl.h>
#pragma comment (lib,"Gdiplus.lib")
#pragma comment (lib, "Shlwapi.lib")
#pragma comment(lib, "Comctl32.lib")

using namespace Gdiplus;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK AuthProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK ViewProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK OrderProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK PhotoProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

std::string getCurrentDate() {
    auto now = std::chrono::system_clock::now();
    time_t now_c = std::chrono::system_clock::to_time_t(now);
    struct tm parts;
    localtime_s(&parts, &now_c);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%d.%m.%Y", &parts);
    return std::string(buffer);
}

Gdiplus::GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;

void InitializeGDIPlus()
{
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

void ShutdownGDIPlus()
{
    Gdiplus::GdiplusShutdown(gdiplusToken);
}   

Image* images[9] = { nullptr };

HWND hAuthWnd = NULL;
HWND hMainWindow = NULL;
HWND addButton = NULL;
HWND changeButton = NULL;
HWND deleteButton = NULL;
HWND leftButton = NULL;
HWND rightButton = NULL;
HWND imageCounter = NULL;
HWND hYearComboBox = NULL;
HWND hModelEdit = NULL;
HWND hDescriptionEdit = NULL;

int currentImageIndex = 0;
int totalImages = 0;
std::wstring photoNames[9];

const int IMAGE_WIDTH = 200;
const int IMAGE_HEIGHT = 200;
const int WINDOW_WIDTH = 600;
const int WINDOW_HEIGHT = 500;

const wchar_t* szAppName = L"Sample Window Class";
const wchar_t* szAuthAppName = L"Auth Window Class";
const wchar_t* szMainWindowTitle = L"Програма без назви";

wchar_t selectedYear[5] = L"";
wchar_t selectedModel[100] = L"";
wchar_t selectedDescription[500] = L"";

struct CarInfo {
    std::wstring photos;
    std::wstring year;
    std::wstring model;
    std::wstring description;
};
std::vector<CarInfo> cars;
void LoadCarsData() {
    cars.clear();
    std::wifstream file(L"D:\\ДИПЛОМ\\WindowsProjectOleg\\materials\\cars.txt");
    file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));
    if (file.is_open()) {
        std::wstring line;
        while (std::getline(file, line)) {
            std::wistringstream wiss(line);
            std::wstring photos, year, model, description;

            if (std::getline(wiss, photos, L'/') && std::getline(wiss, year, L'/') &&
                std::getline(wiss, model, L'/') && std::getline(wiss, description)) {
                CarInfo car;
                car.photos = photos.substr(photos.find(L':') + 1);
                car.year = year.substr(year.find(L':') + 1);
                car.model = model.substr(model.find(L':') + 1);
                car.description = description.substr(description.find(L':') + 1);
                cars.push_back(car);
            }
        }
        file.close();
    }
}
void SaveCarsData()
{
    std::wofstream file(L"D:\\ДИПЛОМ\\WindowsProjectOleg\\materials\\cars.txt");
    file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));
    if (file.is_open())
    {
        for (const auto& car : cars)
        {
            file << L"Photos:" << car.photos
                << L"/Year:" << car.year
                << L"/Model:" << car.model
                << L"/Description:" << car.description << std::endl;
        }
        file.close();
    }
}
#include <regex>
void ShowPhotoWindow(HWND hWndParent, const std::wstring& photos) {
    std::wregex re(L"\"([^\"]+)\"");
    std::wsmatch match;
    std::wstring s = photos;
    std::vector<std::wstring> imagePaths;

    while (std::regex_search(s, match, re)) {
        imagePaths.push_back(match[1]);
        s = match.suffix().str();
    }

    if (!imagePaths.empty()) {
        WNDCLASS wc = {};
        wc.lpfnWndProc = PhotoProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"PhotoClass";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        RegisterClass(&wc);

        HWND hPhotoWnd = CreateWindowEx(
            0,
            L"PhotoClass",
            L"Фотографії авто",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 800, 800, // Розмір вікна збільшено
            hWndParent,
            NULL,
            GetModuleHandle(NULL),
            &imagePaths
        );

        if (hPhotoWnd != NULL) {
            ShowWindow(hPhotoWnd, SW_SHOW);
        }
    }
    else {
        MessageBox(hWndParent, L"Фото не знайдено.", L"Помилка", MB_OK | MB_ICONERROR);
    }
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    InitializeGDIPlus();

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    LoadCarsData();

    WNDCLASS wc = {};
    wc.lpfnWndProc = AuthProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = szAuthAppName;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    hAuthWnd = CreateWindowEx(
        0,
        szAuthAppName,
        L"Авторизація",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 350,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hAuthWnd == NULL)
    {
        return 0;
    }

    ShowWindow(hAuthWnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    for (int i = 0; i < 9; ++i)
    {
        delete images[i];
    }

    GdiplusShutdown(gdiplusToken);
    ShutdownGDIPlus();
    return (int)msg.wParam;

    return 0;
}

LRESULT CALLBACK AuthProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        CreateWindowW(L"STATIC", L"Програма для Автосалону", WS_VISIBLE | WS_CHILD | SS_CENTER, -100, 30, WINDOW_WIDTH, 30, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Логін:", WS_VISIBLE | WS_CHILD, 110, 90, 60, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Пароль:", WS_VISIBLE | WS_CHILD, 110, 120, 60, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 170, 90, 100, 20, hwnd, (HMENU)1, NULL, NULL);
        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD, 170, 120, 100, 20, hwnd, (HMENU)2, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Увійти", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 150, 155, 80, 30, hwnd, (HMENU)3, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Переглянути товар", WS_VISIBLE | WS_CHILD, 90, 210, 200, 30, hwnd, (HMENU)4, NULL, NULL);
        break;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case 3:
        {
            WCHAR login[50], password[50];
            GetWindowText(GetDlgItem(hwnd, 1), login, 50);
            GetWindowText(GetDlgItem(hwnd, 2), password, 50);
            if (wcscmp(login, L"admin") == 0 && wcscmp(password, L"admin") == 0)
            {
                MessageBox(hwnd, L"Успішна авторизація", L"Повідомлення", MB_OK | MB_ICONINFORMATION);
                WNDCLASS wc = {};
                wc.lpfnWndProc = WindowProc;
                wc.hInstance = GetModuleHandle(NULL);
                wc.lpszClassName = szAppName;
                wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
                RegisterClass(&wc);

                hMainWindow = CreateWindowEx(
                    0,
                    szAppName,
                    szMainWindowTitle,
                    WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
                    NULL,
                    NULL,
                    GetModuleHandle(NULL),
                    NULL
                );

                if (hMainWindow == NULL)
                {
                    return 0;
                }

                ShowWindow(hMainWindow, SW_SHOWDEFAULT);

                MSG msg = {};
                while (GetMessage(&msg, NULL, 0, 0))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                break;
            }
            else
            {
                MessageBox(hwnd, L"Неправильний логін або пароль", L"Помилка", MB_OK | MB_ICONERROR);
                SetFocus(GetDlgItem(hwnd, 1));
            }
            break;
        }
        case 4:
        {
            LoadCarsData();
            WNDCLASS wc = {};
            wc.lpfnWndProc = ViewProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"ViewCarsClass";
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            RegisterClass(&wc);

            HWND hViewWnd = CreateWindowEx(
                0,
                L"ViewCarsClass",
                L"Асортимент товару",
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, 700, 500,
                NULL,
                NULL,
                GetModuleHandle(NULL),
                NULL
            );

            if (hViewWnd != NULL)
            {
                ShowWindow(hViewWnd, SW_SHOW);
            }
            break;
        }
        }
        break;
    }
    case WM_CLOSE:
    {
        ShowWindow(hwnd, SW_HIDE);
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        addButton = CreateWindowW(L"BUTTON", L"Додати фото", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            10, 10, 150, 30, hwnd, (HMENU)1, NULL, NULL);

        changeButton = CreateWindowW(L"BUTTON", L"Змінити", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_DISABLED,
            220, 10, 100, 30, hwnd, (HMENU)2, NULL, NULL);

        deleteButton = CreateWindowW(L"BUTTON", L"Видалити фото", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_DISABLED,
            350, 10, 150, 30, hwnd, (HMENU)3, NULL, NULL);

        leftButton = CreateWindowW(L"BUTTON", L"<", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_DISABLED,
            20, 260, 30, 30, hwnd, (HMENU)4, NULL, NULL);

        rightButton = CreateWindowW(L"BUTTON", L">", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_DISABLED,
            130, 260, 30, 30, hwnd, (HMENU)5, NULL, NULL);

        imageCounter = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_CENTER, 60, 260, 60, 30, hwnd, NULL, NULL, NULL);

        CreateWindow(L"STATIC", L"Рік випуску:", WS_VISIBLE | WS_CHILD, 220, 50, 100, 25, hwnd, NULL, NULL, NULL);

        hYearComboBox = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_SIMPLE | WS_VSCROLL, 320, 50, 100, 200, hwnd, NULL, NULL, NULL);
        for (int year = 1960; year <= 2024; ++year) {
            wchar_t yearStr[5];
            _itow_s(year, yearStr, 10);
            SendMessage(hYearComboBox, CB_ADDSTRING, 0, (LPARAM)yearStr);
        }
        SendMessage(hYearComboBox, CB_SETCURSEL, 0, 0);

        CreateWindow(L"STATIC", L"Модель:", WS_VISIBLE | WS_CHILD, 220, 90, 100, 25, hwnd, NULL, NULL, NULL);
        hModelEdit = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 320, 90, 200, 25, hwnd, (HMENU)6, NULL, NULL);
        CreateWindow(L"STATIC", L"Опис:", WS_VISIBLE | WS_CHILD, 220, 130, 100, 25, hwnd, NULL, NULL, NULL);
        hDescriptionEdit = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE, 320, 130, 200, 100, hwnd, (HMENU)7, NULL, NULL);
        CreateWindow(L"BUTTON", L"Додати авто", WS_VISIBLE | WS_CHILD, 280, 260, 150, 35, hwnd, (HMENU)8, NULL, NULL);

        break;
    }
    case WM_COMMAND:
    {
        if (HIWORD(wParam) == BN_CLICKED)
        {
            if (LOWORD(wParam) == 1)
            {
                if (totalImages < 9)
                {
                    OPENFILENAME ofn;
                    WCHAR szFile[MAX_PATH] = L"";

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFile = szFile;
                    ofn.lpstrFile[0] = L'\0';
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = L"Image Files\0*.bmp;*.jpg;*.png;*.gif\0All Files\0*.*\0";
                    ofn.nFilterIndex = 1;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_DONTADDTORECENT;

                    if (GetOpenFileName(&ofn) == TRUE)
                    {
                        if (images[totalImages] != nullptr)
                            delete images[totalImages];
                        images[totalImages] = new Image(ofn.lpstrFile);
                        photoNames[totalImages] = ofn.lpstrFile;

                        currentImageIndex = totalImages;
                        ++totalImages;

                        wchar_t counterText[32];
                        swprintf_s(counterText, sizeof(counterText) / sizeof(wchar_t), L"%d/%d", currentImageIndex + 1, totalImages);
                        SetWindowText(imageCounter, counterText);

                        EnableWindow(changeButton, TRUE);
                        EnableWindow(deleteButton, TRUE);
                        EnableWindow(leftButton, TRUE);
                        EnableWindow(rightButton, TRUE);

                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                }
                else
                {
                    MessageBox(hwnd, L"Ліміт фотографій досягнуто. Неможливо додати більше.", L"Попередження", MB_OK | MB_ICONWARNING);
                }
            }
            else if (LOWORD(wParam) == 2)
            {
                if (currentImageIndex >= 0 && currentImageIndex < totalImages)
                {
                    OPENFILENAME ofn;
                    WCHAR szFile[MAX_PATH] = L"";

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFile = szFile;
                    ofn.lpstrFile[0] = L'\0';
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = L"Image Files\0*.bmp;*.jpg;*.png;*.gif\0All Files\0*.*\0";
                    ofn.nFilterIndex = 1;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_DONTADDTORECENT;

                    if (GetOpenFileName(&ofn) == TRUE)
                    {
                        delete images[currentImageIndex];
                        images[currentImageIndex] = new Image(ofn.lpstrFile);
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                }
            }
            else if (LOWORD(wParam) == 3)
            {
                if (currentImageIndex >= 0 && currentImageIndex < totalImages)
                {
                    delete images[currentImageIndex];
                    for (int i = currentImageIndex; i < totalImages - 1; ++i)
                    {
                        images[i] = images[i + 1];
                        photoNames[i] = photoNames[i + 1];
                    }
                    --totalImages;
                    if (totalImages == 0)
                    {
                        currentImageIndex = 0;

                        EnableWindow(changeButton, FALSE);
                        EnableWindow(deleteButton, FALSE);
                        EnableWindow(leftButton, FALSE);
                        EnableWindow(rightButton, FALSE);
                    }
                    else if (currentImageIndex >= totalImages)
                    {
                        --currentImageIndex;
                    }

                    wchar_t counterText[32];
                    swprintf_s(counterText, sizeof(counterText) / sizeof(wchar_t), L"%d/%d", currentImageIndex + 1, totalImages);
                    SetWindowText(imageCounter, counterText);

                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }
            else if (LOWORD(wParam) == 4)
            {
                if (currentImageIndex > 0)
                {
                    --currentImageIndex;
                    wchar_t counterText[32];
                    swprintf_s(counterText, sizeof(counterText) / sizeof(wchar_t), L"%d/%d", currentImageIndex + 1, totalImages);
                    SetWindowText(imageCounter, counterText);
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }
            else if (LOWORD(wParam) == 5)
            {
                if (currentImageIndex < totalImages - 1)
                {
                    ++currentImageIndex;
                    wchar_t counterText[32];
                    swprintf_s(counterText, sizeof(counterText) / sizeof(wchar_t), L"%d/%d", currentImageIndex + 1, totalImages);
                    SetWindowText(imageCounter, counterText);
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }
            else if (LOWORD(wParam) == 8)
            {
                int yearIndex = SendMessage(hYearComboBox, CB_GETCURSEL, 0, 0);
                if (yearIndex != CB_ERR)
                {
                    wchar_t yearStr[5];
                    SendMessage(hYearComboBox, CB_GETLBTEXT, yearIndex, (LPARAM)yearStr);
                    wcscpy_s(selectedYear, yearStr);
                }
                else
                {
                    MessageBox(hwnd, L"Виберіть рік випуску", L"Помилка", MB_OK | MB_ICONERROR);
                    return 0;
                }
                GetWindowText(hModelEdit, selectedModel, 100);
                if (wcslen(selectedModel) == 0)
                {
                    MessageBox(hwnd, L"Введіть модель", L"Помилка", MB_OK | MB_ICONERROR);
                    return 0;
                }
                GetWindowText(hDescriptionEdit, selectedDescription, 500);
                for (wchar_t* p = selectedDescription; *p != '\0'; ++p)
                {
                    if (*p == '\r' || *p == '\n')
                    {
                        *p = ' ';
                    }
                }

                if (wcslen(selectedDescription) == 0)
                {
                    MessageBox(hwnd, L"Введіть опис", L"Помилка", MB_OK | MB_ICONERROR);
                    return 0;
                }
                std::wstring wideYear(selectedYear);
                std::wstring wideModel(selectedModel);
                std::wstring wideDescription(selectedDescription);

                std::string year = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(wideYear);
                std::string model = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(wideModel);
                std::string description = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(wideDescription);

                std::string filePath = "D:\\ДИПЛОМ\\WindowsProjectOleg\\materials\\cars.txt";

                std::ofstream file(filePath, std::ios::out | std::ios::app | std::ios::binary);

                if (file.is_open()) {
                    file << "\xEF\xBB\xBF";
                    file << "Photos: ";
                    for (int i = 0; i < totalImages; ++i) {
                        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                        std::string photoName = converter.to_bytes(photoNames[i]);
                        file << "\"" << photoName << "\"";
                        if (i != totalImages - 1)
                            file << ", ";
                    }
                    file << " / Year: " << year << " / Model: " << model << " / Description: " << description << "\n";
                    file.close();

                    MessageBox(hwnd, L"Інформація успішно занесена в базу даних.", L"Повідомлення", MB_OK | MB_ICONINFORMATION);
                }
                else {
                    MessageBox(hwnd, L"Не вдалося відкрити файл для запису.", L"Помилка", MB_OK | MB_ICONERROR);
                }
            }
        }
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        if (totalImages > 0 && currentImageIndex >= 0 && currentImageIndex < totalImages)
        {
            Graphics graphics(hdc);
            graphics.DrawImage(images[currentImageIndex], 0, 50, IMAGE_WIDTH, IMAGE_HEIGHT);
        }

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_CLOSE:
    {
        if (MessageBox(hwnd, L"Ви впевнені, що хочете вийти?", L"Попередження", MB_OKCANCEL | MB_ICONWARNING) == IDOK)
        {
            DestroyWindow(hwnd);
        }
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK OrderProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hNameEdit, hPhoneEdit;
    static int carIndex;
    static HWND hParentWnd;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        carIndex = ((CREATESTRUCT*)lParam)->lpCreateParams ? *((int*)((CREATESTRUCT*)lParam)->lpCreateParams) : -1;
        hParentWnd = GetParent(hwnd);
        CreateWindowW(L"STATIC", L"Ваш email:", WS_VISIBLE | WS_CHILD, 20, 20, 100, 20, hwnd, NULL, NULL, NULL);
        hNameEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 120, 20, 200, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Ваш телефон:", WS_VISIBLE | WS_CHILD, 20, 60, 100, 20, hwnd, NULL, NULL, NULL);
        hPhoneEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 120, 60, 200, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Підтвердити замовлення", WS_VISIBLE | WS_CHILD, 120, 100, 200, 30, hwnd, (HMENU)1, NULL, NULL);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 1)
        {
            wchar_t name[100], phone[100];
            GetWindowText(hNameEdit, name, 100);
            GetWindowText(hPhoneEdit, phone, 100);

            if (wcslen(name) == 0 || wcslen(phone) == 0) {
                MessageBox(hwnd, L"Будь ласка, введіть ваш email та номер телефону.", L"Помилка", MB_OK | MB_ICONERROR);
                return 0;
            }
            CarInfo car = cars[carIndex];
            std::ofstream file("order.csv", std::ios::out | std::ios::app);
            if (file.is_open())
            {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                std::string carYear = converter.to_bytes(car.year);
                std::string carModel = converter.to_bytes(car.model);
                std::string carDescription = converter.to_bytes(car.description);
                std::string userName = converter.to_bytes(name);
                std::string userPhone = converter.to_bytes(phone);

                std::string date = getCurrentDate();
                file << "Нове замовлення: "<< ", " << carYear << ", " << carModel << ", " << carDescription << ", " << userName << ", " << userPhone << "," << date << std::endl;
                file.close();

                MessageBox(hwnd, L"Замовлення оформлено! Незабаром з вами зв'яжуться.", L"Успіх", MB_OK);
                cars.erase(cars.begin() + carIndex);
                SaveCarsData();
                SendMessage(hParentWnd, WM_USER + 1, 0, 0);
                DestroyWindow(hParentWnd);
                DestroyWindow(hwnd);
            }
            else
            {
                MessageBox(hwnd, L"Не вдалося відкрити файл для запису.", L"Помилка", MB_OK | MB_ICONERROR);
            }
        }
        break;
    case WM_DESTROY:
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
LRESULT CALLBACK ViewProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND* orderButtons = nullptr;
    static HWND* photoButtons = nullptr;
    static HWND* productLabels = nullptr;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        LoadCarsData();
        int n = cars.size();
        orderButtons = new HWND[n];
        photoButtons = new HWND[n];
        productLabels = new HWND[n];

        CreateWindowW(L"STATIC", L"Список актуальних авто", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 10, 660, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"№", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 40, 30, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Рік випуску", WS_VISIBLE | WS_CHILD | SS_CENTER, 40, 40, 120, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Марка", WS_VISIBLE | WS_CHILD | SS_CENTER, 150, 40, 120, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Опис", WS_VISIBLE | WS_CHILD | SS_CENTER, 270, 40, 240, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Дія", WS_VISIBLE | WS_CHILD | SS_CENTER, 510, 40, 160, 20, hwnd, NULL, NULL, NULL);

        for (int i = 0; i < n; i++)
        {
            std::wstring productText = std::to_wstring(i + 1) + L"          " + cars[i].year + L"                    " + cars[i].model + L"                " + cars[i].description;
            productLabels[i] = CreateWindowW(L"STATIC", productText.c_str(), WS_VISIBLE | WS_CHILD, 10, 70 + i * 40, 500, 30, hwnd, NULL, NULL, NULL);
            orderButtons[i] = CreateWindowW(L"BUTTON", L"Замовити", WS_VISIBLE | WS_CHILD, 520, 70 + i * 40, 100, 30, hwnd, (HMENU)(1000 + i), NULL, NULL);
            photoButtons[i] = CreateWindowW(L"BUTTON", L"Фото", WS_VISIBLE | WS_CHILD, 630, 70 + i * 40, 50, 30, hwnd, (HMENU)(2000 + i), NULL, NULL);
        }
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) >= 1000 && LOWORD(wParam) < 2000)
        {
            int carIndex = LOWORD(wParam) - 1000;

            WNDCLASS wc = {};
            wc.lpfnWndProc = OrderProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"OrderClass";
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            RegisterClass(&wc);

            HWND hOrderWnd = CreateWindowEx(
                0,
                L"OrderClass",
                L"Замовити товар",
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, 400, 200,
                hwnd,
                NULL,
                GetModuleHandle(NULL),
                &carIndex
            );

            if (hOrderWnd != NULL)
            {
                ShowWindow(hOrderWnd, SW_SHOW);
            }
        }
        else if (LOWORD(wParam) >= 2000) {
            int carIndex = LOWORD(wParam) - 2000;
            if (carIndex < cars.size()) {
                std::wstring photoPath = cars[carIndex].photos;
                ShowPhotoWindow(hwnd, photoPath);
            }
        }

        break;
    case WM_USER + 1:
    {
        for (int i = 0; i < cars.size(); i++)
        {
            DestroyWindow(productLabels[i]);
            DestroyWindow(orderButtons[i]);
            DestroyWindow(photoButtons[i]);
        }
        delete[] productLabels;
        delete[] orderButtons;
        delete[] photoButtons;

        LoadCarsData();
        int n = cars.size();
        orderButtons = new HWND[n];
        photoButtons = new HWND[n];
        productLabels = new HWND[n];

        for (int i = 0; i < n; i++)
        {
            std::wstring productText = std::to_wstring(i + 1) + L"      " + cars[i].year + L"      " + cars[i].model + L"      " + cars[i].description;
            productLabels[i] = CreateWindowW(L"STATIC", productText.c_str(), WS_VISIBLE | WS_CHILD, 10, 70 + i * 40, 500, 30, hwnd, NULL, NULL, NULL);
            orderButtons[i] = CreateWindowW(L"BUTTON", L"Замовити", WS_VISIBLE | WS_CHILD, 520, 70 + i * 40, 100, 30, hwnd, (HMENU)(1000 + i), NULL, NULL);
            photoButtons[i] = CreateWindowW(L"BUTTON", L"Фото", WS_VISIBLE | WS_CHILD, 630, 70 + i * 40, 50, 30, hwnd, (HMENU)(2000 + i), NULL, NULL);
        }
        break;
    }
    case WM_DESTROY:
        delete[] orderButtons;
        delete[] photoButtons;
        delete[] productLabels;
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
LRESULT CALLBACK PhotoProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static std::vector<std::wstring> imagePaths;
    static int currentIndex = 0;
    static Gdiplus::Image* image = nullptr;
    static HWND hIndexLabel = nullptr;
    static int windowWidth = 550;
    static int windowHeight = 700;
    const int IMAGE_SIZE = 500;

    switch (uMsg) {
    case WM_CREATE: {
        SetWindowPos(hwnd, NULL, 0, 0, windowWidth, windowHeight, SWP_NOMOVE | SWP_NOZORDER);
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        std::vector<std::wstring>* paths = reinterpret_cast<std::vector<std::wstring>*>(pCreate->lpCreateParams);
        imagePaths = *paths;
        currentIndex = 0;

        if (!imagePaths.empty()) {
            image = Gdiplus::Image::FromFile(imagePaths[currentIndex].c_str());
            if (image->GetLastStatus() != Gdiplus::Ok) {
                MessageBox(hwnd, (L"Не вдалося завантажити зображення: " + imagePaths[currentIndex]).c_str(), L"Помилка", MB_OK | MB_ICONERROR);
                delete image;
                image = nullptr;
                DestroyWindow(hwnd);
            }
        }
        else {
            MessageBox(hwnd, L"Шлях до зображення порожній.", L"Помилка", MB_OK | MB_ICONERROR);
            DestroyWindow(hwnd);
        }

        CreateWindowW(L"BUTTON", L"<", WS_VISIBLE | WS_CHILD, 180, 20, 60, 30, hwnd, (HMENU)1, NULL, NULL);
        CreateWindowW(L"BUTTON", L">", WS_VISIBLE | WS_CHILD, 330, 20, 60, 30, hwnd, (HMENU)2, NULL, NULL);
        hIndexLabel = CreateWindowW(L"STATIC", L"1/1", WS_VISIBLE | WS_CHILD | SS_CENTER, 260, 20, 60, 30, hwnd, NULL, NULL, NULL);
        std::wstring indexText = std::to_wstring(currentIndex + 1) + L"/" + std::to_wstring(imagePaths.size());
        SetWindowText(hIndexLabel, indexText.c_str());
        break;
    }
    case WM_SIZE: {
        windowWidth = LOWORD(lParam);
        windowHeight = HIWORD(lParam);
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 1) {
            if (currentIndex > 0) {
                currentIndex--;
                delete image;
                image = Gdiplus::Image::FromFile(imagePaths[currentIndex].c_str());
                InvalidateRect(hwnd, NULL, TRUE);
                std::wstring indexText = std::to_wstring(currentIndex + 1) + L"/" + std::to_wstring(imagePaths.size());
                SetWindowText(hIndexLabel, indexText.c_str());
            }
        }
        else if (LOWORD(wParam) == 2) {
            if (currentIndex < imagePaths.size() - 1) {
                currentIndex++;
                delete image;
                image = Gdiplus::Image::FromFile(imagePaths[currentIndex].c_str());
                InvalidateRect(hwnd, NULL, TRUE);
                std::wstring indexText = std::to_wstring(currentIndex + 1) + L"/" + std::to_wstring(imagePaths.size());
                SetWindowText(hIndexLabel, indexText.c_str());
            }
        }
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        HBRUSH hGrayBrush = CreateSolidBrush(RGB(192, 192, 192));
        FillRect(hdc, &clientRect, hGrayBrush);
        DeleteObject(hGrayBrush);
        HPEN hPen = CreatePen(PS_SOLID, 3, RGB(0, 0, 0));
        SelectObject(hdc, hPen);
        int x = (windowWidth - IMAGE_SIZE) / 2;
        int y = (windowHeight - IMAGE_SIZE) / 2;
        Rectangle(hdc, x, y, x + IMAGE_SIZE, y + IMAGE_SIZE);
        DeleteObject(hPen);

        if (image) {
            Gdiplus::Graphics graphics(hdc);
            int imageX = x + 3;
            int imageY = y + 3;
            int imageSize = IMAGE_SIZE - 6;
            graphics.DrawImage(image, imageX, imageY, imageSize, imageSize);
        }
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY: {
        if (image) {
            delete image;
        }
        break;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
