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
void update();
void display();

bool run = false;
vector<string> screenBuffer;

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

    
    char x = ',';
    screenBuffer.assign(72, string(256, x));
    
    auto start{chrono::steady_clock::now()};
    auto end{chrono::steady_clock::now()};
    chrono::duration<double> elapsed_seconds;

    cout << "please zoom out until you see the screen" << endl;
    // game loop

    auto inp = [](HANDLE hStdin, INPUT_RECORD* irInBuf, DWORD bufferSize, DWORD* cNumRead) {
        while (true) {
            ReadConsoleInput(hStdin, irInBuf, bufferSize, cNumRead);
        }
    };

    thread thread_obj(inp, hStdin, irInBuf, 128, &cNumRead);

    int y = 0;
    int change = 1;

    while (true){
        elapsed_seconds = end-start;
        if (elapsed_seconds.count() >= 1/7){
            // puts the cursor at (0, 0)
            if (run){
                SetConsoleCursorPosition(hConsole, coord);
                screenBuffer.assign(72, string(256, x));

                drawLine(0, y, 256, 71-y);
                drawLine(0, 71-y, 255, y);

                display();
            }

            for (i = 0; i < cNumRead; i++)
            {
                processInput(irInBuf[i]);
            }

            if (y >= 71) change = -1;
            else if (y <= 0) change = 1;
            y += change;
            

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
    // if (ker.bKeyDown){ cout << ker.wVirtualKeyCode << endl; }
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
    if (x - (int)x > 0.5){ return x + 1; }
    return (int)x;
}

void drawLine(int x1, int y1, int x2, int y2){
    
    double delX = x2 - x1;
    double delY = y2 - y1;

    if (delX != 0){
        double m = delY/delX;
        for (int i = 0; i<delX; i++){
            screenBuffer.at(round(y1+i*m))[x1+i] = '$';
        }
    }
}