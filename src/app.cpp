#include "app.h"
#include "dialog.h"
#include "filemanager.h"
#include <ncurses.h>
#include <algorithm>
#include <filesystem>

// ── Constructor ───────────────────────────────────────────────────
App::App(int argc, char* argv[])
    : currentFormat_("txt"), running_(true)
{
    // Crear las tres zonas de pantalla
    int editorH = LINES - 2; // menos menubar y statusbar

    menubar_   = std::make_unique<MenuBar>(0, 0, COLS);
    editor_    = std::make_unique<Editor>(1, 0, editorH, COLS);
    statusbar_ = std::make_unique<StatusBar>(LINES - 1, 0, COLS);

    buildMenus();

    // Cargar plugins desde carpeta ./plugins
    PluginContext ctx{ editor_.get(), this };
    pluginMgr_.loadFromDirectory("./plugins", ctx);
    buildPluginMenu(); // añadir menú de plugins si los hay

    // Si se pasó un archivo como argumento, abrirlo
    if (argc > 1) {
        currentFile_ = argv[1];
        std::vector<std::string> lines;
        if (FileManager::load(currentFile_, lines)) {
            editor_->setLines(lines);
            pluginMgr_.notifyOpen(currentFile_);
        }
    }
}

App::~App() {}

// ── Bucle principal ───────────────────────────────────────────────
void App::run() {
    while (running_) {
        // Dibujar todo
        menubar_->draw();
        editor_->draw();
        statusbar_->draw(
            editor_->cursorRow(),
            editor_->cursorCol(),
            currentFile_.empty()
                ? "[Sin título]"
                : FileManager::basename(currentFile_),
            editor_->isDirty(),
            (int)editor_->getLines().size()
        );

        // Actualizar posición física del cursor al editor
        // (ya lo hace editor_->draw() pero nos aseguramos)
        doupdate();

        // Leer tecla
        int ch = getch();

        // Resize de terminal
        if (ch == KEY_RESIZE) {
            handleResize();
            continue;
        }

        // F1 = ayuda
        if (ch == KEY_F(1)) {
            actionAbout();
            continue;
        }

        // Ctrl+S = guardar rápido
        if (ch == ('s' & 0x1f)) { actionSave(); continue; }
        // Ctrl+O = abrir
        if (ch == ('o' & 0x1f)) { actionOpen(); continue; }
        // Ctrl+N = nuevo
        if (ch == ('n' & 0x1f)) { actionNew(); continue; }
        // Ctrl+Q = salir
        if (ch == ('q' & 0x1f)) { actionQuit(); continue; }
        // Ctrl+F = buscar/reemplazar
        if (ch == ('f' & 0x1f)) { actionFindReplace(); continue; }
        // Ctrl+G = ir a línea
        if (ch == ('g' & 0x1f)) { actionGotoLine(); continue; }

        // F10 o ESC con menú cerrado = abrir menú
        if (ch == KEY_F(10) || (ch == 27 && !menubar_->isOpen())) {
            menubar_->handleInput(ch);
            continue;
        }

        // Si el menú está abierto, darle prioridad
        if (menubar_->isOpen()) {
            menubar_->handleInput(ch);
            continue;
        }

        // Resto va al editor
        editor_->handleInput(ch);
    }
}

// ── Construcción de menús ─────────────────────────────────────────
void App::buildMenus() {
    // ── Menú Archivo ──────────────────────────────────────────────
    Menu archivo;
    archivo.title = "Archivo";
    archivo.items = {
        { "Nuevo",           "Ctrl+N", 0, [this]{ actionNew(); } },
        { "Abrir...",        "Ctrl+O", 0, [this]{ actionOpen(); } },
        { "---",             "",       0, nullptr },
        { "Guardar",         "Ctrl+S", 0, [this]{ actionSave(); } },
        { "Guardar como...", "",       0, [this]{ actionSaveAs(); } },
        { "Guardar formato", "",       0, [this]{ actionSaveFormat(); } },
        { "---",             "",       0, nullptr },
        { "Salir",           "Ctrl+Q", 0, [this]{ actionQuit(); } },
    };
    menubar_->addMenu(archivo);

    // ── Menú Editar ───────────────────────────────────────────────
    Menu editar;
    editar.title = "Editar";
    editar.items = {
        { "Buscar/Reemplazar", "Ctrl+F", 0, [this]{ actionFindReplace(); } },
        { "Ir a línea...",     "Ctrl+G", 0, [this]{ actionGotoLine(); } },
    };
    menubar_->addMenu(editar);

    // ── Menú Ayuda ────────────────────────────────────────────────
    Menu ayuda;
    ayuda.title = "Ayuda";
    ayuda.items = {
        { "Acerca de", "F1", KEY_F(1), [this]{ actionAbout(); } },
    };
    menubar_->addMenu(ayuda);
}

void App::buildPluginMenu() {
    auto items = pluginMgr_.collectMenuItems();
    if (items.empty()) return;

    Menu pm;
    pm.title = "Plugins";
    for (auto& [plugin, mi] : items) {
        IPlugin* p = plugin;
        std::string label = mi.label;
        MenuItem mitem;
        mitem.label        = label;
        mitem.shortcutHint = "";
        mitem.key          = mi.shortcut;
        mitem.action       = [p, label]{ p->execute(label); };
        pm.items.push_back(mitem);
    }
    menubar_->addMenu(pm);
}

