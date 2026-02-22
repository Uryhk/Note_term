#include "menubar.h"
#include <algorithm>

#define COLOR_MENUBAR  4
#define COLOR_DROPDOWN 5
#define COLOR_SEL_ITEM 6

MenuBar::MenuBar(int y, int x, int width)
    : winY_(y), winX_(x), width_(width),
      activeMenu_(-1), activeItem_(0), open_(false), dropWin_(nullptr)
{
    init_pair(COLOR_MENUBAR,  COLOR_BLACK,  COLOR_CYAN);
    init_pair(COLOR_DROPDOWN, COLOR_WHITE,  COLOR_BLUE);
    init_pair(COLOR_SEL_ITEM, COLOR_BLACK,  COLOR_WHITE);

    win_ = newwin(1, width_, winY_, winX_);
    keypad(win_, TRUE);
}

MenuBar::~MenuBar() {
    closeDropdown();
    if (win_) delwin(win_);
}

void MenuBar::resize(int y, int x, int width) {
    winY_ = y; winX_ = x; width_ = width;
    wresize(win_, 1, width_);
    mvwin(win_, winY_, winX_);
}

void MenuBar::addMenu(const Menu& menu) {
    menus_.push_back(menu);
}

int MenuBar::menuTitleX(int idx) const {
    int x = 1;
    for (int i = 0; i < idx; ++i)
        x += (int)menus_[i].title.size() + 2;
    return x;
}

void MenuBar::draw() {
    werase(win_);
    wbkgd(win_, COLOR_PAIR(COLOR_MENUBAR));
    wattron(win_, COLOR_PAIR(COLOR_MENUBAR));

    for (int i = 0; i < (int)menus_.size(); ++i) {
        int tx = menuTitleX(i);
        if (open_ && i == activeMenu_)
            wattron(win_, A_REVERSE);
        mvwprintw(win_, 0, tx, " %s ", menus_[i].title.c_str());
        if (open_ && i == activeMenu_)
            wattroff(win_, A_REVERSE);
    }

    // Ayuda rápida al final
    std::string hint = " F1:Ayuda ";
    mvwprintw(win_, 0, width_ - (int)hint.size(), "%s", hint.c_str());

    wattroff(win_, COLOR_PAIR(COLOR_MENUBAR));
    wrefresh(win_);

    if (open_) drawDropdown();
}

void MenuBar::openMenu(int idx) {
    closeDropdown();
    activeMenu_ = idx;
    activeItem_ = 0;
    open_       = true;

    const Menu& m    = menus_[idx];
    int itemCount    = (int)m.items.size();
    int maxLabelLen  = 0;
    int maxHintLen   = 0;
    for (auto& item : m.items) {
        maxLabelLen = std::max(maxLabelLen, (int)item.label.size());
        maxHintLen  = std::max(maxHintLen,  (int)item.shortcutHint.size());
    }
    int dropW = maxLabelLen + maxHintLen + 6;
    int dropH = itemCount + 2;

    int startX = menuTitleX(idx);
    if (startX + dropW > width_) startX = width_ - dropW;
    if (startX < 0) startX = 0;

    dropWin_ = newwin(dropH, dropW, winY_ + 1, startX);
    keypad(dropWin_, TRUE);
}

void MenuBar::closeDropdown() {
    if (dropWin_) {
        delwin(dropWin_);
        dropWin_ = nullptr;
    }
    open_ = false;
    activeMenu_ = -1;
}

void MenuBar::drawDropdown() {
    if (!dropWin_ || activeMenu_ < 0) return;
    const Menu& m = menus_[activeMenu_];

    int dropH, dropW;
    getmaxyx(dropWin_, dropH, dropW);

    werase(dropWin_);
    wbkgd(dropWin_, COLOR_PAIR(COLOR_DROPDOWN));
    box(dropWin_, 0, 0);

    for (int i = 0; i < (int)m.items.size(); ++i) {
        bool sel = (i == activeItem_);
        if (sel) wattron(dropWin_, COLOR_PAIR(COLOR_SEL_ITEM));
        else     wattron(dropWin_, COLOR_PAIR(COLOR_DROPDOWN));

        // Separador si label == "---"
        if (m.items[i].label == "---") {
            mvwhline(dropWin_, i + 1, 1, ACS_HLINE, dropW - 2);
        } else {
            mvwprintw(dropWin_, i + 1, 1, " %-*s %s ",
                      dropW - (int)m.items[i].shortcutHint.size() - 5,
                      m.items[i].label.c_str(),
                      m.items[i].shortcutHint.c_str());
        }

        if (sel) wattroff(dropWin_, COLOR_PAIR(COLOR_SEL_ITEM));
        else     wattroff(dropWin_, COLOR_PAIR(COLOR_DROPDOWN));
    }

    wrefresh(dropWin_);
}

void MenuBar::executeItem() {
    if (activeMenu_ < 0 || activeMenu_ >= (int)menus_.size()) return;
    const Menu& m = menus_[activeMenu_];
    if (activeItem_ < 0 || activeItem_ >= (int)m.items.size()) return;
    const MenuItem& item = m.items[activeItem_];
    if (item.label == "---") return; // separador
    closeDropdown();
    if (item.action) item.action();
}

bool MenuBar::handleInput(int ch) {
    if (!open_) {
        // F-keys para abrir menús directamente
        for (int i = 0; i < (int)menus_.size(); ++i) {
            for (auto& item : menus_[i].items) {
                if (item.key != 0 && ch == item.key) {
                    if (item.action) item.action();
                    return true;
                }
            }
        }
        // Alt+letra o F10 para abrir menú
        if (ch == KEY_F(10)) {
            openMenu(0);
            return true;
        }
        return false;
    }

    // Menú abierto
    const Menu& m = menus_[activeMenu_];
    switch (ch) {
    case KEY_LEFT:
        activeMenu_ = (activeMenu_ - 1 + (int)menus_.size()) % (int)menus_.size();
        openMenu(activeMenu_);
        return true;

    case KEY_RIGHT:
        activeMenu_ = (activeMenu_ + 1) % (int)menus_.size();
        openMenu(activeMenu_);
        return true;

    case KEY_UP:
        activeItem_ = (activeItem_ - 1 + (int)m.items.size()) % (int)m.items.size();
        // Saltar separadores
        while (m.items[activeItem_].label == "---")
            activeItem_ = (activeItem_ - 1 + (int)m.items.size()) % (int)m.items.size();
        return true;

    case KEY_DOWN:
        activeItem_ = (activeItem_ + 1) % (int)m.items.size();
        while (m.items[activeItem_].label == "---")
            activeItem_ = (activeItem_ + 1) % (int)m.items.size();
        return true;

    case '\n':
    case KEY_ENTER:
        executeItem();
        return true;

    case 27: // ESC
        closeDropdown();
        return true;

    default:
        // Atajo de letra dentro del menú
        for (int i = 0; i < (int)m.items.size(); ++i) {
            if (!m.items[i].label.empty() &&
                ::tolower(m.items[i].label[0]) == ::tolower(ch)) {
                activeItem_ = i;
                executeItem();
                return true;
            }
        }
        break;
    }
    return true; // Si el menú está abierto, consumir todas las teclas
}

void MenuBar::close() {
    closeDropdown();
}
