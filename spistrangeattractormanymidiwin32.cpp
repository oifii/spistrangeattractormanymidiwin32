/*
 * Copyright (c) 2010-2016 Stephane Poirier
 *
 * stephane.poirier@oifii.org
 *
 * Stephane Poirier
 * 3532 rue Ste-Famille, #3
 * Montreal, QC, H2X 2L1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// spistrangeattractormanymidiwin32.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "spistrangeattractormanymidiwin32.h"
#include <mmsystem.h> //for timeSetEvent()
/*
#include <iostream>
#include <fstream>
*/
#include <string>
#include <vector>
using namespace std;
/*
#include "oifiilib.h" //note: oifiilib.lib/.dll is an MFC extension and resource DLL
*/

#include <list>
#include "portmidi.h"
#include <map>
#include <stdio.h>
#include "resource.h"

#include  <GL/gl.h>
#include <GL/glu.h>
#include "glfont.h"
#include "spiwavsetlib.h"
#include <assert.h>

//#define MAX_LOADSTRING 100
#define MAX_LOADSTRING 1024
#define NOTEREMAP_NONE				0
#define NOTEREMAP_C_MINORPENTATONIC	1
#define NOTEREMAP_C3				2

// Global Variables:
HINSTANCE hInst;								// current instance
//TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
//TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR szTitle[1024]={"spistrangeattractormanymidiwin32title"};					// The title bar text
TCHAR szWindowClass[1024]={"spistrangeattractormanymidiwin32class"};			// the main window class name

//new parameters
int global_idcolor=0;
string global_begin="begin.ahk";
string global_end="end.ahk";

int global_titlebardisplay=1; //0 for off, 1 for on
int global_acceleratoractive=0; //0 for off, 1 for on
int global_menubardisplay=0; //0 for off, 1 for on

DWORD global_startstamp_ms;
float global_duration_sec=3600;
//float global_sleeptimeperstrangeattractor_sec=1;
float global_sleeptimeperstrangeattractor_sec=30;
int global_x=100;
int global_y=100;
int global_xwidth=300;
int global_yheight=300;
BYTE global_alpha=220;

int global_imageid=0;
HWND global_hwnd=NULL;
MMRESULT global_timer=1; //was 0
int global_imageheight=-1; //will be computed within WM_SIZE handler
int global_imagewidth=-1; //will be computed within WM_SIZE handler 
/*
vector<string> global_txtfilenames;
COW2Doc* global_pOW2Doc=NULL;
COW2View* global_pOW2View=NULL;
*/

const int global_pmeventlistsize = 64;
list<PmEvent*> global_pmeventlist[global_pmeventlistsize]; 
list<PmEvent*>::iterator it_pmeventlist;
bool global_pmevenlistbusyfrommiditimer[global_pmeventlistsize];
bool global_pmevenlistbusyfromglobaltimer[global_pmeventlistsize];
bool global_miditimerskip=false;
UINT global_miditimer=2;
UINT global_miditimer_programchange=3;
int global_prevstep=0; //was -1
int global_midistep_ms=250;
//int global_midistep_ms=125;
int global_midiprogramchangeperiod_ms=1000*3*60;
FILE* pFILE = NULL;

map<string,int> global_midioutputdevicemap;
string global_midioutputdevicename="Out To MIDI Yoke:  1";
//string global_midioutputdevicename="E-DSP MIDI Port [FFC0]";
//string global_midioutputdevicename="E-DSP MIDI Port 2 [FFC0]";
int global_outputmidichannel=0;
int global_midicontrolnumber=9; //9 is undefined, so it is available for us to use
bool global_bsendmidi=true;
bool global_bsendmidi_usingremap=true;
PmStream* global_pPmStream = NULL; // midi output
string global_noteremapstring="NONE";
//int global_noteremapid;

bool global_bdrawlabel=true;
GLFont global_GLFont;
float x = 0.1, y = 0.1;		// starting point
float a = -0.966918;		// coefficients for "The King's Dream"
float b = 2.879879;			// coefficients a and b will be modified at random between a range of -3.0 to + 3.0 
float c = 0.765145;
float d = 0.744728;
int	global_count=0;
int global_axiscoef=1;
HDC global_hDC; //returned by opengl InitGL()


int	initialIterations = 100,	// initial number of iterations
					// to allow the attractor to settle
	iterations = 10000;		// number of times to iterate through
	//iterations = 100000;		// number of times to iterate through
	//iterations = 1000000;		// number of times to iterate through
					// the functions and draw a point

