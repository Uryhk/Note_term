#pragma once
#include "iplugin.h"
#include <string>
#include <vector>
#include <memory>

struct LoadedPlugin {
    IPlugin*    instance;
    void*       handle;     // dlopen handle
    std::string path;
};

class PluginManager {
public:
    PluginManager() = default;
    ~PluginManager();

    // Carga todos los .so de la carpeta dada
    void loadFromDirectory(const std::string& dir, PluginContext ctx);

    // Carga un .so específico
    bool loadPlugin(const std::string& soPath, PluginContext ctx);

    // Lista de plugins cargados
    const std::vector<LoadedPlugin>& plugins() const { return plugins_; }

    // Disparar hook onSave en todos los plugins
    void notifySave(const std::string& filepath);

    // Disparar hook onOpen en todos los plugins
    void notifyOpen(const std::string& filepath);

    // Recolectar todos los PluginMenuItems de todos los plugins
    std::vector<std::pair<IPlugin*, PluginMenuItem>> collectMenuItems() const;

private:
    std::vector<LoadedPlugin> plugins_;
};
