//main.h

#define SAFE_RELEASE(x) if (x) { x->Release(); x = NULL; }

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef uintptr_t PTR;


//globals
bool showmenu = false;
bool initonce = false;
int countnum = -1;

//item states
bool wallhackm = 1; //models, enabled by default
bool wallhacki = 0; //items
bool wallhackw = 0; //weapons
bool wallhacka = 0; //ammo

//rendertarget
ID3D11RenderTargetView* mainRenderTargetViewD3D11;

//wh
ID3D11RasterizerState* DEPTHBIASState_FALSE;
ID3D11RasterizerState* DEPTHBIASState_TRUE;

//wire
ID3D11RasterizerState* rwState;
ID3D11RasterizerState* rsState;

//get dir
using namespace std;
#include <fstream>
char dlldir[320];
char* GetDirFile(char* filename)
{
	static char path[320];
	strcpy_s(path, dlldir);
	strcat_s(path, filename);
	return path;
}

//log
void Log(const char* fmt, ...)
{
	if (!fmt)	return;

	char		text[4096];
	va_list		ap;
	va_start(ap, fmt);
	vsprintf_s(text, fmt, ap);
	va_end(ap);

	ofstream logfile(GetDirFile("log.txt"), ios::app);
	if (logfile.is_open() && text)	logfile << text << endl;
	logfile.close();
}

#include <string>
void SaveCfg()
{
	ofstream fout;
	fout.open(GetDirFile("qcwh.ini"), ios::trunc);
	fout << "Wallhack " << wallhackm << endl;
	fout << "Wallhack " << wallhacki << endl;
	fout << "Wallhack " << wallhackw << endl;
	fout << "Wallhack " << wallhacka << endl;
	fout.close();
}

void LoadCfg()
{
	ifstream fin;
	string Word = "";
	fin.open(GetDirFile("qcwh.ini"), ifstream::in);
	fin >> Word >> wallhackm;
	fin >> Word >> wallhacki;
	fin >> Word >> wallhackw;
	fin >> Word >> wallhacka;
	fin.close();
}
