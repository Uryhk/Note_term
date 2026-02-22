#pragma once
#include <string>
#include <vector>

enum class FileFormat {
    TXT,
    MD,
    HTML,
    CSV
};

class FileManager {
public:
    // Lee el archivo y devuelve sus líneas
    static bool load(const std::string& path, std::vector<std::string>& lines);

    // Guarda las líneas en disco según el formato elegido
    static bool save(const std::string& path,
                     const std::vector<std::string>& lines,
                     FileFormat fmt = FileFormat::TXT);

    // Inferir formato según extensión del path
    static FileFormat detectFormat(const std::string& path);

    // Devuelve el nombre del archivo sin ruta
    static std::string basename(const std::string& path);
};
