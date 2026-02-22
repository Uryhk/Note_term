#include "dialog.h"
#include <algorithm>
#include <cstring>

// ── Helpers internos ──────────────────────────────────────────────

static void drawBox(WINDOW* w, const std::string& title) {
    box(w, 0, 0);
    if (!title.empty()) {
        int wy, wx;
        getmaxyx(w, wy, wx);
        (void)wy;
        int tx = (wx - (int)title.size() - 2) / 2;
        if (tx < 1) tx = 1;
        mvwprintw(w, 0, tx, " %s ", title.c_str());
    }
}

// Crear ventana centrada
static WINDOW* centeredWin(int h, int w) {
    int sy, sx;
    getmaxyx(stdscr, sy, sx);
    int y = (sy - h) / 2;
    int x = (sx - w) / 2;
    if (y < 0) y = 0;
    if (x < 0) x = 0;
    WINDOW* win = newwin(h, w, y, x);
    keypad(win, TRUE);
    return win;
}

// ── dialogInput ───────────────────────────────────────────────────
bool dialogInput(const std::string& title,
                 const std::string& prompt,
                 std::string& result,
                 int maxLen)
{
    int w = std::max(50, (int)prompt.size() + 6);
    int h = 5;
    WINDOW* win = centeredWin(h, w);
    drawBox(win, title);
    mvwprintw(win, 1, 2, "%s", prompt.c_str());

    // Campo de entrada
    std::string buf = result;
    int cursor = (int)buf.size();
    bool running = true;
    bool confirmed = false;

    while (running) {
        // Dibujar campo
        mvwprintw(win, 2, 2, "%*s", w - 4, ""); // limpiar
        // Mostrar solo los últimos (w-4) caracteres
        int dispStart = std::max(0, cursor - (w - 5));
        std::string disp = buf.substr(dispStart);
        if ((int)disp.size() > w - 4) disp = disp.substr(0, w - 4);
        mvwprintw(win, 2, 2, "%s", disp.c_str());
        // Mover cursor visual
        wmove(win, 2, 2 + (cursor - dispStart));
        wrefresh(win);

        int ch = wgetch(win);
        switch (ch) {
        case 27: // ESC
            running = false;
            confirmed = false;
            break;
        case '\n':
        case KEY_ENTER:
            running = false;
            confirmed = true;
            break;
        case KEY_BACKSPACE:
        case 127:
        case '\b':
            if (cursor > 0) {
                buf.erase(cursor - 1, 1);
                --cursor;
            }
            break;
        case KEY_DC:
            if (cursor < (int)buf.size())
                buf.erase(cursor, 1);
            break;
        case KEY_LEFT:
            if (cursor > 0) --cursor;
            break;
        case KEY_RIGHT:
            if (cursor < (int)buf.size()) ++cursor;
            break;
        case KEY_HOME:
            cursor = 0;
            break;
        case KEY_END:
            cursor = (int)buf.size();
            break;
        default:
            if (ch >= 32 && ch < 256 && (int)buf.size() < maxLen) {
                buf.insert(cursor, 1, (char)ch);
                ++cursor;
            }
            break;
        }
    }

    delwin(win);
    touchwin(stdscr);
    refresh();

    if (confirmed) result = buf;
    return confirmed;
}

// ── dialogFilePath ────────────────────────────────────────────────
bool dialogFilePath(const std::string& title, std::string& path) {
    return dialogInput(title, "Ruta del archivo:", path, 512);
}

// ── dialogChoose ──────────────────────────────────────────────────
bool dialogChoose(const std::string& title,
                  const std::vector<std::string>& options,
                  int& selectedIndex)
{
    if (options.empty()) return false;

    int w = 40;
    for (auto& o : options)
        if ((int)o.size() + 4 > w) w = (int)o.size() + 4;

    int h = (int)options.size() + 4;
    WINDOW* win = centeredWin(h, w);
    drawBox(win, title);

    int sel = selectedIndex >= 0 && selectedIndex < (int)options.size()
              ? selectedIndex : 0;
    bool running = true;
    bool confirmed = false;

    while (running) {
        for (int i = 0; i < (int)options.size(); ++i) {
            if (i == sel) wattron(win, A_REVERSE);
            mvwprintw(win, i + 2, 2, " %-*s ", w - 5, options[i].c_str());
            if (i == sel) wattroff(win, A_REVERSE);
        }
        mvwprintw(win, h - 1, 2, "[Enter] OK  [Esc] Cancelar");
        wrefresh(win);

        int ch = wgetch(win);
        switch (ch) {
        case 27: running = false; confirmed = false; break;
        case '\n': case KEY_ENTER: running = false; confirmed = true; break;
        case KEY_UP:   if (sel > 0) --sel; break;
        case KEY_DOWN: if (sel < (int)options.size() - 1) ++sel; break;
        }
    }

    delwin(win);
    touchwin(stdscr);
    refresh();

    if (confirmed) selectedIndex = sel;
    return confirmed;
}

