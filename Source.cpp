#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <chrono>
#include <vector>

DWORD displayMode = CONSOLE_WINDOWED_MODE;
HANDLE console, hScreenBuffer;
CONSOLE_FONT_INFOEX fontInfo;
COORD screenSize = { 0, 0 };
std::vector<CHAR_INFO> screenBuffer;
SMALL_RECT screenCoords;
const int MAX_TRAIL_LENGTH = 30;
const int MAX_SPACE_BETWEEN_TRAILS = 50;
const int TEXT_ROWS = 8;
const int TEXT_COLS = 111;

struct Drop {
	int x; // no reason for x to be a double
	double y, speed;
	char val;
};

double ticksPerSecond = 60;
std::string availableCharacters;
std::string text = 
"______               _  _     _____                      _                                _            __   __ "
"| _  \\               ()| |   |_   _|                    | |                              | |          / _| / _|"
"| | | |  ___   _ __  |/| |_    | |    ___   _   _   ___ | |__    _ __ ___   _   _   ___  | |_  _   _ | |_ | |_ "
"| | | | / _ \\ | '_ \\   | __|   | |   / _ \\ | | | | / __|| '_ \\  | '_ ` _ \\ | | | | / __| | __|| | | ||  _||  _|"
"| |/ / | (_) || | | |  | |_    | |  | (_) || |_| || (__ | | | | | | | | | || |_| | \\__ \\ | |_ | |_| || |  | |  "
"|___ /  \\___/ |_| |_|  \\__ |    \\_ / \\___/  \\__,_| \\___||_| |_| |_| |_| |_| \\__, | |___ / \\__| \\__,_||_|  |_|  "
"	                                                                            __/ |                             "
"	                                                                           |___ /                            ";
std::vector<Drop> drops;

void loadCharacters();
void createScreen();
void makeNewDropAt(int);
void updateDrops();
void drawDrops();
void drawText();
void draw();
void getScreenSize();

int main() {
	// Setup
	srand(time(0));
	using namespace std::chrono;
	console = GetStdHandle(STD_OUTPUT_HANDLE);

	hScreenBuffer = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

	fontInfo = { sizeof(CONSOLE_FONT_INFOEX), 0, {8, 16}, 400, (WCHAR) "Courier New" };
	loadCharacters();
	SetConsoleTitle("Don't Touch Please :)");
	
	// error handling
	if (!SetCurrentConsoleFontEx(hScreenBuffer, FALSE, &fontInfo)) {
		printf("Can't use Courier New :/\n");
		return -1;
	}

	if (hScreenBuffer == NULL) {
		printf("Couldn't create a screen buffer :(\n");
		return -1;
	}

	// initial load
	getScreenSize();
	createScreen();

	ReadConsoleOutput(console, screenBuffer.data(), screenSize, { 0, 0 }, &screenCoords);
	SetConsoleActiveScreenBuffer(hScreenBuffer);
	
	high_resolution_clock::time_point clock = high_resolution_clock::now();
	duration<double> elapsedTime;

	printf("Author: Christopher Camarillo");
	printf("Press [ENTER] to return to windowed mode and [SPACE] to exit!\n");
	system("timeout 3 /NOBREAK");

	while (!GetAsyncKeyState(VK_SPACE)) {

		// Toggle fullscreen
		if (GetAsyncKeyState(VK_RETURN)) {
			while (GetAsyncKeyState(VK_RETURN)) {} // do nothing;

			displayMode = (displayMode == CONSOLE_WINDOWED_MODE) ? CONSOLE_FULLSCREEN_MODE : CONSOLE_WINDOWED_MODE;
			SetConsoleDisplayMode(hScreenBuffer, displayMode, NULL);

			// gather screen information and recreate buffer
			getScreenSize();
			createScreen();
		}

		// timing calculations
		elapsedTime = high_resolution_clock::now() - clock;
		
		// make it rain
		if (elapsedTime.count() > 1/ticksPerSecond) {
			updateDrops();
			clock = high_resolution_clock::now();
		}

		draw();
	}
	SetConsoleActiveScreenBuffer(console);
	system("pause");
	return 0;
}

