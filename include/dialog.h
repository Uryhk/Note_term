#pragma once
#include <ncurses.h>
#include <string>
#include <vector>

// ─── Diálogo de entrada de texto simple ───────────────────────────
// Devuelve true si el usuario confirmó (Enter), false si canceló (Esc)
bool dialogInput(const std::string& title,
                 const std::string& prompt,
                 std::string& result,
                 int maxLen = 255);

// ─── Diálogo de guardar / abrir (ingresa ruta manualmente) ────────
bool dialogFilePath(const std::string& title,
                    std::string& path);

// ─── Diálogo de selección de formato ─────────────────────────────
// options: lista de strings, selectedIndex: índice elegido
bool dialogChoose(const std::string& title,
                  const std::vector<std::string>& options,
                  int& selectedIndex);

// ─── Diálogo de buscar y reemplazar ──────────────────────────────
struct FindReplaceParams {
    std::string needle;
    std::string replacement;
    bool caseSensitive = false;
    bool replaceAll    = false;
};
bool dialogFindReplace(FindReplaceParams& params);

// ─── Diálogo de ir a línea ────────────────────────────────────────
bool dialogGotoLine(int maxLine, int& targetLine);

// ─── Mensaje de alerta / confirmación ────────────────────────────
void dialogAlert(const std::string& title, const std::string& msg);
bool dialogConfirm(const std::string& title, const std::string& msg);
