#include "statusbar.h"
#include <sstream>
#include <cstring>

#define COLOR_STATUS 3

StatusBar::StatusBar(int y, int x, int width)
    : winY_(y), winX_(x), width_(width), showTemp_(false)
{
    init_pair(COLOR_STATUS, COLOR_BLACK, COLOR_CYAN);
    win_ = newwin(1, width_, winY_, winX_);
}

StatusBar::~StatusBar() {
    if (win_) delwin(win_);
}

void StatusBar::resize(int y, int x, int width) {
    winY_ = y; winX_ = x; width_ = width;
    wresize(win_, 1, width_);
    mvwin(win_, winY_, winX_);
}

void StatusBar::draw(int row, int col,
                     const std::string& filename,
                     bool dirty, int totalLines)
{
    werase(win_);
    wbkgd(win_, COLOR_PAIR(COLOR_STATUS));
    wattron(win_, COLOR_PAIR(COLOR_STATUS));

    if (showTemp_) {
        // Mostrar mensaje temporal centrado
        int len = (int)tempMsg_.size();
        int startX = std::max(0, (width_ - len) / 2);
        mvwprintw(win_, 0, startX, "%s", tempMsg_.c_str());
        showTemp_ = false;
    } else {
        // Nombre de archivo + estado
        std::string name = filename.empty() ? "[Sin título]" : filename;
        if (dirty) name += " [*]";

        // Derecha: posición del cursor
        std::ostringstream right;
        right << "Ln " << (row + 1) << "/" << totalLines
              << "  Col " << (col + 1)
              << "  F1:Ayuda";

        int rightLen  = (int)right.str().size();
        int nameLen   = (int)name.size();
        int padding   = width_ - nameLen - rightLen - 2;
        if (padding < 1) padding = 1;

        mvwprintw(win_, 0, 1, "%s", name.c_str());
        mvwprintw(win_, 0, width_ - rightLen - 1, "%s", right.str().c_str());
    }

    wattroff(win_, COLOR_PAIR(COLOR_STATUS));
    wrefresh(win_);
}

void StatusBar::showMessage(const std::string& msg, int /*durationMs*/) {
    tempMsg_ = msg;
    showTemp_ = true;
}
