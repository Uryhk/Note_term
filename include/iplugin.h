#pragma once
#include <string>
#include <vector>

// Forward declarations
class Editor;
class App;

// ─────────────────────────────────────────────
//  Contexto que el plugin recibe para operar
// ─────────────────────────────────────────────
struct PluginContext {
    Editor* editor;   // acceso al editor principal
    App*    app;      // acceso a la aplicación
};

// ─────────────────────────────────────────────
//  Entrada de menú que el plugin puede registrar
// ─────────────────────────────────────────────
struct PluginMenuItem {
    std::string label;       // texto que aparece en el menú
    std::string category;    // "Plugins" u otro grupo
    int         shortcut;    // tecla (ej: KEY_F5) o 0 si ninguna
};

// ─────────────────────────────────────────────
//  Interfaz que todo plugin DEBE implementar
// ─────────────────────────────────────────────
class IPlugin {
public:
    virtual ~IPlugin() = default;

    // Nombre único del plugin
    virtual std::string name()    const = 0;
    // Versión legible
    virtual std::string version() const = 0;
    // Descripción breve
    virtual std::string description() const = 0;

    // Llamado una vez al cargar el plugin
    virtual void initialize(PluginContext ctx) = 0;

    // Entradas de menú que este plugin añade
    virtual std::vector<PluginMenuItem> menuItems() const = 0;

    // Ejecutar la acción del ítem de menú indicado por su label
    virtual void execute(const std::string& actionLabel) = 0;

    // Llamado cuando se guarda un archivo (hook opcional)
    virtual void onSave(const std::string& /*filepath*/) {}

    // Llamado cuando se abre un archivo (hook opcional)
    virtual void onOpen(const std::string& /*filepath*/) {}
};

// ─────────────────────────────────────────────
//  Funciones que la .so/.dll DEBE exportar
// ─────────────────────────────────────────────
extern "C" {
    typedef IPlugin* (*CreatePluginFn)();
    typedef void     (*DestroyPluginFn)(IPlugin*);
}
