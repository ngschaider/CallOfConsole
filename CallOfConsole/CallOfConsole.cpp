// CallOfConsole.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include <iostream>
#include <chrono>
#include <vector>
#include <utility>
#include <algorithm>

#include <stdio.h>
#include <Windows.h>

using namespace std;

int screenWidth = 120;
int screenHeight = 40;

int mapWidth = 16;
int mapHeight = 16;

float playerRot = 0.0f;
float playerX = 14.0f;
float playerY = 5.0f;

float fov = 3.1415926535f / 4.0f;

float depth = 16.0f;
float walkingSpeed = 5.0f;

float rotationSpeed = 0.01f;

bool IsKeyPressed(char c) {
	SHORT keyState = GetAsyncKeyState((unsigned short)c);
	return keyState & 0x8000;
}

bool isWall(wstring map, int x, int y) {
	int index = x + mapWidth * y;
	return map.c_str()[index] == '#';
}

void drawMinimap(wstring map, wchar_t* screen) {
	// Display Map
	for (int x = 0; x < mapWidth; x++) {
		for (int y = 0; y < mapWidth; y++)
		{
			screen[(y + 1) * screenWidth + x] = map[y * mapWidth + x];
		}
	}
	screen[((int)playerX + 1) * screenWidth + (int)playerY] = 'P';
}

void handleInput(wstring map, float deltaTime) {
	// rotate left
	if (IsKeyPressed('A')) {
		playerRot -= (walkingSpeed * 0.75f) * deltaTime;
	}

	// rotate right
	if (IsKeyPressed('D')) {
		playerRot += (walkingSpeed * 0.75f) * deltaTime;
	}

	// Move forward
	if (IsKeyPressed('W')) {
		playerX += sinf(playerRot) * walkingSpeed * deltaTime;
		playerY += cosf(playerRot) * walkingSpeed * deltaTime;

		if (isWall(map, (int) playerX, (int) playerY))
		{
			playerX -= sinf(playerRot) * walkingSpeed * deltaTime;
			playerY -= cosf(playerRot) * walkingSpeed * deltaTime;
		}
	}

	// Move backward
	if (IsKeyPressed('S')) {
		playerX -= sinf(playerRot) * walkingSpeed * deltaTime;
		playerY -= cosf(playerRot) * walkingSpeed * deltaTime;


		if (isWall(map, (int) playerX, (int) playerY))
		{
			playerX += sinf(playerRot) * walkingSpeed * deltaTime;
			playerY += cosf(playerRot) * walkingSpeed * deltaTime;
		}
	}
}

