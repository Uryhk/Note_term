#pragma once
#include <ncurses.h>
#include <string>

class StatusBar {
public:
    StatusBar(int y, int x, int width);
    ~StatusBar();

    void draw(int row, int col, const std::string& filename,
              bool dirty, int totalLines);
    void showMessage(const std::string& msg, int durationMs = 2000);
    void resize(int y, int x, int width);

private:
    WINDOW* win_;
    int winY_, winX_, width_;
    std::string tempMsg_;
    bool        showTemp_;
};
