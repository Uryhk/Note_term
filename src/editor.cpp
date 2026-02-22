#include "editor.h"
#include <algorithm>
#include <sstream>

// ── Paleta de colores ─────────────────────────────────────────────
#define COLOR_EDITOR_BG   1
#define COLOR_CURSOR_LINE 2

// ── Constructor / Destructor ──────────────────────────────────────
Editor::Editor(int y, int x, int height, int width)
    : winY_(y), winX_(x), height_(height), width_(width),
      curRow_(0), curCol_(0), viewRow_(0), viewCol_(0), dirty_(false)
{
    init_pair(COLOR_EDITOR_BG,   COLOR_WHITE,  COLOR_BLACK);
    init_pair(COLOR_CURSOR_LINE, COLOR_BLACK,  COLOR_WHITE);

    win_ = newwin(height_, width_, winY_, winX_);
    keypad(win_, TRUE);
    lines_.push_back(""); // documento vacío tiene al menos una línea
}

Editor::~Editor() {
    if (win_) delwin(win_);
}

// ── Resize ────────────────────────────────────────────────────────
void Editor::resize(int y, int x, int height, int width) {
    winY_ = y; winX_ = x; height_ = height; width_ = width;
    wresize(win_, height_, width_);
    mvwin(win_, winY_, winX_);
    scrollToCursor();
}

// ── Dibujo ────────────────────────────────────────────────────────
void Editor::draw() {
    werase(win_);
    wbkgd(win_, COLOR_PAIR(COLOR_EDITOR_BG));

    for (int vr = 0; vr < height_; ++vr) {
        int dr = vr + viewRow_;
        if (dr < (int)lines_.size()) {
            drawLine(vr, dr);
        }
    }

    // Posicionar cursor físico
    int cy = curRow_ - viewRow_;
    int cx = curCol_ - viewCol_;
    if (cy >= 0 && cy < height_ && cx >= 0 && cx < width_) {
        wmove(win_, cy, cx);
    }

    wrefresh(win_);
}

void Editor::drawLine(int visualRow, int docRow) {
    wmove(win_, visualRow, 0);
    wclrtoeol(win_);

    bool isCursorLine = (docRow == curRow_);

    if (isCursorLine)
        wattron(win_, COLOR_PAIR(COLOR_CURSOR_LINE));
    else
        wattron(win_, COLOR_PAIR(COLOR_EDITOR_BG));

    const std::string& line = lines_[docRow];
    int startCol = viewCol_;
    int endCol   = std::min((int)line.size(), viewCol_ + width_);

    for (int c = startCol; c < endCol; ++c) {
        waddch(win_, (unsigned char)line[c]);
    }
    // Rellenar resto de la línea con espacios para resaltar línea cursor
    int printed = endCol - startCol;
    for (int p = printed; p < width_; ++p) waddch(win_, ' ');

    if (isCursorLine)
        wattroff(win_, COLOR_PAIR(COLOR_CURSOR_LINE));
    else
        wattroff(win_, COLOR_PAIR(COLOR_EDITOR_BG));
}

// ── Manejo de Input ───────────────────────────────────────────────
void Editor::handleInput(int ch) {
    switch (ch) {
    case KEY_UP:    moveCursorUp();    break;
    case KEY_DOWN:  moveCursorDown();  break;
    case KEY_LEFT:  moveCursorLeft();  break;
    case KEY_RIGHT: moveCursorRight(); break;

    case KEY_HOME:
        curCol_ = 0;
        viewCol_ = 0;
        break;

    case KEY_END:
        curCol_ = (int)lines_[curRow_].size();
        scrollToCursor();
        break;

    case KEY_PPAGE: // Page Up
        curRow_ = std::max(0, curRow_ - (height_ - 1));
        clampCursor();
        scrollToCursor();
        break;

    case KEY_NPAGE: // Page Down
        curRow_ = std::min((int)lines_.size() - 1, curRow_ + (height_ - 1));
        clampCursor();
        scrollToCursor();
        break;

    case KEY_BACKSPACE:
    case 127:
    case '\b':
        deleteCharBack();
        break;

    case KEY_DC: // Delete
        deleteCharFwd();
        break;

    case '\n':
    case KEY_ENTER:
        insertNewline();
        break;

    case KEY_BTAB: // Shift+Tab — no acción en editor básico
        break;

    case '\t':
        // Insertar 4 espacios como tab
        for (int i = 0; i < 4; ++i) insertChar(' ');
        break;

    default:
        if (ch >= 32 && ch < 256) { // caracteres imprimibles
            insertChar(ch);
        }
        break;
    }
}

// ── Movimiento del cursor ─────────────────────────────────────────
void Editor::moveCursorUp() {
    if (curRow_ > 0) {
        --curRow_;
        clampCursor();
        scrollToCursor();
    }
}

void Editor::moveCursorDown() {
    if (curRow_ < (int)lines_.size() - 1) {
        ++curRow_;
        clampCursor();
        scrollToCursor();
    }
}

void Editor::moveCursorLeft() {
    if (curCol_ > 0) {
        --curCol_;
    } else if (curRow_ > 0) {
        --curRow_;
        curCol_ = (int)lines_[curRow_].size();
    }
    scrollToCursor();
}