int main()
{
	wstring map;
	map += L"#########.......";
	map += L"#...............";
	map += L"#.......########";
	map += L"#..............#";
	map += L"#......##......#";
	map += L"#......##......#";
	map += L"#..............#";
	map += L"###............#";
	map += L"##.............#";
	map += L"#......####..###";
	map += L"#......#.......#";
	map += L"#......#.......#";
	map += L"#..............#";
	map += L"#......#########";
	map += L"#..............#";
	map += L"################";

	auto lastFrame = chrono::system_clock::now();
	auto thisFrame = chrono::system_clock::now();

	// Create screen buffer
	wchar_t* screen = new wchar_t[screenWidth * screenHeight];
	HANDLE console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

	SetConsoleActiveScreenBuffer(console);

	while (true) {
		thisFrame = chrono::system_clock::now();
		chrono::duration<float> deltaTimeDuration = thisFrame - lastFrame;
		lastFrame = thisFrame;

		float deltaTime = deltaTimeDuration.count();

		handleInput(map, deltaTime);

		for (int x = 0; x < screenWidth; x++) {
			// For each column, calculate the projected ray angle into world space
			float rayAngle = (playerRot - fov / 2.0f) + ((float)x / (float)screenWidth) * fov;

			// Find distance to wall
			float stepSize = 0.1f; // for ray casting, decrease to increase resolution
			float distanceToWall = 0.0f;

			bool hasHitWall = false; // Set when ray hits wall block
			bool hasHitBoundary = false; // Set when ray hits boundary between two wall blocks

			// Unit vector for ray in player space
			float eyeX = sinf(rayAngle);
			float eyeY = cosf(rayAngle);

			while (!hasHitWall && distanceToWall < depth)
			{
				distanceToWall += stepSize;
				int nTestX = (int)(playerX + eyeX * distanceToWall);
				int nTestY = (int)(playerY + eyeY * distanceToWall);

				// Test if ray is out of bounds
				if (nTestX < 0 || nTestX >= mapWidth || nTestY < 0 || nTestY >= mapHeight)
				{
					hasHitWall = true;			// Just set distance to maximum depth
					distanceToWall = depth;
				}
				else
				{
					// Ray is inbounds so test to see if the ray cell is a wall block
					if (map.c_str()[nTestX * mapWidth + nTestY] == '#')
					{
						// Ray has hit wall
						hasHitWall = true;

						// To highlight tile boundaries, cast a ray from each corner
						// of the tile, to the player. The more coincident this ray
						// is to the rendering ray, the closer we are to a tile
						// boundary, which we'll shade to add detail to the walls
						vector<pair<float, float>> p;

						// Test each corner of hit tile, storing the distance from
						// the player, and the calculated dot product of the two rays
						for (int tx = 0; tx < 2; tx++)
							for (int ty = 0; ty < 2; ty++)
							{
								// Angle of corner to eye
								float vy = (float)nTestY + ty - playerY;
								float vx = (float)nTestX + tx - playerX;
								float d = sqrt(vx * vx + vy * vy);
								float dot = (eyeX * vx / d) + (eyeY * vy / d);
								p.push_back(make_pair(d, dot));
							}

						// Sort Pairs from closest to farthest
						sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) {
							return left.first < right.first;
							});

						// First two/three are closest (we will never see all four)
						float fBound = 0.01f;
						if (acos(p.at(0).second) < fBound) hasHitBoundary = true;
						if (acos(p.at(1).second) < fBound) hasHitBoundary = true;
						if (acos(p.at(2).second) < fBound) hasHitBoundary = true;
					}
				}
			}

			int ceiling = (float) screenHeight / 2.0 - screenHeight / distanceToWall;
			int floor = screenHeight - ceiling;

			short shade = ' ';
			if (distanceToWall <= depth / 4.0f) {
				shade = 0x2588;
			}
			else if (distanceToWall <= depth / 3.0f) {
				shade = 0x2593;
			}
			else if (distanceToWall <= depth / 2.0f) {
				shade = 0x2592;
			}
			else if (distanceToWall <= depth) {
				shade = 0x2591;
			}
			else {
				shade = ' ';
			}

			if (hasHitBoundary) {
				shade = ' ';
			}


			for (int y = 0; y < screenHeight; y++)
			{
				// Each Row
				if (y <= ceiling) {
					screen[y * screenWidth + x] = ' ';
				}
				else if (y > ceiling && y <= floor) {
					screen[y * screenWidth + x] = shade;
				}
				else // Floor
				{
					// Shade floor based on distance
					float b = 1.0f - (((float)y - screenHeight / 2.0f) / ((float)screenHeight / 2.0f));
					if (b < 0.25) {
						shade = '#';
					}
					else if (b < 0.5) {
						shade = 'x';
					}
					else if (b < 0.75) {
						shade = '.';
					}
					else if (b < 0.9) {
						shade = '-';
					}
					else {
						shade = ' ';
					}
					screen[y * screenWidth + x] = shade;
				}
			}
		}

		// Display Stats
		swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f, FPS=%3.2f", playerX, playerY, playerRot, 1.0f / deltaTime);

		drawMinimap(map, screen);

		// Display Frame
		screen[screenWidth * screenHeight - 1] = '\0';

		DWORD bytesWritten;
		WriteConsoleOutputCharacter(console, screen, screenWidth * screenHeight, { 0,0 }, &bytesWritten);
	}

	return 0;
}