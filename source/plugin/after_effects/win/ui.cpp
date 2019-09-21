#include "../ui.h"

#include <stdint.h>
#include <vector>


#include <Windows.h>

#include "codec_registration.hpp"

// dialog comtrols
enum : int32_t {
	OUT_noUI = -1,
	OUT_OK = IDOK,
	OUT_Cancel = IDCANCEL,
    OUT_SubTypes_Menu = 3,
    OUT_SubTypes_Label = 4,
    OUT_Quality_Menu = 5,
    OUT_Quality_Label = 6,
    OUT_ChunkCount_Menu = 7,
    OUT_ChunkCount_Label = 8
};


// globals
HINSTANCE hDllInstance = NULL;

static WORD	g_item_clicked = 0;

static LRESULT g_SubType = 0;
static LRESULT g_Quality = 0;
static LRESULT g_ChunkCount = 0;

static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{
    switch (message) 
    { 
		case WM_INITDIALOG:
			do{
                const auto& codec = *CodecRegistry::codec();
                {
                    HWND menu = GetDlgItem(hwndDlg, OUT_SubTypes_Menu);
                    bool hasSubTypes(codec.details().subtypes.size() > 0);
                    if (hasSubTypes) {
                        // set up the menu
                        auto subTypes = codec.details().subtypes;
                        auto subType = subTypes.begin();

                        for (int i = 0; i < subTypes.size(); ++i, ++subType)
                        {
                            SendMessage(menu, (UINT)CB_ADDSTRING, (WPARAM)wParam, (LPARAM)(LPCTSTR)subType->second.c_str());
                            DWORD subTypeVal = reinterpret_cast<DWORD&>(subType->first);
                            SendMessage(menu, (UINT)CB_SETITEMDATA, (WPARAM)i, (LPARAM)subTypeVal); // subtype fourcc

                            if (subTypeVal == g_SubType)
                                SendMessage(menu, CB_SETCURSEL, (WPARAM)i, (LPARAM)0);
                        }
                    }
                    else
                    {
                        // do not show subtypes item or label
                        ShowWindow(menu, SW_HIDE);
                        HWND label = GetDlgItem(hwndDlg, OUT_SubTypes_Label);
                        ShowWindow(label, SW_HIDE);
                    }
                }

                {
                    HWND menu = GetDlgItem(hwndDlg, OUT_Quality_Menu);
                    if (codec.details().quality.hasQualityForAnySubType)
                    {
                        // set up the menu
                        auto qualities = codec.details().quality.descriptions;
                        auto quality = qualities.begin();

                        for (int i = 0; i < qualities.size(); ++i, ++quality)
                        {
                            SendMessage(menu, (UINT)CB_ADDSTRING, (WPARAM)wParam, (LPARAM)(LPCTSTR)quality->second.c_str());
                            SendMessage(menu, (UINT)CB_SETITEMDATA, (WPARAM)i, (LPARAM)(DWORD)quality->first); // this is the quality enum

                            if (quality->first == g_Quality)
                                SendMessage(menu, CB_SETCURSEL, (WPARAM)i, (LPARAM)0);
                        }

                        // enable / disable depending upon selected codec subtype
                        auto enabledForSelectedCodecSubtype = codec.details().hasQualityForSubType(reinterpret_cast<Codec4CC&>(g_SubType));
                        EnableWindow( menu, enabledForSelectedCodecSubtype ? TRUE : FALSE );
                    }
                    else
                    {
                        // do not show qualities item
                        ShowWindow(menu, SW_HIDE);
                    }
                }

                {
                    HWND menu = GetDlgItem(hwndDlg, OUT_ChunkCount_Menu);
                    if (codec.details().hasChunkCount)
                    {
                        // set up the menu
                        auto descriptions = std::array<std::pair<int, std::string>, 9>{
                            { {0, "Auto"}, {1, "1"}, {2, "2"}, { 3, "3"}, { 4, "4"},
                            { 5, "5"}, { 6, "6"}, { 7, "7" }, { 8, "8" }
                        } };
                        auto description = descriptions.begin();

                        for (int i = 0; i < descriptions.size(); ++i, ++description)
                        {
                            SendMessage(menu, (UINT)CB_ADDSTRING, (WPARAM)wParam, (LPARAM)(LPCTSTR)description->second.c_str());
                            SendMessage(menu, (UINT)CB_SETITEMDATA, (WPARAM)i, (LPARAM)(DWORD)description->first);

                            if (description->first == g_ChunkCount) {
                                SendMessage(menu, CB_SETCURSEL, (WPARAM)i, (LPARAM)0);
                            }
                        }
                    }
                    else
                    {
                        // do not show qualities item
                        ShowWindow(menu, SW_HIDE);
                        HWND label = GetDlgItem(hwndDlg, OUT_ChunkCount_Label);
                        ShowWindow(label, SW_HIDE);
                    }
                }
            } while(0);


			return TRUE;
 

        case WM_COMMAND: 
			g_item_clicked = LOWORD(wParam);

            switch (LOWORD(wParam)) 
            { 
                case OUT_OK: 
				case OUT_Cancel:  // do the same thing, but g_item_clicked will be different
					do{
                        // subType
                        const auto& codec = *CodecRegistry::codec();

                        bool hasSubTypes(codec.details().subtypes.size() > 0);
                        if (hasSubTypes) {
                            HWND menu = GetDlgItem(hwndDlg, OUT_SubTypes_Menu);

                            LRESULT cur_sel = SendMessage(menu, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                            g_SubType = SendMessage(menu, (UINT)CB_GETITEMDATA, (WPARAM)cur_sel, (LPARAM)0);
                        }

                        // quality
                        if (codec.details().quality.hasQualityForAnySubType)
                        {
                            HWND menu = GetDlgItem(hwndDlg, OUT_Quality_Menu);

                            // get the channel index associated with the selected menu item
                            LRESULT cur_sel = SendMessage(menu, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                            g_Quality = SendMessage(menu, (UINT)CB_GETITEMDATA, (WPARAM)cur_sel, (LPARAM)0);
                        }

                        // chunk count
                        if (codec.details().hasChunkCount)
                        {
                            HWND menu = GetDlgItem(hwndDlg, OUT_ChunkCount_Menu);

                            LRESULT cur_sel = SendMessage(menu, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                            g_ChunkCount = SendMessage(menu, (UINT)CB_GETITEMDATA, (WPARAM)cur_sel, (LPARAM)0);
                        }
					}while(0);

					//PostMessage((HWND)hwndDlg, WM_QUIT, (WPARAM)WA_ACTIVE, lParam);
					EndDialog(hwndDlg, 0);
                    //DestroyWindow(hwndDlg); 
                    return TRUE;
            } 
    } 
    return FALSE; 
} 

bool
ui_OutDialog(Codec4CC& subType, int& quality, int &chunkCount, void *platformSpecific)
{
	// set globals
    g_SubType = reinterpret_cast<DWORD&>(subType);
	g_Quality = quality;
    g_ChunkCount = chunkCount;

    // do dialog
    HWND* hwndOwner = (HWND *)platformSpecific;
    DialogBox(hDllInstance, (LPSTR)"OUTDIALOG", *hwndOwner, (DLGPROC)DialogProc);

	if(g_item_clicked == OUT_OK)
	{
        const auto& codec = *CodecRegistry::codec();

        bool hasSubTypes = (codec.details().subtypes.size() > 0);
        if (hasSubTypes)
            subType = reinterpret_cast<Codec4CC&>(g_SubType);

        if (codec.details().quality.hasQualityForAnySubType)
            quality = (int)g_Quality;
		
        if (codec.details().hasChunkCount)
            chunkCount = (int)g_ChunkCount;

        return true;
	}
	else
		return false;
}

BOOL WINAPI DllMain(HANDLE hInstance, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		hDllInstance = (HINSTANCE)hInstance;

	return TRUE;   // Indicate that the DLL was initialized successfully.
}
