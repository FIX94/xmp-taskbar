/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <windows.h>
#include <shobjidl.h>
#include <cstdio>
#include "xmpdsp.h"

static XMPFUNC_MISC *xmpfmisc;
static XMPFUNC_STATUS *xmpfstatus;
static ITaskbarList3* m_pTaskBarlist = NULL;
static UINT_PTR timer_ptr = NULL;
static HWND xmpwin;

static void WINAPI DSP_About(HWND win);
static void *WINAPI DSP_New(void);
static void WINAPI DSP_Free(void *inst);
static const char *WINAPI DSP_GetDescription(void *inst);
static DWORD WINAPI DSP_GetConfig(void *inst, void *config);
static BOOL WINAPI DSP_SetConfig(void *inst, void *config, DWORD size);

static XMPDSP dsp = {
    XMPDSP_FLAG_NODSP,
    "Taskbar Progress",
    DSP_About,
    DSP_New,
    DSP_Free,
    DSP_GetDescription,
    NULL,
    DSP_GetConfig,
    DSP_SetConfig,
};

static void WINAPI DSP_About(HWND win)
{
	char mBoxChar[256];
	sprintf(mBoxChar, "Taskbar Progress v0.1 for XMPlay\n"
		"Copyright (C) 2016 FIX94\n"
		"Built: %s %s", __DATE__, __TIME__);
	MessageBox(win,
		mBoxChar,
		"About Plugin",
		MB_ICONINFORMATION);
}

static const char *WINAPI DSP_GetDescription(void *inst)
{
    return dsp.name;
}

VOID CALLBACK updateTaskbar(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	//SendMessage seems to be reliable when it comes to subsongs
	int cPosMs = SendMessage(xmpfmisc->GetWindow(), WM_USER, 0, IPC_GETOUTPUTTIME);
	int lTotal = SendMessage(xmpfmisc->GetWindow(), WM_USER, 1, IPC_GETOUTPUTTIME) * 1000;
	if (m_pTaskBarlist)
	{	//actual isPlaying function does not return pause status so use SendMessage
		if (SendMessage(xmpfmisc->GetWindow(), WM_USER, 0, IPC_ISPLAYING) == 3)
			m_pTaskBarlist->SetProgressState(hwnd, TBPF_PAUSED);
		else //everything except 3 for us means that its playing
			m_pTaskBarlist->SetProgressState(hwnd, TBPF_INDETERMINATE);
		m_pTaskBarlist->SetProgressValue(hwnd, cPosMs, lTotal);
	}
}

static void *WINAPI DSP_New()
{
	//Get Main Window for Updates
	xmpwin=xmpfmisc->GetWindow();
	//Create TaskbarList Instance
	CoCreateInstance(
		CLSID_TaskbarList, NULL, CLSCTX_ALL,
		IID_ITaskbarList3, (void**)&m_pTaskBarlist);
	//If everything went well set start value
	if (m_pTaskBarlist)
	{
		m_pTaskBarlist->SetProgressState(xmpwin, TBPF_INDETERMINATE);
		m_pTaskBarlist->SetProgressValue(xmpwin, 0, 1);
	}
	//hook up a timer to the XMPlay window
	timer_ptr = SetTimer(xmpwin, 0, 50, updateTaskbar);

    return (void*)1;
}

static void WINAPI DSP_Free(void *inst)
{
	if (m_pTaskBarlist)
	{
		m_pTaskBarlist->SetProgressState(xmpwin, TBPF_NOPROGRESS);
		m_pTaskBarlist->Release();
	}
	m_pTaskBarlist = NULL;
	if (timer_ptr)
	{
		KillTimer(xmpwin, timer_ptr);
		timer_ptr = NULL;
	}
}

// we dont have any config so these are just empty
static DWORD WINAPI DSP_GetConfig(void *inst, void *config)
{
	return 0;
}

static BOOL WINAPI DSP_SetConfig(void *inst, void *config, DWORD size)
{
    return TRUE;
}

// get the plugin's XMPDSP interface
XMPDSP *WINAPI XMPDSP_GetInterface2(DWORD face, InterfaceProc faceproc)
{
	if (face != XMPDSP_FACE) return NULL;
	xmpfmisc = (XMPFUNC_MISC*)faceproc(XMPFUNC_MISC_FACE); // import "misc" functions
	xmpfstatus = (XMPFUNC_STATUS*)faceproc(XMPFUNC_STATUS_FACE); // import "status" functions
	return &dsp;
}

BOOL WINAPI DllMain(HINSTANCE hDLL, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(hDLL);
    return 1;
}
