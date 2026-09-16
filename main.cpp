#include <windows.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

using namespace std;

DWORD fdwSaveOldMode;

void KeyEventProc(KEY_EVENT_RECORD);
void ResizeEventProc(WINDOW_BUFFER_SIZE_RECORD);
void processInput(INPUT_RECORD);
void drawLine(int x1, int y1, int x2, int y2);
void drawSquare(int x, int y, int width, int height, char chr, bool filled);
void drawTextBox(int x, int y, string txt);
int round(double x);
void update();
void display();

bool run = false;
int HEIGHT = 72;
int WIDTH = 256;

vector<string> screenBuffer;

struct Pipe{
	int x, y;
	int wdh = 10;
	bool point = true;
};

struct Player{
	int x;
	float y;
	int width=5;
	float jump = -2.5;
	float velocity=0.0;
	float acceleration=0.4;
	int score = 0;
	int highscore = 0;
};

Player p = {30, float(HEIGHT/2)};

int randint(int lowest, int highest){ return lowest+rand()%(highest-lowest); }

vector<Pipe> pipes;

void reset(){
	pipes = {{256, randint(10, HEIGHT-10)}, {346, randint(10, HEIGHT-10)}, {436, randint(10, HEIGHT-10)}};
	p.y = float(HEIGHT/2);
	p.velocity = 0;
	p.highscore = max(p.highscore, p.score);
	p.score = 0;
}

void pipe_handle(float delta){
	int gapDist = 8;
	for (int i = 0; i<pipes.size(); i++){
		drawSquare(pipes[i].x, 0, pipes[i].wdh, pipes[i].y-gapDist, '7', false);
		drawSquare(pipes[i].x, pipes[i].y+gapDist, pipes[i].wdh, HEIGHT, '7', false);
		
		if (pipes[i].x < p.x+p.width & pipes[i].x + pipes[i].wdh > p.x){
			if (pipes[i].y-gapDist > p.y || pipes[i].y+gapDist < p.y+p.width){
				reset();
			}
			else if (pipes[i].point == true){
				p.score += 1;
				pipes[i].point = false;
			}
		}
		
		pipes[i].x -= 3*delta;
		if (pipes[i].x + pipes[i].wdh < 0){ 
			pipes[i].x = WIDTH+1;
			pipes[i].y = randint(10, HEIGHT-10);
			pipes[i].point = true;
		}
	}
}

void player_handle(float delta){
	drawSquare(p.x, round(p.y), p.width, p.width, 'O', true);
	
	p.y += p.velocity;
	p.velocity += p.acceleration;
	p.velocity = min(p.velocity, 10.0f);
	
	if (p.y+p.width < 0 || p.y-1 > HEIGHT) { reset(); }
}

int main() {
    // get console handle
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

    INPUT_RECORD irInBuf[128];
    DWORD cNumRead, fdwMode, i;

    GetConsoleMode(hStdin, &fdwSaveOldMode);

    // Enable the window and mouse input events.

    fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
    SetConsoleMode(hStdin, fdwMode);

    COORD coord;
    coord.X = 0;
    coord.Y = 0;

    
    char x = ' ';
    screenBuffer.assign(HEIGHT, string(WIDTH, x));
    
    auto start{chrono::steady_clock::now()};
    auto end{chrono::steady_clock::now()};
    chrono::duration<double> elapsed_seconds;

    cout << "please zoom out until you see the screen" << endl;
    // game loop
	reset();
	
    auto inp = [](HANDLE hStdin, INPUT_RECORD* irInBuf, DWORD bufferSize, DWORD* cNumRead) {
        while (true) {
            ReadConsoleInput(hStdin, irInBuf, bufferSize, cNumRead);
        }
    };

    thread thread_obj(inp, hStdin, irInBuf, 128, &cNumRead);

    int y = 0;
	float delta;
    while (true){
        elapsed_seconds = end-start;
        if (elapsed_seconds.count() >= 1.0/16){
			delta = elapsed_seconds.count()*16;
            // puts the cursor at (0, 0)
            if (run){
                SetConsoleCursorPosition(hConsole, coord);
                screenBuffer.assign(HEIGHT, string(WIDTH, x));
				
				// game logic
				pipe_handle(delta);
				player_handle(delta);
				drawTextBox(WIDTH*0.90, 5, "HS: " + to_string(p.highscore) + " SCORE: " + to_string(p.score));
				
                display();
            }

            for (i = 0; i < cNumRead; i++)
            {
                processInput(irInBuf[i]);
            }

            start = chrono::steady_clock::now();
        }
        end = chrono::steady_clock::now();
    }
    
    SetConsoleMode(hStdin, fdwSaveOldMode);

    return 0;
}

