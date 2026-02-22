#pragma once
#include <ncurses.h>
#include <string>
#include <vector>

class Editor {
public:
    Editor(int y, int x, int height, int width);
    ~Editor();

    void draw();
    void handleInput(int ch);

    // Texto completo como vector de líneas
    std::vector<std::string>& getLines() { return lines_; }
    const std::vector<std::string>& getLines() const { return lines_; }

    // Carga nuevo contenido (reemplaza todo)
    void setLines(const std::vector<std::string>& lines);
    void clear();

    // Posición del cursor (lógica, basada en documento)
    int cursorRow() const { return curRow_; }
    int cursorCol() const { return curCol_; }

    // Indica si hubo cambios desde el último guardado
    bool isDirty() const { return dirty_; }
    void setDirty(bool d) { dirty_ = d; }

    // Redimensionar ventana (para resize de terminal)
    void resize(int y, int x, int height, int width);

    // Insertar texto programáticamente (para plugins)
    void insertText(const std::string& text);

    // Obtener texto completo como string
    std::string getText() const;

    // Buscar y reemplazar
    // Devuelve número de reemplazos realizados
    int findReplace(const std::string& needle,
                    const std::string& replacement,
                    bool caseSensitive,
                    bool replaceAll);

    // Ir a línea específica
    void gotoLine(int line);

private:
    WINDOW* win_;
    int winY_, winX_, height_, width_;

    std::vector<std::string> lines_;
    int curRow_, curCol_;   // posición lógica en el documento
    int viewRow_, viewCol_; // desplazamiento del viewport

    bool dirty_;

    // Helpers
    void moveCursorUp();
    void moveCursorDown();
    void moveCursorLeft();
    void moveCursorRight();
    void insertChar(int ch);
    void deleteCharBack();  // Backspace
    void deleteCharFwd();   // Delete
    void insertNewline();
    void scrollToCursor();
    void clampCursor();

    // Dibuja una línea del documento en la fila visual dada
    void drawLine(int visualRow, int docRow);
};
