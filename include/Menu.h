#ifndef MENU_H
#define MENU_H

#include "SistemaFicheiros.h"

class Menu {
public:
    static void MostrarMenu();
    static bool ExecutarOpcao(SistemaFicheiros &SF);
    static bool MenuInicializacao(SistemaFicheiros &SF);

private:
    // Submenus organizados
    static void MenuEstatisticas(SistemaFicheiros &SF);
    static void MenuPesquisas(SistemaFicheiros &SF);
    static void MenuMovimentos(SistemaFicheiros &SF);
    static void MenuXML(SistemaFicheiros &SF);
    static void MenuAvancado(SistemaFicheiros &SF);
};

#endif // MENU_H
