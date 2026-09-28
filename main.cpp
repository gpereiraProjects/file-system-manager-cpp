#include "IncludesGerais.h"
#include "SistemaFicheiros.h"
#include "Utils.h"
#include "Menu.h"
#include "Logger.h"

using namespace std;

int main()
{
    Logger::init("app.log");
    Logger::log(Logger::Level::INFO, "Programa iniciado.");
    Utils::UTF8();                                // configura a consola para UTF-8 (Windows)
    SistemaFicheiros SF;                          // cria o objecto do sistema de ficheiros

    if(!Menu::MenuInicializacao(SF)){
        cout << "A sair do programa.\n";
        Logger::log(Logger::Level::INFO, "Programa terminado pelo utilizador.");
        return 0;
    }

    // Menu interativo
    bool continuar = true;
    while (continuar) {
        continuar = Menu::ExecutarOpcao(SF);
    }
    Logger::log(Logger::Level::INFO, "Programa terminado.");
    return 0;
}
