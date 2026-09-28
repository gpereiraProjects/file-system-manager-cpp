#ifndef MENU_H
#define MENU_H

#include "SistemaFicheiros.h"

class Menu {
public:
    static void MostrarMenu();
    static bool ExecutarOpcao(SistemaFicheiros &SF);
    static bool MenuInicializacao(SistemaFicheiros &SF);

private:
    enum class InputStatus { Success, Invalid, EndOfInput };

    static InputStatus LerInteiro(int &valor);
    static bool LerTexto(std::string &valor);

    // Submenus organizados
    static bool MenuEstatisticas(SistemaFicheiros &SF);
    static bool MenuPesquisas(SistemaFicheiros &SF);
    static bool MenuMovimentos(SistemaFicheiros &SF);
    static bool MenuXML(SistemaFicheiros &SF);
    static bool MenuAvancado(SistemaFicheiros &SF);
};

#endif // MENU_H
