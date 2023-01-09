#pragma once

#include <utility>
#include <vector>
#include <iostream>
#include <fcntl.h>
#include <windows.h>
#include <sstream>
#include <chrono>
#include <conio.h>

#define DEFAULT_GAME_WIDTH  31 // must be odd
#define DEFAULT_GAME_HEIGHT 20
#define DEFAULT_GAME_TIME   15000

using namespace std;

#define RANDOM_GEN(a, b) ((rand() % ((b) - (a))) + (a))

namespace GameObjects {
  using Timer = std::chrono::system_clock;

  class Snake {
    struct Part {
      int x;
      int dir;
    };

   public:
    int x, y;
    // parts
    vector<Part> parts;

   public:
    explicit Snake(int x, int y, int length, int dir) : x(x), y(y) {
      for (int i=0; i < length; ++i)
        parts.push_back(Part{dir > 0 ? x + i : x + length - i, dir});
    }

    void update(int width) {
      for (auto& part : parts) {
        part.x += part.dir;
        if (part.x >= width or part.x < 0) {
          part.dir *= -1; // reverse direction
          part.x += part.dir;
        }
      }
    }
  };

  class Obstacle {
   public:
    int x, y;
    int width;

   public:
    explicit Obstacle(int x, int y, int width) : x(x), y(y), width(width) {
    }
  };

  class Plane {
   public:
    int width;
    int height;
    //
    vector<Snake>    snakes;
    vector<Obstacle> obstacles;

   public:
    explicit Plane(int width, int height) : width(width), height(height) {
    }

    void generate(int beginOffset, int endOffset) {
      for (int y=beginOffset; y<height-endOffset-1; y+=2) {
        // create snake
        {
          int length = RANDOM_GEN(3, 7);
          int dir    = RANDOM_GEN(0, 2) ? 1 : -1;
          // append to snake vector
          snakes.emplace_back(RANDOM_GEN(length + 1, width - length), y+1, length, dir);
        }
        // create obstacles
        {
          for (int x=RANDOM_GEN(1, 3); x < width; x += RANDOM_GEN(2, 4)) {
            int length = RANDOM_GEN(2, 7);
            // append to obstacle vector
            if (x + length >= width)
              length = width - x;
            obstacles.emplace_back(x, y, length);
            x += length;
          }
        }
      }
    }

    void updateSnakes() {
      for (auto& snake : snakes)
        snake.update(width);
    }
  };

  class Player {
   public:
    enum MovementStatus {
      HitObstacle,
      HitSnake,
      HitSnakeHead,
      Succeed
    };

   public:
    int x, y;
    //
    Plane& plane;

   public:
    explicit Player(int x, int y, Plane& plane) : x(x), y(y), plane(plane) {
    }

    MovementStatus move(int xO, int yO) {
      int xT = x + xO,
          yT = y + yO;
      // check for collision with obstacle
      if (xT < 0 or xT >= plane.width or
          yT < 0 or yT >= plane.height)
        return HitObstacle;
      for (auto const& obstacle : plane.obstacles)
        if (obstacle.y == yT and obstacle.x <= xT and xT < obstacle.x + obstacle.width)
          return HitObstacle;
      // check for collision with snake
      for (int i=0; i<plane.snakes.size(); ++i) {
        auto const& snake = plane.snakes[i];
        if (snake.y == yT) {
          int begin = snake.parts.front().x,
              end   = snake.parts.back ().x;
          if (end == xT)
            return HitSnakeHead;
          if (min(begin, end) <= xT and xT <= max(begin, end)) {
            plane.snakes.erase(plane.snakes.begin() + i);
            //
            x = xT, y = yT;
            return HitSnake;
          }
        }
      }
      // no collision
      x = xT, y = yT;
      return Succeed;
    }
  };

  class Display {
   public:
    Plane  &plane;
    Player &player;
    //
    vector<wstring> buffer;
    //
    HANDLE handle;

