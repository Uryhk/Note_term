#pragma once
#include <ncurses.h>
#include <string>
#include <vector>
#include <functional>

struct MenuItem {
    std::string label;
    std::string shortcutHint; // ej: "Ctrl+S"
    int         key;          // tecla de atajo o 0
    std::function<void()> action;
};

struct Menu {
    std::string title;
    std::vector<MenuItem> items;
};

class MenuBar {
public:
    MenuBar(int y, int x, int width);
    ~MenuBar();

    void addMenu(const Menu& menu);
    void draw();

    // Devuelve true si el menú consumió la tecla
    bool handleInput(int ch);

    bool isOpen() const { return open_; }
    void close();

    void resize(int y, int x, int width);

private:
    WINDOW* win_;
    int winY_, winX_, width_;

    std::vector<Menu> menus_;
    int  activeMenu_;  // índice del menú desplegado (-1 = ninguno)
    int  activeItem_;  // ítem seleccionado dentro del menú
    bool open_;

    WINDOW* dropWin_;  // ventana del dropdown actual

    void openMenu(int idx);
    void closeDropdown();
    void drawDropdown();
    void executeItem();

    // Calcula posición X donde empieza el título del menú idx
    int menuTitleX(int idx) const;
};