void loadCharacters() {
	// loads all available characters from this font

	// [a-z] and [A-Z]
	for (int i = 0; i < 26; i++) {
		availableCharacters.push_back('a' + i);
		availableCharacters.push_back('A' + i);
	}

	// 0 - 9
	for (int i = 0; i < 10; i++)
		availableCharacters.push_back('0' + i);

	// other special
	availableCharacters.append("$+-*/=%\"'#&_(),.;:?!\\|{}<>[]^~");
}

void createScreen() {
	system("cls");

	// make new drops
	drops.clear();

	for (int i = 0; i < screenSize.X; i++)
		makeNewDropAt(i);

	// initalize and prepare screen buffer for reading/writing
	screenBuffer.reserve(screenSize.X * screenSize.Y);
	screenBuffer.clear();
	for (int i = 0; i < screenBuffer.capacity(); i++)
		screenBuffer.push_back({ ' ' });

	ReadConsoleOutput(hScreenBuffer, screenBuffer.data(), screenSize, { 0, 0 }, &screenCoords);
}

void getScreenSize() {
	_CONSOLE_SCREEN_BUFFER_INFO screenInfo;
	GetConsoleScreenBufferInfo(hScreenBuffer, &screenInfo);
	screenSize = screenInfo.dwSize;
	screenCoords = {0, 0, screenSize.X - 1, screenSize.Y - 1};
}


void makeNewDropAt(int x) {
	double dropY = -(rand() % MAX_SPACE_BETWEEN_TRAILS + 1);
	double dropSpeed = 0.5 + (rand() % 50 + 1) / 100; // 0.5 - 1
	int trailLength = rand() % MAX_TRAIL_LENGTH + 1;
	char randomChar = availableCharacters[rand() % availableCharacters.size()];

	// initalize a new drop starting at the top of the screen at a random X with speed in range 0.1 - 1.0
	drops.push_back({ x, dropY, dropSpeed, randomChar});

	// Create a trail to "clean up"
	drops.push_back({ x, dropY - trailLength, dropSpeed, ' ' });
}

void updateDrops() {
	for (Drop &drop : drops) {
		if (drop.y < screenSize.Y - 1)
			drop.y += drop.speed;
		else {
			drop.y = 0;
			drop.speed = (rand() % 100 + 1) / 100.0;
		}

		if (drop.val != ' ')
			drop.val = { availableCharacters[rand() % availableCharacters.size()] };
	}
}

void drawDrops() {
	int dropIndex;
	for (int i = 0; i < screenSize.X * screenSize.Y - 1; i++)
		screenBuffer[i].Attributes = FOREGROUND_GREEN;

	for (Drop &drop : drops) {
		dropIndex = drop.x + (int) (drop.y) * screenSize.X;
		if (drop.y >= 0 && drop.y < screenSize.Y - 1) {
			screenBuffer[dropIndex].Char = { (WCHAR) drop.val };
			screenBuffer[dropIndex].Attributes = 0xF;
		}
	}
}

void drawText() {
	// Find location to place text (middle)
	int startX = (screenSize.X - TEXT_COLS) / 2;
	int startY = (screenSize.Y - TEXT_ROWS) / 2;

	// Draw text
	for (int row = 0; row < TEXT_ROWS; row++) {
		for (int col = 0; col < TEXT_COLS; col++) {
			if (text[col + row * TEXT_COLS] != ' ') {
				screenBuffer[startX + (startY + row) * screenSize.X + col].Attributes = 0xF;
				screenBuffer[startX + (startY + row) * screenSize.X + col].Char = { (WCHAR) text[col + row*TEXT_COLS] };
			}
		}
	}
}

void draw() {
	drawDrops();
	drawText();

	if (!WriteConsoleOutput(hScreenBuffer, screenBuffer.data(), screenSize, { 0, 0 }, &screenCoords))
		printf("Couldn't write to console!\n");
}