// process keyboard input
void KeyEventProc(KEY_EVENT_RECORD ker)
{
	if (ker.bKeyDown & ker.wVirtualKeyCode == 32) { p.velocity = p.jump; }
}

void ResizeEventProc(WINDOW_BUFFER_SIZE_RECORD wbsr)
{
    //printf("Console screen buffer is %d columns by %d rows.\n", wbsr.dwSize.X, wbsr.dwSize.Y);
    if (wbsr.dwSize.X >= 275) run = true;
    else run = false;
}

// get input and pass to appropreate functon
void processInput(INPUT_RECORD type){
    switch(type.EventType){
        case KEY_EVENT: // keyboard input
            KeyEventProc(type.Event.KeyEvent);
            break;

        case MOUSE_EVENT:
            break;

        case WINDOW_BUFFER_SIZE_EVENT: // scrn buf. resizing
            ResizeEventProc(type.Event.WindowBufferSizeEvent);
            break;

        case FOCUS_EVENT:  // disregard focus events

        case MENU_EVENT:   // disregard menu events
            break;

        default:
            break;
    }
}

void display(){
    for (string i: screenBuffer)
                    cout << i << endl;
}

// I don't want to import a how library just for one function
int round(double x){
    if (x - (int)x > 0.5){ return (int)x + 1; }
    return (int)x;
}

void drawLine(int x1, int y1, int x2, int y2, char chr){
    x1 = max(0, min(x1, WIDTH));
	x2 = max(0, min(x2, WIDTH));
	y1 = max(0, min(y1, HEIGHT));
	y2 = max(0, min(y2, HEIGHT));
	
    double dx = abs(x2 - x1);
    short sx = (x1 < x2) ? 1 : -1;
    double dy = -abs(y2 - y1);
    short sy = (y1 < y2) ? 1 : -1;
    double error = dx + dy;
    double e2;

    while (true){
        screenBuffer.at(round(y1))[x1] = chr;
        e2 = 2 * error;
        if (e2 >= dy){
            if (x1 == x2) break;
            error += dy;
            x1 += sx;
        }
        if (e2 <= dx){
            if (y1 == y2) break;
            error = error + dx;
            y1 += sy;
        }
    }
}

void drawSquare(int x, int y, int width, int height, char chr, bool filled){
	if (y + height > HEIGHT) { height = HEIGHT - y-1; }
	if (filled == false){
		drawLine(x, y, x+width, y, chr);
		drawLine(x, y, x, y+height, chr);
		drawLine(x+width, y, x+width, y+height, chr);
		drawLine(x, y+height, x+width, y+height, chr);
	} else {
		for (int i = y; i<min(y+height, HEIGHT); i++){
			drawLine(x, i, x+width, i, chr);
		}
	}
}

void drawTextBox(int x, int y, string txt){
	drawSquare(x, y, txt.size()+3, 4, '`', false);
	drawSquare(x+1, y+1, txt.size()+1, 3, ' ', true);
	for (int i=0; i<txt.size(); i++){
		screenBuffer.at(round(y+2))[x+2+i] = txt[i];
	}
}
	