class glfloatpair
{
public:
	GLfloat x;
	GLfloat y;

	glfloatpair(GLfloat xx, GLfloat yy)
	{
		x = xx;
		y = yy;
	}
	glfloatpair(double xx, double yy)
	{
		x = xx;
		y = yy;
	}
};

std::vector<glfloatpair*> global_pairvector;
std::vector<glfloatpair*>::iterator it;


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
void OnTimerMidi();
void OnTimerMidiProgramChange();
void InitGL(HWND hWnd, HDC & hDC, HGLRC & hRC);
void CloseGL(HWND hWnd, HDC hDC, HGLRC hRC);
void SetupAnimation(HDC hDC, int Width, int Height);
void CleanupAnimation();



int RandomInteger(int lowest, int highest) 
{
	int random_integer;
	int range=(highest-lowest)+1; 
	//random_integer = lowest+int(range*rand()/(RAND_MAX + 1.0));
	random_integer = lowest+rand()%range;
	return random_integer;
}

float RandomFloat(float a, float b) 
{
    float random = ((float) rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring &wstr)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo( size_needed, 0 );
    WideCharToMultiByte                  (CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Convert an UTF8 string to a wide Unicode String
std::wstring utf8_decode(const std::string &str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo( size_needed, 0 );
    MultiByteToWideChar                  (CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}


void CALLBACK StartGlobalProcess(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	//WavSetLib_Initialize(global_hwnd, IDC_MAIN_STATIC, global_staticwidth, global_staticheight, global_fontwidth, global_fontheight, global_staticalignment);
	//HDC* pHDC=(HDC*)dwUser;

	DWORD nowstamp_ms = GetTickCount();
	while( (global_duration_sec<0.0f) || ((nowstamp_ms-global_startstamp_ms)/1000.0f)<global_duration_sec )
	{
		InvalidateRect(global_hwnd, NULL, false);

		//create midi events, various notes with various durations
		if(global_bsendmidi)
		{
			//global_miditimerskip=true;
			global_miditimerskip=false;
			for(int k=0; k<global_pmeventlistsize; k++)
			{
				//for testing
				if(k%4!=0) continue;
				//if(pFILE) fprintf(pFILE, "k=%d\n",k);
				//constant note 
				//int=note=60;
				//climbing note
				//int note=60+k;
				//random note
				//int note = RandomInteger(0, 127);
				//strange attractor note
				// compute a new point using the strange attractor equations
				float xnew = sin(y*b) + c*sin(x*b);
				float ynew = sin(x*a) + d*sin(y*a);
				// save the new point
				x = xnew;
				y = ynew;
 				int note = (x/2.0f)*64+64;
				if(note>=128) note=127;
				if(note<=0) note=0;
				//int velocity=100;
				int velocity = (y/2.0f)*64+64;
				if(velocity>=128) velocity=127;
				if(velocity<=0) velocity=0;
				/*
				if(global_noteremapid==NOTEREMAP_NONE)
				{
					//do nothing
				}
				else if(global_noteremapid==NOTEREMAP_C_MINORPENTATONIC)
				{
					//remap note
					note = GetNoteRemap_C_MinorPentatonic(note);
				}
				else if(global_noteremapid==NOTEREMAP_C3)
				{
					//remap note
					note = GetNoteRemap_C3(note);
				}
				*/
				note = GetNoteRemap(global_noteremapstring.c_str(), note);

				//note on
				PmEvent* pPmEvent = new PmEvent;
				pPmEvent->timestamp = 0;
				pPmEvent->message = Pm_Message(0x90+global_outputmidichannel, note, velocity);
				while(global_pmevenlistbusyfrommiditimer[k]==true) Sleep(10);
				global_pmevenlistbusyfromglobaltimer[k]=true;
				global_pmeventlist[k].push_back(pPmEvent);
				global_pmevenlistbusyfromglobaltimer[k]=false;
				//duration
				//int random_integer = 4;
				int random_integer = RandomInteger(1,8);
				int kk=k+random_integer;
				if(kk>(global_pmeventlistsize-1)) kk=kk-global_pmeventlistsize;
				//create note off midi message
				pPmEvent = new PmEvent;
				pPmEvent->timestamp = 0;
				pPmEvent->message = Pm_Message(0x90+global_outputmidichannel, note, 0);
				while(global_pmevenlistbusyfrommiditimer[kk]==true) Sleep(10);
				global_pmevenlistbusyfromglobaltimer[kk]=true;
				global_pmeventlist[kk].push_back(pPmEvent);
				global_pmevenlistbusyfromglobaltimer[kk]=false;

			}
			global_miditimerskip=false;
		}
		Sleep((int)(global_sleeptimeperstrangeattractor_sec*1000));

		nowstamp_ms = GetTickCount();
	}
	PostMessage(global_hwnd, WM_DESTROY, 0, 0);
}




PCHAR*
    CommandLineToArgvA(
        PCHAR CmdLine,
        int* _argc
        )
    {
        PCHAR* argv;
        PCHAR  _argv;
        ULONG   len;
        ULONG   argc;
        CHAR   a;
        ULONG   i, j;

        BOOLEAN  in_QM;
        BOOLEAN  in_TEXT;
        BOOLEAN  in_SPACE;

        len = strlen(CmdLine);
        i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

        argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
            i + (len+2)*sizeof(CHAR));

        _argv = (PCHAR)(((PUCHAR)argv)+i);

        argc = 0;
        argv[argc] = _argv;
        in_QM = FALSE;
        in_TEXT = FALSE;
        in_SPACE = TRUE;
        i = 0;
        j = 0;

        while( a = CmdLine[i] ) {
            if(in_QM) {
                if(a == '\"') {
                    in_QM = FALSE;
                } else {
                    _argv[j] = a;
                    j++;
                }
            } else {
                switch(a) {
                case '\"':
                    in_QM = TRUE;
                    in_TEXT = TRUE;
                    if(in_SPACE) {
                        argv[argc] = _argv+j;
                        argc++;
                    }
                    in_SPACE = FALSE;
                    break;
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    if(in_TEXT) {
                        _argv[j] = '\0';
                        j++;
                    }
                    in_TEXT = FALSE;
                    in_SPACE = TRUE;
                    break;
                default:
                    in_TEXT = TRUE;
                    if(in_SPACE) {
                        argv[argc] = _argv+j;
                        argc++;
                    }
                    _argv[j] = a;
                    j++;
                    in_SPACE = FALSE;
                    break;
                }
            }
            i++;
        }
        _argv[j] = '\0';
        argv[argc] = NULL;

        (*_argc) = argc;
        return argv;
    }


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	global_startstamp_ms = GetTickCount();

	//LPWSTR *szArgList;
	LPSTR *szArgList;
	int nArgs;
	//szArgList = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	szArgList = CommandLineToArgvA(GetCommandLineA(), &nArgs);
	if( NULL == szArgList )
	{
		//wprintf(L"CommandLineToArgvA failed\n");
		return FALSE;
	}
	LPWSTR *szArgListW;
	int nArgsW;
	szArgListW = CommandLineToArgvW(GetCommandLineW(), &nArgsW);
	if( NULL == szArgListW )
	{
		//wprintf(L"CommandLineToArgvW failed\n");
		return FALSE;
	}
	if(nArgs>1)
	{
		global_midioutputdevicename = szArgList[1];
	}
	if(nArgs>2)
	{
		global_outputmidichannel = atoi(szArgList[2]);
	}
	if(nArgs>3)
	{
		global_duration_sec = atof(szArgList[3]);
	}
	if(nArgs>4)
	{
		global_sleeptimeperstrangeattractor_sec = atof(szArgList[4]);
	}
	if(nArgs>5)
	{
		global_midistep_ms = atoi(szArgList[5]);
	}
	if(nArgs>6)
	{
		global_midiprogramchangeperiod_ms = atoi(szArgList[6]);
	}
	if(nArgs>7)
	{
		global_noteremapstring = szArgList[7];
	}
	if(nArgs>8)
	{
		global_x = atoi(szArgList[8]);
	}
	if(nArgs>9)
	{
		global_y = atoi(szArgList[9]);
	}
	if(nArgs>10)
	{
		global_xwidth = atoi(szArgList[10]);
	}
	if(nArgs>11)
	{
		global_yheight = atoi(szArgList[11]);
	}
	if(nArgs>12)
	{
		global_alpha = atoi(szArgList[12]);
	}
	if(nArgs>13)
	{
		global_titlebardisplay = atoi(szArgList[13]);
	}
	if(nArgs>14)
	{
		global_menubardisplay = atoi(szArgList[14]);
	}
	if(nArgs>15)
	{
		global_acceleratoractive = atoi(szArgList[15]);
	}
	if(nArgs>16)
	{
		global_idcolor = atoi(szArgList[16]);
	}
	if(nArgs>17)
	{
		strcpy(szWindowClass, szArgList[17]); 
	}
	if(nArgs>18)
	{
		strcpy(szTitle, szArgList[18]); 
	}
	if(nArgs>19)
	{
		global_begin = szArgList[19]; 
	}
	if(nArgs>20)
	{
		global_end = szArgList[20]; 
	}
	LocalFree(szArgList);
	LocalFree(szArgListW);

	int nShowCmd = false;
	//ShellExecuteA(NULL, "open", "begin.bat", "", NULL, nShowCmd);
	ShellExecuteA(NULL, "open", global_begin.c_str(), "", NULL, nShowCmd);

	pFILE = fopen("debug.txt", "w");
	//parameter validation
	if(global_outputmidichannel<0 || global_outputmidichannel>15)
	{
		if(pFILE) fprintf(pFILE, "invalid outputmidichannel, midi channel must range from 0 to 15 inclusively.\n");
		return FALSE;
	}
	/*
	if(global_noteremapstring!="NONE" && 
		global_noteremapstring!="C_MINORPENTATONIC" &&
		global_noteremapstring!="C3")
	{
		if(pFILE) fprintf(pFILE, "invalid noteremapstring, noteremapstring must either be NONE, C_MINORPENTATONIC or C3.\n");
		return FALSE;
	}
	if(global_noteremapstring=="NONE")
	{
		global_noteremapid=NOTEREMAP_NONE;
	}
	else if(global_noteremapstring=="C_MINORPENTATONIC")
	{
		global_noteremapid=NOTEREMAP_C_MINORPENTATONIC;
	}
	else if(global_noteremapstring=="C3")
	{
		global_noteremapid=NOTEREMAP_C3;
	}
	else
	{
		assert(false);
		if(pFILE) fprintf(pFILE, "invalid noteremapstring, noteremapstring must either be NONE, C_MINORPENTATONIC or C3.\n");
		return FALSE;
	}
	*/

	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	//LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	//LoadString(hInstance, IDC_SPISTRANGEATTRACTORMANYMIDIWIN32, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	if(global_acceleratoractive)
	{
		hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SPISTRANGEATTRACTORMANYMIDIWIN32));
	}
	else
	{
		hAccelTable = NULL;
	}


	//////////////////////////
	//initialize random number
	//////////////////////////
	srand(GetTickCount());


	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SPISTRANGEATTRACTORMANYMIDIWIN32));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);

	if(global_menubardisplay)
	{
		wcex.lpszMenuName = MAKEINTRESOURCE(IDC_SPISTRANGEATTRACTORMANYMIDIWIN32);
	}
	else
	{
		wcex.lpszMenuName = NULL; //no menu
	}
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

	//global_hFont=CreateFontW(global_fontheight,0,0,0,FW_NORMAL,0,0,0,0,0,0,2,0,L"SYSTEM_FIXED_FONT");
	//global_hFont=CreateFontW(global_fontheight,0,0,0,FW_NORMAL,0,0,0,0,0,0,2,0,L"Segoe Script");

	if(global_titlebardisplay)
	{
		hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, //original with WS_CAPTION etc.
			global_x, global_y, global_xwidth, global_yheight, NULL, NULL, hInstance, NULL);
	}
	else
	{
		hWnd = CreateWindow(szWindowClass, szTitle, WS_POPUP | WS_VISIBLE, //no WS_CAPTION etc.
			global_x, global_y, global_xwidth, global_yheight, NULL, NULL, hInstance, NULL);
	}
	if (!hWnd)
	{
		return FALSE;
	}
	global_hwnd = hWnd;

	SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(hWnd, 0, global_alpha, LWA_ALPHA);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	//global_timer=timeSetEvent(100,25,(LPTIMECALLBACK)&StartGlobalProcess,0,TIME_ONESHOT);
	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	//static HDC hDC; //returned by opengl InitGL()
	static HGLRC hRC; //returned by opengl InitGL()

	switch (message)
	{
	case WM_CREATE:
		{
			/*
			//0) execute cmd line to get all folder's .bmp filenames
			string quote = "\"";
			string pathfilter;
			string path=global_imagefolder;
			pathfilter = path + "\\*.bmp";
			//pathfilter = path + "\\*.jpg";
			string systemcommand;
			//systemcommand = "DIR " + quote + pathfilter + quote + "/B /O:N > wsic_filenames.txt"; //wsip tag standing for wav set (library) instrumentset (class) populate (function)
			systemcommand = "DIR " + quote + pathfilter + quote + "/B /S /O:N > spivoronoi_filenames.txt"; // /S for adding path into "wsic_filenames.txt"
			system(systemcommand.c_str());

			//2) load in all "spiss_filenames.txt" file
			//vector<string> global_txtfilenames;
			ifstream ifs("spivoronoi_filenames.txt");
			string temp;
			while(getline(ifs,temp))
			{
				//txtfilenames.push_back(path + "\\" + temp);
				global_txtfilenames.push_back(temp);
			}
			*/
			// setup OpenGL, then animation
			//InitGL( hWnd, hDC, hRC );  
			InitGL( hWnd, global_hDC, hRC );  
			//SetupAnimation(hDC, global_xwidth, global_yheight);
			SetupAnimation(global_hDC, global_xwidth, global_yheight);

			//3) initialize portmidi
			if(global_bsendmidi)
			{
				/////////////////////////
				//portmidi initialization
				/////////////////////////
				PmError err;
				Pm_Initialize();
				// list device information 
				if(pFILE) fprintf(pFILE, "MIDI output devices:\n");
				for (int i = 0; i < Pm_CountDevices(); i++) 
				{
					const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
					if (info->output) 
					{
						if(pFILE) fprintf(pFILE, "%d: %s, %s\n", i, info->interf, info->name);
						string devicename = info->name;
						global_midioutputdevicemap.insert(pair<string,int>(devicename,i));
					}
				}
				int midioutputdeviceid = 13;
				map<string,int>::iterator it;
				it = global_midioutputdevicemap.find(global_midioutputdevicename);
				if(it!=global_midioutputdevicemap.end())
				{
					midioutputdeviceid = (*it).second;
					if(pFILE) fprintf(pFILE, "%s maps to %d\n", global_midioutputdevicename.c_str(), midioutputdeviceid);
				}
				if(pFILE) fprintf(pFILE, "device %d selected\n", midioutputdeviceid);
				//err = Pm_OpenInput(&midi_in, inp, NULL, 512, NULL, NULL);
				err = Pm_OpenOutput(&global_pPmStream, midioutputdeviceid, NULL, 512, NULL, NULL, 0); //0 latency
				if (err) 
				{
					if(pFILE) fprintf(pFILE, Pm_GetErrorText(err));
					//Pt_Stop();
					//Terminate();
					//mmexit(1);
					global_bsendmidi = false;
				}
				//2.5)
				for(int k=0; k<global_pmeventlistsize; k++)
				{
					global_pmevenlistbusyfrommiditimer[k]=false;
					global_pmevenlistbusyfromglobaltimer[k]=false;
				}
				/*
				//3) set midi timers
				SetTimer( hWnd, global_miditimer, global_midistep_ms, NULL );
				SetTimer( hWnd, global_miditimer_programchange, global_midiprogramchangeperiod_ms, NULL );
				*/
			}

			//start timers
			//global_timer=timeSetEvent(100,25,(LPTIMECALLBACK)&StartGlobalProcess,(DWORD_PTR)(&hDC),TIME_ONESHOT);
			global_timer=timeSetEvent(1,25,(LPTIMECALLBACK)&StartGlobalProcess,0,TIME_ONESHOT);
			if(global_bsendmidi)
			{
				//3) set midi timers
				SetTimer( hWnd, global_miditimer, global_midistep_ms, NULL );
				SetTimer( hWnd, global_miditimer_programchange, global_midiprogramchangeperiod_ms, NULL );
			}
		}
		break;
	case WM_SIZE:
		{
			RECT rcClient;
			GetClientRect(hWnd, &rcClient);

			global_imagewidth = rcClient.right - 0;
			global_imageheight = rcClient.bottom - 0; 

			//window resizing stuff
			glViewport(0, 0, (GLsizei) global_imagewidth, (GLsizei) global_imageheight);

		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
    case WM_ERASEBKGND:
		{
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);

		char buffer[100];
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* //no rotation
        spin = spin + 1; 
		*/
        glPushMatrix();
        /*
		glRotatef(spin, 0.0, 0.0, 1.0);
		*/
        glPushMatrix();
		

		global_pairvector.clear();

		// draw some points
		glBegin(GL_POINTS);
 
			// go through the equations many times, drawing a point for each iteration
			//for (int i = 0; i &lt; iterations; i++) 
			for (int i = 0; i<iterations; i++) 
			{
 
				// compute a new point using the strange attractor equations
				float xnew = sin(y*b) + c*sin(x*b);
				float ynew = sin(x*a) + d*sin(y*a);
 
				// save the new point
				x = xnew;
				y = ynew;
 
				// draw the new point
				glVertex3f(x, y, 0.0f);
			}
 
		glEnd();

		//add last pair to vector
		global_pairvector.push_back(new glfloatpair(x,y));

		if(global_bdrawlabel)
		{
			glEnable(GL_TEXTURE_2D);
			//glColor4f(1.0f, 0.0f, 0.0f, 1.0f); //red
			//glColor4f(0.0f, 0.0f, 1.0f, 1.0f); //blue
			glScalef(0.02, 0.02, 0.02);
			for (int ii = 0; ii<1; ii++) 
			//for (int ii = 0; ii<iterations; ii++) 
			//for (int ii = 0; ii<10; ii++) 
			{
 
				// compute a new point using the strange attractor equations
				//float xnew = sin(y*b) + c*sin(x*b);
				//float ynew = sin(x*a) + d*sin(y*a);
 
				// save the new point
				//x = xnew;
				//y = ynew;
 				//spi, begin
				GLfloat xlabel = (global_pairvector[ii])->x;
				GLfloat ylabel = (global_pairvector[ii])->y; 

				//sprintf_s(buffer, 100-1, "%d", ii); 
				sprintf_s(buffer, 100-1, "+", ii); 
				global_GLFont.TextOut(buffer, xlabel/0.02,ylabel/0.02,0);
				//glColor4f(1.0f, 1.0f, 1.0f, 1.0f); //white
				//glScalef(1.0, 1.0, 1.0);
				//spi, end
				//glFlush();
			}
			glDisable(GL_TEXTURE_2D);
			//glColor4f(1.0f, 1.0f, 1.0f, 1.0f); //white
			glScalef(1.0, 1.0, 1.0);
		}

        glPopMatrix();

        glFlush();
        SwapBuffers(hdc);
		//SwapBuffers(global_hDC);
        glPopMatrix();
		
		/*
		//change coefficients a and b for next blit
		a = RandomFloat(-3.0, 3.0);
		b = RandomFloat(-3.0, 3.0);
		//change coefficients c and d for next blit
		c = RandomFloat(-0.5, 1.0);
		d = RandomFloat(-0.5, 1.0);
		*/
		/*
		//change only one coefficient gradually along one axis
		a = a+0.1;
		if(a>3.0) a=-3.0;
		b = b+0.1;
		if(b>3.0) b=-3.0;
		c = c+0.1;
		if(c>1.0) c=-0.5;
		d = d+0.1;
		if(d>1.0) d=-0.5;
		*/
		/*
		a = a+0.1;
		if(a>3.0) a=-3.0;
		c = c+0.1;
		if(c>1.0) c=-0.5;
		*/
		switch(global_axiscoef)
		{
		case 1:
			a = a+0.1;
			if(a>3.0) 
			{
				a=-3.0;
				global_count++;
				if(global_count==2) 
				{
					global_count=0;
					a = -0.966918;		// coefficients for "The King's Dream"
					b = 2.879879;		
					c = 0.765145;
					d = 0.744728;
					global_axiscoef=RandomInteger(1,4);
				}
			}
			break;
		case 2:
			b = b+0.1;
			if(b>3.0) 
			{
				b=-3.0;
				global_count++;
				if(global_count==2) 
				{
					global_count=0;
					a = -0.966918;		// coefficients for "The King's Dream"
					b = 2.879879;		
					c = 0.765145;
					d = 0.744728;
					global_axiscoef=RandomInteger(1,4);
				}
			}
			break;
		case 3:
			c = c+0.1;
			if(c>1.0) 
			{
				c=-0.5;
				global_count++;
				if(global_count==2) 
				{
					global_count=0;
					a = -0.966918;		// coefficients for "The King's Dream"
					b = 2.879879;		
					c = 0.765145;
					d = 0.744728;
					global_axiscoef=RandomInteger(1,4);
				}
			}
			break;
		case 4:
			d = d+0.1;
			if(d>1.0) 
			{
				d=-0.5;
				global_count++;
				if(global_count==2) 
				{
					global_count=0;
					a = -0.966918;		// coefficients for "The King's Dream"
					b = 2.879879;		
					c = 0.765145;
					d = 0.744728;
					global_axiscoef=RandomInteger(1,4);
				}
			}
			break;
		}
		//reset position for net blit
		x = 0.1;
		y = 0.1;

		EndPaint(hWnd, &ps);
		break;
	  case WM_TIMER:
		  if(wParam==global_miditimer)
		  {
			  OnTimerMidi();
		  }
		  else if(wParam==global_miditimer_programchange)
		  {
			  OnTimerMidiProgramChange();
		  }
		return 0;                           

	case WM_DESTROY:
		{
			if (global_timer) timeKillEvent(global_timer);
			/*
			if(global_pOW2Doc) delete global_pOW2Doc;
			if(global_pOW2View) delete global_pOW2View;
			*/
			for(int k=0; k<global_pmeventlistsize; k++)
			{
				for (it_pmeventlist = global_pmeventlist[k].begin(); it_pmeventlist != global_pmeventlist[k].end(); it_pmeventlist++)
				{
					if(*it_pmeventlist) delete *it_pmeventlist;
				}
				global_pmeventlist[k].clear();
			}
			CleanupAnimation();
			//CloseGL( hWnd, hDC, hRC );
			CloseGL( hWnd, global_hDC, hRC );
			if(pFILE) fclose(pFILE);
			//erase vector
			for(it=global_pairvector.begin(); it!=global_pairvector.end(); it++)
			{
				if(*it) delete *it;
			}
			global_pairvector.clear();

			if(global_bsendmidi)
			{
				KillTimer(hWnd, global_miditimer);
				KillTimer(hWnd, global_miditimer_programchange);
				if(global_pPmStream) 
				{
					//send all note off
					for(int k=0; k<128; k++)
					{
						PmEvent myPmEvent;
						myPmEvent.timestamp = 0;
						myPmEvent.message = Pm_Message(0x90+global_outputmidichannel, k, 0);
						//send midi event
						Pm_Write(global_pPmStream, &myPmEvent, 1);
					}
					Pm_Close(global_pPmStream);
				}
				//Pt_Stop();
				Pm_Terminate();
			}
			int nShowCmd = false;
			//ShellExecuteA(NULL, "open", "end.bat", "", NULL, nShowCmd);
			ShellExecuteA(NULL, "open", global_end.c_str(), "", NULL, nShowCmd);

			PostQuitMessage(0);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void OnTimerMidi()
{
	if(global_bsendmidi)
	{
		//global_prevstep++;
		if(global_prevstep<0) global_prevstep=0;
		if(global_prevstep>(global_pmeventlistsize-1)) global_prevstep=0; //restart to begining

		//if(global_miditimerskip==true || global_pmevenlistbusyfromglobaltimer[global_prevstep]==false)
		if(global_miditimerskip==false && global_pmevenlistbusyfromglobaltimer[global_prevstep]==false)
		{
			//if(pFILE) fprintf(pFILE, "global_prevstep=%d, ",global_prevstep);
			global_pmevenlistbusyfrommiditimer[global_prevstep]=true;
			for (it_pmeventlist = global_pmeventlist[global_prevstep].begin(); it_pmeventlist != global_pmeventlist[global_prevstep].end(); it_pmeventlist++)
			{
				if((*it_pmeventlist)!=NULL)
				{
					//if(pFILE) fprintf(pFILE, "midievent sent, ");
					//*it_pmeventlist; //spi, was useless
					//send midi event
					Pm_Write(global_pPmStream, (*it_pmeventlist), 1);
					//delete pmevent
					delete *it_pmeventlist;
					*it_pmeventlist = NULL;
				}
			}
			//all midi event sent and deleted, empty list
			//global_pmeventlist[global_prevstep].empty();
			global_pmeventlist[global_prevstep].clear();
			global_pmevenlistbusyfrommiditimer[global_prevstep]=false;
			//if(pFILE) fprintf(pFILE, "\n");
		}
		global_prevstep++;
	}
}

void OnTimerMidiProgramChange()
{
	if(global_bsendmidi)
	{
		PmEvent myPmEvent;

		int random_integer;
		int lowest=0;
		int highest=128-1;
		//int highest=7-1;
		int range=(highest-lowest)+1;
		//random_integer = lowest+int(range*rand()/(RAND_MAX + 1.0));
		random_integer = lowest+rand()%range;
		//fprintf(pFILE, "random_integer=%d\n", random_integer);	
		
		//PmEvent myPmEvent;
		myPmEvent.timestamp = 0;
		int midiprogramnumber = random_integer;
		if(midiprogramnumber>=128) midiprogramnumber=127;
		if(midiprogramnumber<=0) midiprogramnumber=0;
		int notused = 0;
		myPmEvent.message = Pm_Message(0xC0+global_outputmidichannel, midiprogramnumber, 0x00);
		//myPmEvent.message = Pm_Message(192+global_outputmidichannel, midiprogramnumber, 0);
		//send midi event
		Pm_Write(global_pPmStream, &myPmEvent, 1);
		
	}
}


// Initialize OpenGL
static void InitGL(HWND hWnd, HDC & hDC, HGLRC & hRC)
{
  
  PIXELFORMATDESCRIPTOR pfd;
  ZeroMemory( &pfd, sizeof pfd );
  pfd.nSize = sizeof pfd;
  pfd.nVersion = 1;
  //pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL; //blaine's
  pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 24;
  
  hDC = GetDC( hWnd );
  
  int i = ChoosePixelFormat( hDC, &pfd );  
  SetPixelFormat( hDC, i, &pfd );

  hRC = wglCreateContext( hDC );
  wglMakeCurrent( hDC, hRC );

}

// Shut down OpenGL
static void CloseGL(HWND hWnd, HDC hDC, HGLRC hRC)
{
  wglMakeCurrent( NULL, NULL );
  wglDeleteContext( hRC );

  ReleaseDC( hWnd, hDC );
}


void SetupAnimation(HDC hDC, int Width, int Height)
{
		//glFont font
		GLuint textureName;
		glGenTextures(1, &textureName);
		global_GLFont.Create("arial-10.glf", textureName);
		//global_GLFont.Create("arial-20.glf", textureName);
		//global_GLFont.Create("arial-bold-10.glf", textureName);
		//global_GLFont.Create("segeo-script-10.glf", textureName);

		/*
		//spi, begin
		createPalette();
		stepX = (maxX-minX)/(GLfloat)Width; // calculate new value of step along X axis
		stepY = (maxY-minY)/(GLfloat)Height; // calculate new value of step along Y axis
		glShadeModel(GL_SMOOTH);
		glEnable(GL_DEPTH_TEST);
		//spi, end
		*/
        //window resizing stuff
        glViewport(0, 0, (GLsizei) Width, (GLsizei) Height);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        
		//spi, begin
        //glOrtho(-300, 300, -240, 240, 25, 75); //original
        //glOrtho(-300, 300, -8, 8, 25, 75); //spi, last
		glOrtho(-2.0f, 2.0f, -2.0f, 2.0f, ((GLfloat)-1), (GLfloat)1); //spi
        //glOrtho(-150, 150, -120, 120, 25, 75); //spi
		//spi, end
        
		
		glMatrixMode(GL_MODELVIEW);

        glLoadIdentity();
		/*
        gluLookAt(0.0, 0.0, 50.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0); //original
                //camera xyz, the xyz to look at, and the up vector (+y is up)
        */

        //background
        glClearColor(0.0, 0.0, 0.0, 0.0); //0.0s is black

		// set the foreground (pen) color
		//glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		// enable blending
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		// enable point smoothing
		glEnable(GL_POINT_SMOOTH);
		glPointSize(1.0f);		
		
		/*
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
        glShadeModel(GL_SMOOTH); 
		*/

        //no need to initialize any objects
        //but this is where I'd do it

		/*
        glColor3f(0.1, 1.0, 0.3); //green
		*/
		// compute some initial iterations to settle into the orbit of the attractor
		//for (int i = 0; i &lt; initialIterations; i++) 
		for (int i = 0; i<initialIterations; i++) 
		{
 
			// compute a new point using the strange attractor equations
			float xnew = sin(y*b) + c*sin(x*b);
			float ynew = sin(x*a) + d*sin(y*a);
 
			// save the new point
			x = xnew;
			y = ynew;
		}
		//spi, begin
		//draw strange attractor once
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPushMatrix();
		glBegin(GL_POINTS);
			// go through the equations many times, drawing a point for each iteration
			for (int i = 0; i<iterations; i++) 
			//for (int i = 0; i<10; i++) 
			{
				// compute a new point using the strange attractor equations
				float xnew = sin(y*b) + c*sin(x*b);
				float ynew = sin(x*a) + d*sin(y*a);
 
				// save the new point
				x = xnew;
				y = ynew;
 
				// draw the new point
				glVertex3f(x, y, 0.0f);
			}
		glEnd();
		glFlush();
        SwapBuffers(hDC);
        glPopMatrix();

		if(global_idcolor==0)
		{
			glColor4f(0xff/0xff, 0xff/0xff, 0xff/0xff, 1.0f); //white
		}
		else if(global_idcolor==1)
		{
			glColor4f(255/255, 0.0f, 0.0f, 1.0f); //red
		}
		else if(global_idcolor==2)
		{
			glColor4f(0.0f, 255/255, 0.0f, 1.0f); //green
		}
		else if(global_idcolor==3)
		{
			glColor4f(0.0f, 0.0f, 255/255, 1.0f); //blue
		}
		//spi, end
}


void CleanupAnimation()
{
        //didn't create any objects, so no need to clean them up
}
