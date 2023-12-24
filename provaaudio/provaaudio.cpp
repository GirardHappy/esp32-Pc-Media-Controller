#include <iostream>
#include <string>
#include "AudioController.h"
using namespace std;



int main() {
	AudioController::initialize();
	AudioController* spotify = AudioController::getControllerByAppName("Spotify");
	spotify->setVolume(1);
	return 0;
}