// ── dialogFindReplace ─────────────────────────────────────────────
bool dialogFindReplace(FindReplaceParams& params) {
    int w = 60, h = 10;
    WINDOW* win = centeredWin(h, w);
    drawBox(win, "Buscar y Reemplazar");

    mvwprintw(win, 1, 2, "Buscar   :");
    mvwprintw(win, 3, 2, "Reemplaz.:");
    mvwprintw(win, 5, 2, "[M] May/Min: %s", params.caseSensitive ? "SI" : "NO");
    mvwprintw(win, 6, 2, "[A] Todo   : %s", params.replaceAll   ? "SI" : "NO");
    mvwprintw(win, 8, 2, "[Enter] Buscar/Reemplazar  [Esc] Cancelar");

    // Campo activo: 0 = needle, 1 = replacement
    int field = 0;
    std::string bufs[2] = { params.needle, params.replacement };
    int cursors[2] = { (int)bufs[0].size(), (int)bufs[1].size() };
    bool running = true;
    bool confirmed = false;

    auto drawField = [&](int f) {
        int row = (f == 0) ? 1 : 3;
        bool active = (f == field);
        if (active) wattron(win, A_UNDERLINE);
        mvwprintw(win, row, 13, "%-*s", w - 16, bufs[f].c_str());
        if (active) {
            wmove(win, row, 13 + cursors[f]);
            wattroff(win, A_UNDERLINE);
        }
    };

    while (running) {
        drawField(0);
        drawField(1);
        mvwprintw(win, 5, 2, "[M] May/Min: %s", params.caseSensitive ? "SI" : "NO");
        mvwprintw(win, 6, 2, "[A] Todo   : %s", params.replaceAll   ? "SI" : "NO");
        // Cursor en campo activo
        wmove(win, (field == 0 ? 1 : 3), 13 + cursors[field]);
        wrefresh(win);

        int ch = wgetch(win);
        switch (ch) {
        case 27: running = false; confirmed = false; break;
        case '\n': case KEY_ENTER: running = false; confirmed = true; break;
        case '\t': field = 1 - field; break;
        case 'm': case 'M': params.caseSensitive = !params.caseSensitive; break;
        case 'a': case 'A': params.replaceAll    = !params.replaceAll;    break;
        case KEY_BACKSPACE: case 127: case '\b':
            if (cursors[field] > 0) {
                bufs[field].erase(cursors[field] - 1, 1);
                --cursors[field];
            }
            break;
        case KEY_DC:
            if (cursors[field] < (int)bufs[field].size())
                bufs[field].erase(cursors[field], 1);
            break;
        case KEY_LEFT:  if (cursors[field] > 0) --cursors[field]; break;
        case KEY_RIGHT: if (cursors[field] < (int)bufs[field].size()) ++cursors[field]; break;
        default:
            if (ch >= 32 && ch < 256) {
                bufs[field].insert(cursors[field], 1, (char)ch);
                ++cursors[field];
            }
            break;
        }
    }

    delwin(win);
    touchwin(stdscr);
    refresh();

    if (confirmed) {
        params.needle      = bufs[0];
        params.replacement = bufs[1];
    }
    return confirmed;
}

// ── dialogGotoLine ────────────────────────────────────────────────
bool dialogGotoLine(int maxLine, int& targetLine) {
    std::string val = std::to_string(targetLine);
    std::string prompt = "Ir a línea (1-" + std::to_string(maxLine) + "):";
    if (!dialogInput("Ir a Línea", prompt, val, 10)) return false;
    try {
        int n = std::stoi(val);
        if (n >= 1 && n <= maxLine) {
            targetLine = n;
            return true;
        }
    } catch (...) {}
    return false;
}

// ── dialogAlert ───────────────────────────────────────────────────
void dialogAlert(const std::string& title, const std::string& msg) {
    int w = std::max(40, (int)msg.size() + 6);
    int h = 5;
    WINDOW* win = centeredWin(h, w);
    drawBox(win, title);
    mvwprintw(win, 1, 2, "%s", msg.c_str());
    mvwprintw(win, 3, (w - 14) / 2, "[ Enter/Esc ]");
    wrefresh(win);
    int ch;
    do { ch = wgetch(win); } while (ch != '\n' && ch != 27 && ch != ' ');
    delwin(win);
    touchwin(stdscr);
    refresh();
}

// ── dialogConfirm ─────────────────────────────────────────────────
bool dialogConfirm(const std::string& title, const std::string& msg) {
    int w = std::max(50, (int)msg.size() + 6);
    int h = 6;
    WINDOW* win = centeredWin(h, w);
    drawBox(win, title);
    mvwprintw(win, 1, 2, "%s", msg.c_str());
    mvwprintw(win, 3, 2, "[S] Sí     [N] No");
    wrefresh(win);
    int ch;
    do { ch = wgetch(win); } while (ch != 's' && ch != 'S' &&
                                     ch != 'n' && ch != 'N' && ch != 27);
    delwin(win);
    touchwin(stdscr);
    refresh();
    return (ch == 's' || ch == 'S');
}