void Editor::moveCursorRight() {
    int lineLen = (int)lines_[curRow_].size();
    if (curCol_ < lineLen) {
        ++curCol_;
    } else if (curRow_ < (int)lines_.size() - 1) {
        ++curRow_;
        curCol_ = 0;
    }
    scrollToCursor();
}

// ── Edición ───────────────────────────────────────────────────────
void Editor::insertChar(int ch) {
    lines_[curRow_].insert(curCol_, 1, (char)ch);
    ++curCol_;
    dirty_ = true;
    scrollToCursor();
}

void Editor::deleteCharBack() {
    if (curCol_ > 0) {
        lines_[curRow_].erase(curCol_ - 1, 1);
        --curCol_;
        dirty_ = true;
    } else if (curRow_ > 0) {
        // Unir con línea anterior
        int prevLen = (int)lines_[curRow_ - 1].size();
        lines_[curRow_ - 1] += lines_[curRow_];
        lines_.erase(lines_.begin() + curRow_);
        --curRow_;
        curCol_ = prevLen;
        dirty_ = true;
    }
    scrollToCursor();
}

void Editor::deleteCharFwd() {
    int lineLen = (int)lines_[curRow_].size();
    if (curCol_ < lineLen) {
        lines_[curRow_].erase(curCol_, 1);
        dirty_ = true;
    } else if (curRow_ < (int)lines_.size() - 1) {
        // Unir con línea siguiente
        lines_[curRow_] += lines_[curRow_ + 1];
        lines_.erase(lines_.begin() + curRow_ + 1);
        dirty_ = true;
    }
}

void Editor::insertNewline() {
    std::string rest = lines_[curRow_].substr(curCol_);
    lines_[curRow_] = lines_[curRow_].substr(0, curCol_);
    lines_.insert(lines_.begin() + curRow_ + 1, rest);
    ++curRow_;
    curCol_ = 0;
    dirty_ = true;
    scrollToCursor();
}

// ── Scroll ────────────────────────────────────────────────────────
void Editor::scrollToCursor() {
    // Vertical
    if (curRow_ < viewRow_)
        viewRow_ = curRow_;
    else if (curRow_ >= viewRow_ + height_)
        viewRow_ = curRow_ - height_ + 1;

    // Horizontal
    if (curCol_ < viewCol_)
        viewCol_ = curCol_;
    else if (curCol_ >= viewCol_ + width_)
        viewCol_ = curCol_ - width_ + 1;
}

void Editor::clampCursor() {
    if (curRow_ < 0) curRow_ = 0;
    if (curRow_ >= (int)lines_.size())
        curRow_ = (int)lines_.size() - 1;
    int lineLen = (int)lines_[curRow_].size();
    if (curCol_ > lineLen) curCol_ = lineLen;
}

// ── API pública ────────────────────────────────────────────────────
void Editor::setLines(const std::vector<std::string>& lines) {
    lines_ = lines;
    if (lines_.empty()) lines_.push_back("");
    curRow_ = 0; curCol_ = 0;
    viewRow_ = 0; viewCol_ = 0;
    dirty_ = false;
}

void Editor::clear() {
    lines_.clear();
    lines_.push_back("");
    curRow_ = 0; curCol_ = 0;
    viewRow_ = 0; viewCol_ = 0;
    dirty_ = false;
}

void Editor::insertText(const std::string& text) {
    for (char c : text) {
        if (c == '\n') insertNewline();
        else           insertChar((unsigned char)c);
    }
}

std::string Editor::getText() const {
    std::ostringstream oss;
    for (size_t i = 0; i < lines_.size(); ++i) {
        if (i) oss << '\n';
        oss << lines_[i];
    }
    return oss.str();
}

void Editor::gotoLine(int line) {
    line = std::max(1, std::min(line, (int)lines_.size()));
    curRow_ = line - 1;
    curCol_ = 0;
    scrollToCursor();
}

int Editor::findReplace(const std::string& needle,
                        const std::string& replacement,
                        bool caseSensitive,
                        bool replaceAll) {
    if (needle.empty()) return 0;
    int count = 0;

    auto strFind = [&](const std::string& haystack, size_t from) -> size_t {
        if (caseSensitive) {
            return haystack.find(needle, from);
        } else {
            std::string h = haystack, n = needle;
            std::transform(h.begin(), h.end(), h.begin(), ::tolower);
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);
            return h.find(n, from);
        }
    };

    for (int r = 0; r < (int)lines_.size(); ++r) {
        size_t pos = 0;
        while ((pos = strFind(lines_[r], pos)) != std::string::npos) {
            lines_[r].replace(pos, needle.size(), replacement);
            pos += replacement.size();
            ++count;
            dirty_ = true;
            if (!replaceAll) {
                // Mover cursor al primer resultado
                curRow_ = r;
                curCol_ = (int)pos - (int)replacement.size();
                scrollToCursor();
                return count;
            }
        }
    }
    if (count > 0 && replaceAll) {
        curRow_ = 0; curCol_ = 0;
        scrollToCursor();
    }
    return count;
}
