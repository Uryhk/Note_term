#include "filemanager.h"
#include <fstream>
#include <sstream>
#include <algorithm>

// ── Cargar archivo ────────────────────────────────────────────────
bool FileManager::load(const std::string& path, std::vector<std::string>& lines) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    lines.clear();
    std::string line;
    while (std::getline(f, line)) {
        // Eliminar \r si el archivo tiene terminaciones CRLF
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(line);
    }
    // Si el archivo estaba vacío, al menos una línea vacía
    if (lines.empty()) lines.push_back("");
    return true;
}

// ── Guardar archivo ────────────────────────────────────────────────
bool FileManager::save(const std::string& path,
                       const std::vector<std::string>& lines,
                       FileFormat fmt) {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    switch (fmt) {

    case FileFormat::TXT:
    case FileFormat::MD:
        // Texto plano — una línea por salto de línea
        for (size_t i = 0; i < lines.size(); ++i) {
            f << lines[i];
            if (i + 1 < lines.size()) f << '\n';
        }
        break;

    case FileFormat::HTML: {
        f << "<!DOCTYPE html>\n<html>\n<head>\n"
          << "<meta charset=\"UTF-8\">\n"
          << "<title>Documento</title>\n"
          << "<style>body{font-family:monospace;white-space:pre-wrap;}</style>\n"
          << "</head>\n<body>\n";
        for (const auto& line : lines) {
            // Escapar caracteres especiales HTML
            std::string escaped;
            for (char c : line) {
                if      (c == '&')  escaped += "&amp;";
                else if (c == '<')  escaped += "&lt;";
                else if (c == '>')  escaped += "&gt;";
                else if (c == '"')  escaped += "&quot;";
                else                escaped += c;
            }
            f << escaped << "<br>\n";
        }
        f << "</body>\n</html>\n";
        break;
    }

    case FileFormat::CSV:
        // Guardar como CSV: cada línea es una fila con una sola columna
        // (el usuario puede editar para añadir más columnas con comas)
        for (const auto& line : lines) {
            // Si la línea contiene comas o comillas, envolver en comillas
            bool needsQuotes = (line.find(',') != std::string::npos ||
                                line.find('"') != std::string::npos ||
                                line.find('\n') != std::string::npos);
            if (needsQuotes) {
                f << '"';
                for (char c : line) {
                    if (c == '"') f << '"'; // escapar comilla doble
                    f << c;
                }
                f << '"';
            } else {
                f << line;
            }
            f << '\n';
        }
        break;
    }

    return true;
}

// ── Detectar formato por extensión ────────────────────────────────
FileFormat FileManager::detectFormat(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot == std::string::npos) return FileFormat::TXT;

    std::string ext = path.substr(dot + 1);
    // Convertir a minúsculas
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "md" || ext == "markdown") return FileFormat::MD;
    if (ext == "html" || ext == "htm")    return FileFormat::HTML;
    if (ext == "csv")                     return FileFormat::CSV;
    return FileFormat::TXT;
}

// ── Nombre base del archivo ────────────────────────────────────────
std::string FileManager::basename(const std::string& path) {
    auto slash = path.rfind('/');
    if (slash == std::string::npos) slash = path.rfind('\\');
    if (slash == std::string::npos) return path;
    return path.substr(slash + 1);
}
