# ─────────────────────────────────────────────────────────────────
#  NotepadTUI — Makefile
# ─────────────────────────────────────────────────────────────────
CXX      = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -Iinclude
LDFLAGS  = -lncurses

SRC_DIR  = src
OBJ_DIR  = build
BIN      = notepad

# Archivos fuente (excluye main para poder listarlos separado)
SOURCES  = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS  = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))

# ── Objetivo principal ────────────────────────────────────────────
all: $(OBJ_DIR) $(BIN) plugins

$(BIN): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(BIN) $(LDFLAGS) -ldl

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ── Plugins ───────────────────────────────────────────────────────
plugins: plugins/wordcount.so

plugins/wordcount.so: plugins/wordcount/wordcount.cpp include/iplugin.h include/editor.h
	$(CXX) $(CXXFLAGS) -shared -fPIC \
	    plugins/wordcount/wordcount.cpp \
	    -o plugins/wordcount.so

# ── Limpieza ──────────────────────────────────────────────────────
clean:
	rm -rf $(OBJ_DIR) $(BIN) plugins/*.so

# ── Compilar y ejecutar ───────────────────────────────────────────
run: all
	./$(BIN)

.PHONY: all plugins clean run
