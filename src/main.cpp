#include <ncurses.h>
#include <cstdio>
#include "app.h"

int main(int argc, char* argv[]) {
    // ── 1. Inicializar ncurses ──────────────────────────────────
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    mousemask(0, nullptr);  // sin mouse por ahora
    start_color();
    use_default_colors();
    curs_set(1);

    // ── 2. Verificar tamaño mínimo de terminal ──────────────────
    if (LINES < 10 || COLS < 40) {
        endwin();
        fprintf(stderr, "Terminal demasiado pequeña. Mínimo 40 columnas x 10 filas.\n");
        return 1;
    }

    // ── 3. Crear app y correr ────────────────────────────────────
    {
        App app(argc, argv);
        app.run();
    }

    // ── 4. Restaurar terminal ────────────────────────────────────
    endwin();
    return 0;
}
