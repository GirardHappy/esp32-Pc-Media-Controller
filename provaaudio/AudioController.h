#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <string>
#include <vector>
#include <psapi.h>
using namespace std;

class AudioController {
	static bool initialized;
	static IAudioSessionManager2* sessionManager;
	static IAudioSessionEnumerator* sessionEnumerator;
	static IAudioEndpointVolume* masterVolume;
	static int enumeratorCount;
	IAudioSessionControl2* controller;
	ISimpleAudioVolume* volume;

	static void isInitialized() {
		if (!initialized) {
			throw exception("Audio Controller not initialized - call AudioController::initialize()");
		}
	}

	static string appNameFromPid(int pid) {
		string result = "UNKNOWN";
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		if (hProcess != NULL) {
			char buffer[MAX_PATH];
			GetProcessImageFileNameA(hProcess, buffer, MAX_PATH);
			result = std::string(buffer);
			CloseHandle(hProcess);
		}
		int i;
		int len = result.length();
		for (i = len; i >= 0; i--) {
			if (result[i] == '\\') {
				break;
			}
		}
		result = result.substr(i + 1, len - i - 5);
		return result;
	}

public:
	static vector<AudioController> audioControllers;
	string appName;
	int pid;

	AudioController(IAudioSessionControl2* controller, ISimpleAudioVolume* volume, string appName, int pid) {
		this->controller = controller;
		this->volume = volume;
		this->appName = appName;
		this->pid = pid;
	}


	static void initialize() {
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
		IMMDevice* pDevice;
		IMMDeviceEnumerator* pEnumerator;

		// Create the device enumerator.
		if (CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator) != S_OK) {
			throw exception("Failed to create device enumerator");
		}

		if (pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice) != S_OK) {
			throw exception("Failed to get default audio device");
		}

		if (pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID*)&masterVolume) != S_OK) {
			throw exception("Failed to get master volume endpoint");
		}

		if (pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&sessionManager) != S_OK) {
			throw exception("Failed to create session manager");
		}

		if (pDevice != nullptr) {
			pDevice->Release();
		}
		if (pEnumerator != nullptr) {
			pEnumerator->Release();
		}

		initialized = true;
		Update();
	}

	static void Update() {
		isInitialized();
		if (sessionManager->GetSessionEnumerator(&sessionEnumerator) != S_OK)
			throw exception("Failed to get session enumerator");

		if (sessionEnumerator->GetCount(&enumeratorCount) != S_OK)
			throw exception("Failed to get session count");

		audioControllers.clear();
		for (int i = 0; i < enumeratorCount; ++i) {
			IAudioSessionControl* session = nullptr;
			IAudioSessionControl2* session2 = nullptr;
			ISimpleAudioVolume* volume = nullptr;
			DWORD pid = 0;
			string appName = "";
			if (sessionEnumerator->GetSession(i, &session) == S_OK) {
				session->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&session2);
				if (session2->GetProcessId(&pid) == S_OK) {
					session->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&volume);
					appName = appNameFromPid(pid);
					audioControllers.push_back(AudioController(session2, volume, appName, pid));
				}
				session->Release();
			}

		}
	}

	static void setMasterVolume(float vol) {
		isInitialized();
		masterVolume->SetMasterVolumeLevelScalar(vol, NULL);
	}

	static AudioController* getControllerByAppName(string s) {
		Update();
		for (auto c : audioControllers) {
			if (c.appName == s) {
				return &c;
			}
		}
		return NULL;
	}

	static vector<AudioController> getControllersByAppName(string s) {
		Update();
		vector<AudioController> r = vector<AudioController>();
		for (auto c : audioControllers) {
			if (c.appName == s) {
				r.push_back(c);
			}
		}
		return r;
	}

	void setVolume(float vol) {
		volume->SetMasterVolume(vol, NULL);
	}
};

bool AudioController::initialized = false;
IAudioSessionManager2* AudioController::sessionManager;
IAudioSessionEnumerator* AudioController::sessionEnumerator;
int AudioController::enumeratorCount;
vector<AudioController> AudioController::audioControllers;
IAudioEndpointVolume* AudioController::masterVolume;

