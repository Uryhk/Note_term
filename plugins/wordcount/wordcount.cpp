// Plugin de ejemplo: cuenta palabras y caracteres del documento actual
// Compilar: g++ -shared -fPIC -o wordcount.so wordcount.cpp -I../../include

#include "../../include/iplugin.h"
#include "../../include/editor.h"
#include <ncurses.h>
#include <sstream>
#include <string>
#include <vector>

class WordCountPlugin : public IPlugin {
public:
    std::string name()        const override { return "WordCount"; }
    std::string version()     const override { return "1.0"; }
    std::string description() const override {
        return "Cuenta palabras y caracteres del documento.";
    }

    void initialize(PluginContext ctx) override {
        ctx_ = ctx;
    }

    std::vector<PluginMenuItem> menuItems() const override {
        return {{ "Contar palabras", "Plugins", 0 }};
    }

    void execute(const std::string& /*action*/) override {
        if (!ctx_.editor) return;

        const auto& lines = ctx_.editor->getLines();
        int chars = 0, words = 0, linesCount = (int)lines.size();

        for (const auto& line : lines) {
            chars += (int)line.size();
            bool inWord = false;
            for (char c : line) {
                if (c == ' ' || c == '\t') {
                    inWord = false;
                } else {
                    if (!inWord) { ++words; inWord = true; }
                }
            }
        }

        // Mostrar resultado en un diálogo simple de ncurses
        int h = 8, w = 40;
        int sy, sx;
        getmaxyx(stdscr, sy, sx);
        WINDOW* win = newwin(h, w, (sy - h) / 2, (sx - w) / 2);
        keypad(win, TRUE);
        box(win, 0, 0);
        mvwprintw(win, 0, (w - 14) / 2, " Word Count ");
        mvwprintw(win, 2, 4, "Líneas    : %d", linesCount);
        mvwprintw(win, 3, 4, "Palabras  : %d", words);
        mvwprintw(win, 4, 4, "Caracteres: %d", chars);
        mvwprintw(win, 6, (w - 18) / 2, "[ Presiona una tecla ]");
        wrefresh(win);
        wgetch(win);
        delwin(win);
        touchwin(stdscr);
        refresh();
    }

private:
    PluginContext ctx_{};
};

// ── Funciones exportadas ──────────────────────────────────────────
extern "C" {
    IPlugin* createPlugin()          { return new WordCountPlugin(); }
    void     destroyPlugin(IPlugin* p) { delete p; }
}
