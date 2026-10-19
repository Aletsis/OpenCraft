#include "core/Log.hpp"
#include "app/Application.hpp"
#include <cstdlib>
#include <iostream>

using namespace OpenCraft;

namespace {

constexpr const char* VERSION = OPENCRAFT_VERSION_STRING;

void printHelp(const char* argv0) {
    std::cout << "OpenCraft " << VERSION << "\n"
              << "Uso: " << argv0 << " [opciones]\n\n"
              << "Opciones:\n"
              << "  --world <id>          Abrir directamente el mundo indicado\n"
              << "  --create <nombre>     Crear un mundo nuevo y entrar en él\n"
              << "  --seed <semilla>      Semilla para --create (número o texto)\n"
              << "  --creative            Crear el mundo de --create en modo creativo\n"
              << "  --list                Listar los mundos guardados y salir\n"
              << "  --width <px>          Ancho inicial de la ventana (por defecto 1280)\n"
              << "  --height <px>         Alto inicial de la ventana (por defecto 720)\n"
              << "  --fullscreen          Iniciar a pantalla completa\n"
              << "  --autosave <seg>      Intervalo de autoguardado (por defecto 30)\n"
              << "  --script <archivo>    Ejecutar un guion de pruebas automáticas (QA)\n"
              << "  --version             Mostrar la versión y salir\n"
              << "  --help                Mostrar esta ayuda\n\n"
              << "Variables de entorno:\n"
              << "  OPENCRAFT_ASSETS_DIR  Carpeta de recursos (shaders) si no está junto al ejecutable\n"
              << "  XDG_DATA_HOME         Base de mundos, copias y capturas (por defecto ~/.local/share/opencraft)\n";
}

} // namespace

int main(int argc, char* argv[]) {
    AppConfig config;
    config.title = "OpenCraft";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "Falta el valor de " << arg << "\n";
                std::exit(2);
            }
            return argv[++i];
        };
        try {
            if (arg == "--help" || arg == "-h") {
                printHelp(argv[0]);
                return 0;
            } else if (arg == "--version") {
                std::cout << "OpenCraft " << VERSION << "\n";
                return 0;
            } else if (arg == "--list") {
                config.listWorldsOnly = true;
            } else if (arg == "--world") {
                config.worldId = next();
            } else if (arg == "--create") {
                config.createWorldName = next();
            } else if (arg == "--seed") {
                config.createWorldSeed = next();
            } else if (arg == "--creative") {
                config.createWorldMode = 1;
            } else if (arg == "--width") {
                config.width = std::stoi(next());
            } else if (arg == "--height") {
                config.height = std::stoi(next());
            } else if (arg == "--fullscreen") {
                config.fullscreen = true;
            } else if (arg == "--autosave") {
                config.autoSaveInterval = std::stof(next());
            } else if (arg == "--script") {
                config.scriptPath = next();
            } else {
                std::cerr << "Opción desconocida: " << arg << " (use --help)\n";
                return 2;
            }
        } catch (const std::exception&) {
            std::cerr << "Valor no válido para " << arg << "\n";
            return 2;
        }
    }

    Log::info("Iniciando OpenCraft {}", VERSION);
    Application app(config);
    if (!app.init()) {
        if (config.listWorldsOnly) return 0;
        Log::error("Fallo crítico durante la inicialización de la aplicación");
        return 1;
    }
    return app.run();
}