// ── Acciones ──────────────────────────────────────────────────────
void App::actionNew() {
    if (!confirmUnsaved()) return;
    editor_->clear();
    currentFile_   = "";
    currentFormat_ = "txt";
    statusbar_->showMessage("Nuevo documento creado.");
}

void App::actionOpen() {
    if (!confirmUnsaved()) return;

    std::string path;
    if (!dialogFilePath("Abrir archivo", path)) return;

    std::vector<std::string> lines;
    if (!FileManager::load(path, lines)) {
        dialogAlert("Error", "No se pudo abrir el archivo.");
        return;
    }

    editor_->setLines(lines);
    currentFile_ = path;

    FileFormat fmt = FileManager::detectFormat(path);
    switch (fmt) {
    case FileFormat::MD:   currentFormat_ = "md";   break;
    case FileFormat::HTML: currentFormat_ = "html"; break;
    case FileFormat::CSV:  currentFormat_ = "csv";  break;
    default:               currentFormat_ = "txt";  break;
    }

    pluginMgr_.notifyOpen(path);
    statusbar_->showMessage("Archivo abierto: " + FileManager::basename(path));
}

void App::actionSave() {
    if (currentFile_.empty()) {
        actionSaveAs();
        return;
    }
    FileFormat fmt = FileManager::detectFormat(currentFile_);
    if (!FileManager::save(currentFile_, editor_->getLines(), fmt)) {
        dialogAlert("Error", "No se pudo guardar el archivo.");
        return;
    }
    editor_->setDirty(false);
    pluginMgr_.notifySave(currentFile_);
    statusbar_->showMessage("Guardado: " + FileManager::basename(currentFile_));
}

void App::actionSaveAs() {
    std::string path = currentFile_;
    if (!dialogFilePath("Guardar como", path)) return;

    FileFormat fmt = FileManager::detectFormat(path);
    if (!FileManager::save(path, editor_->getLines(), fmt)) {
        dialogAlert("Error", "No se pudo guardar el archivo.");
        return;
    }
    currentFile_ = path;
    editor_->setDirty(false);
    pluginMgr_.notifySave(path);
    statusbar_->showMessage("Guardado como: " + FileManager::basename(path));
}

void App::actionSaveFormat() {
    std::vector<std::string> formats = {
        "TXT  - Texto plano (.txt)",
        "MD   - Markdown (.md)",
        "HTML - Página web (.html)",
        "CSV  - Valores separados (.csv)"
    };
    int sel = 0;
    if (!dialogChoose("Elegir formato de guardado", formats, sel)) return;

    // Pedir nombre de archivo
    std::string extensions[] = { ".txt", ".md", ".html", ".csv" };
    FileFormat fmts[] = {
        FileFormat::TXT, FileFormat::MD,
        FileFormat::HTML, FileFormat::CSV
    };

    std::string path = currentFile_;
    // Sugerir extensión correcta
    if (path.empty()) path = "documento" + extensions[sel];

    if (!dialogFilePath("Guardar en formato", path)) return;

    if (!FileManager::save(path, editor_->getLines(), fmts[sel])) {
        dialogAlert("Error", "No se pudo guardar.");
        return;
    }
    currentFile_ = path;
    editor_->setDirty(false);
    statusbar_->showMessage("Exportado como: " + FileManager::basename(path));
}

void App::actionFind() {
    actionFindReplace();
}

void App::actionFindReplace() {
    FindReplaceParams p;
    if (!dialogFindReplace(p)) return;
    if (p.needle.empty()) return;

    int count = editor_->findReplace(
        p.needle, p.replacement, p.caseSensitive, p.replaceAll);

    if (count == 0) {
        statusbar_->showMessage("No se encontró: " + p.needle);
    } else {
        statusbar_->showMessage(
            std::to_string(count) + " reemplazo(s) realizado(s).");
    }
}

void App::actionGotoLine() {
    int target = editor_->cursorRow() + 1;
    int maxLine = (int)editor_->getLines().size();
    if (!dialogGotoLine(maxLine, target)) return;
    editor_->gotoLine(target);
}

void App::actionAbout() {
    dialogAlert("Acerca de NotepadTUI",
        "NotepadTUI v1.0\n"
        "Editor de texto TUI en C++ + ncurses\n\n"
        "Atajos:\n"
        "  Ctrl+N Nuevo    Ctrl+O Abrir\n"
        "  Ctrl+S Guardar  Ctrl+Q Salir\n"
        "  Ctrl+F Buscar   Ctrl+G Ir a línea\n"
        "  F10    Menú");
}

void App::actionQuit() {
    if (!confirmUnsaved()) return;
    running_ = false;
}

// ── Helpers ───────────────────────────────────────────────────────
bool App::confirmUnsaved() {
    if (!editor_->isDirty()) return true;
    return dialogConfirm("Cambios sin guardar",
        "Hay cambios sin guardar. ¿Continuar y descartar?");
}

void App::handleResize() {
    endwin();
    refresh();
    clear();

    int editorH = LINES - 2;

    menubar_->resize(0, 0, COLS);
    editor_->resize(1, 0, editorH, COLS);
    statusbar_->resize(LINES - 1, 0, COLS);

    clearok(stdscr, TRUE);
    refresh();
}