   public:
    Display(Plane &plane, Player &player) : plane(plane), player(player) {
      // create buffer lines
      buffer.resize(plane.height);
      //
      handle = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    void display() {
      clearBuffer();
      renderObstacles();
      renderSnakes();
      renderPlayer();
      printAll();
    }

    void renderPlayer() {
      buffer[player.y][player.x] = L'◯';
    }

    void renderObstacles() {
      for (auto const& obstacle : plane.obstacles)
        for (int x=0; x<obstacle.width; ++x)
            buffer[obstacle.y][obstacle.x + x] = L'═';
    };

    void renderSnakes() {
      for (auto const& snake : plane.snakes) {
        for (auto const &part: snake.parts)
          buffer[snake.y][part.x] = L'─';
        buffer[snake.y][snake.parts.back().x] = L'╂';
      }
    }

    void printAll(int delay = 0, bool clear = true) const {
      // clear if is needed
      if (clear)
        clearAll();
      // print buffer lines
      wcout << L'╔' << repeatText(L"═", plane.width) << L'╗' << endl;
      for (auto const& line : buffer)
        wcout << (line[0] == L'═' ? L'╠' : L'║') << line << (line[plane.width-1] == L'═' ? L'╣' : L'║') << endl;
      wcout << L'╚' << repeatText(L"═", plane.width) << L'╝' << endl;
      // delay if is needed
      if (delay)
        Sleep(delay);
    }

    static void printMessage(wstring const& text, int width, int sleep=0) {
      int length = (int) text.length();
      int padding = (width - length) / 2;
      // show message
      wcout << L'╔' << repeatText(L"═", width) << L'╗' << endl;
      wcout << L"║"
            << repeatText(L" ", padding) + text + repeatText(L" ", padding + (length + 1) % 2)
            << L"║" << endl;
      wcout << L'╚' << repeatText(L"═", width) << L'╝' << endl;
      // sleep if needed
      if (sleep)
        Sleep(sleep);
    }

    void clearBuffer() {
      for (auto &line : buffer)
        line = wstring(plane.width, L' ');
    }

    void clearAll() const {
      static COORD position{0, 0};
      //
      SetConsoleCursorPosition(handle, position);
    }

   public:
    static wstring toString(long double number) {
      std::wostringstream stream;
                          stream << number;
      return stream.str();
    }

   private:
    static wstring repeatText(wstring const& text, int n) {
      wstring result;
      for (int i = 0; i < n; ++i)
        result += text;
      // return repeated text
      return result;
    }
  };
}

void run(){
   _setmode(_fileno(stdout), 0x20000);
  srand(time(nullptr));

  using namespace GameObjects;

  auto plane   = Plane(DEFAULT_GAME_WIDTH, DEFAULT_GAME_HEIGHT);
       plane.generate(3, 0);
  auto player  = Player(plane.width / 2, 1, plane);
  auto display = Display(plane, player);

  bool stateRunning = false;
  bool stateLoose   = false;
  int  stateSnakes  = 0;

  chrono::milliseconds
    pointUpdate(0),
    pointDraw(0),
    //
    pointBegin(0);

  display.display();
  while (not stateLoose) {
    auto now = chrono::duration_cast<chrono::milliseconds>(Timer::now().time_since_epoch());
    //
    int result = -1;
    // process movement
    if (kbhit()) {
      // read input
      switch (_getch()) {
        case 'w': result = player.move(0, -1); break; // up
        case 's': result = player.move(0,  1); break; // down
        case 'a': result = player.move(-1, 0); break; // left
        case 'd': result = player.move( 1, 0); break; // right
        default: continue;
      }
      // check if game isn't started
      if (not stateRunning) {
        stateRunning = true;
        // begin the timer
        pointBegin = now;
      }
    }
    //
    if (stateRunning and (now - pointBegin).count() > DEFAULT_GAME_TIME)
      break;
    if ((now - pointUpdate).count() > 150) {
      pointUpdate = now;
      // update plane
      plane.updateSnakes();
      if (result == -1)
        result = player.move(0, 0);
    }
    if ((now - pointDraw).count() > 16) {
      pointDraw = now;
      // draw elements
      display.display();
      Display::printMessage(
          Display::toString((double) (DEFAULT_GAME_TIME - (stateRunning ? (now - pointBegin).count() : 0)) / 1000.), plane.width);
    }
    // check movement status
    if (result != -1)
      switch (result) {
        case Player::HitSnake: {
          stateSnakes++;
        } break;
        case Player::HitSnakeHead: {
          stateLoose = true;
        } break;
        default:;
      }
  }

  display.display();
  if (stateLoose)
    GameObjects::Display::printMessage(L"You got hit by snake!", plane.width, 10000);
  else {
    wstringstream message;
    message << "You caught " << stateSnakes << " snakes!";
    GameObjects::Display::printMessage(message.str(), plane.width, 10000);
  }
}