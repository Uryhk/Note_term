#include "pluginmanager.h"
#include <dlfcn.h>       // dlopen, dlsym, dlclose en Linux
#include <dirent.h>      // opendir / readdir
#include <cstring>
#include <iostream>

PluginManager::~PluginManager() {
    for (auto& lp : plugins_) {
        if (lp.instance) {
            // Intentar obtener la función de destrucción
            DestroyPluginFn destroyFn =
                (DestroyPluginFn)dlsym(lp.handle, "destroyPlugin");
            if (destroyFn) destroyFn(lp.instance);
            else delete lp.instance;
        }
        if (lp.handle) dlclose(lp.handle);
    }
}

bool PluginManager::loadPlugin(const std::string& soPath, PluginContext ctx) {
    void* handle = dlopen(soPath.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cerr << "[PluginManager] No se pudo cargar: " << soPath
                  << " — " << dlerror() << '\n';
        return false;
    }

    CreatePluginFn createFn =
        (CreatePluginFn)dlsym(handle, "createPlugin");
    if (!createFn) {
        std::cerr << "[PluginManager] Símbolo 'createPlugin' no encontrado en "
                  << soPath << '\n';
        dlclose(handle);
        return false;
    }

    IPlugin* plugin = createFn();
    if (!plugin) {
        dlclose(handle);
        return false;
    }

    plugin->initialize(ctx);
    plugins_.push_back({ plugin, handle, soPath });
    return true;
}

void PluginManager::loadFromDirectory(const std::string& dir, PluginContext ctx) {
    DIR* d = opendir(dir.c_str());
    if (!d) return;

    struct dirent* entry;
    while ((entry = readdir(d)) != nullptr) {
        std::string name = entry->d_name;
        // Buscar archivos .so
        if (name.size() > 3 &&
            name.substr(name.size() - 3) == ".so") {
            loadPlugin(dir + "/" + name, ctx);
        }
    }
    closedir(d);
}

void PluginManager::notifySave(const std::string& filepath) {
    for (auto& lp : plugins_)
        if (lp.instance) lp.instance->onSave(filepath);
}

void PluginManager::notifyOpen(const std::string& filepath) {
    for (auto& lp : plugins_)
        if (lp.instance) lp.instance->onOpen(filepath);
}

std::vector<std::pair<IPlugin*, PluginMenuItem>>
PluginManager::collectMenuItems() const {
    std::vector<std::pair<IPlugin*, PluginMenuItem>> result;
    for (auto& lp : plugins_) {
        if (!lp.instance) continue;
        for (auto& mi : lp.instance->menuItems())
            result.push_back({ lp.instance, mi });
    }
    return result;
}
