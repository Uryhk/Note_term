#pragma once
#include "editor.h"
#include "menubar.h"
#include "statusbar.h"
#include "pluginmanager.h"
#include <string>
#include <memory>

class App {
public:
    App(int argc, char* argv[]);
    ~App();

    void run();

    // Acciones invocadas desde menús o plugins
    void actionNew();
    void actionOpen();
    void actionSave();
    void actionSaveAs();
    void actionSaveFormat();   // elegir formato al guardar
    void actionFind();
    void actionFindReplace();
    void actionGotoLine();
    void actionAbout();
    void actionQuit();

    Editor*  getEditor()  { return editor_.get(); }

private:
    std::unique_ptr<Editor>        editor_;
    std::unique_ptr<MenuBar>       menubar_;
    std::unique_ptr<StatusBar>     statusbar_;
    PluginManager                  pluginMgr_;

    std::string currentFile_;
    std::string currentFormat_; // "txt", "md", "html", "csv"
    bool        running_;

    void buildMenus();
    void buildPluginMenu();
    void handleResize();
    bool confirmUnsaved(); // pregunta si hay cambios sin guardar
};
