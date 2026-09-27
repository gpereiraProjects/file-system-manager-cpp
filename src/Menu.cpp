#include "Menu.h"
#include "SistemaFicheiros.h"
#include "Utils.h"

using namespace std;

/// ===================== SUBMENUS ==========================

void Menu::MenuEstatisticas(SistemaFicheiros &SF) {
  int op;
  do {
    Utils::limparEcra();
    cout << "\n===== ESTATÍSTICAS =====\n";
    cout << "1. Estatísticas de Ficheiros\n";
    cout << "2. Estatísticas de Directorias\n";
    cout << "3. Memória Total\n";
    cout << "4. Voltar\n";
    cout << "Escolha: ";

    // 1. Valida se a entrada é um número. Se for letra, limpa o erro e o buffer
    // para evitar loop infinito.
    if (!(cin >> op)) {
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      cout << "Entrada inválida! Insira um número.\n";
      Utils::esperarEnter();
      continue;
    }
    // 2. Remove o 'Enter' (\n) residual do buffer para não saltar os próximos
    // getlines ou esperarEnter
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    switch (op) {
    case 1: { // Estatísticas de ficheiros
      Utils::limparEcra();
      cout << "\n--- FICHEIROS ---\n";
      cout << "Total de ficheiros: " << SF.ContarFicheiros() << endl;
      if (string *r = SF.FicheiroMaior()) {
        cout << "Ficheiro maior: " << *r << endl;
        delete r;
      } else
        cout << "Nenhum ficheiro encontrado.\n";
      Utils::esperarEnter();
      break;
    }

    case 2: { // Estatísticas de diretórios
      Utils::limparEcra();
      cout << "\n--- DIRECTORIAS ---\n";
      cout << "Total de directorias: " << SF.ContarDirectorias() << endl;
      string *r;
      r = SF.DirectoriaMaisElementos();
      if (r) {
        cout << "Mais elementos: " << *r << endl;
        delete r;
      }
      r = SF.DirectoriaMenosElementos();
      if (r) {
        cout << "Menos elementos: " << *r << endl;
        delete r;
      }
      r = SF.DirectoriaMaisEspaco();
      if (r) {
        cout << "Mais espaço: " << *r << endl;
        delete r;
      }
      Utils::esperarEnter();
      break;
    }

    case 3: // Memória total usada
      Utils::limparEcra();
      cout << "\n--- Memória Total ---\n";
      cout << "Memória total: " << SF.Memoria() << " bytes\n";
      Utils::esperarEnter();
      break;
    }
  } while (op != 4);
}

void Menu::MenuPesquisas(SistemaFicheiros &SF) {
  int op;
  string nome;
  list<string> lista;
  do {
    Utils::limparEcra();
    cout << "\n===== PESQUISAS =====\n";
    cout << "1. Pesquisar ficheiro/diretoria\n";
    cout << "2. Pesquisar todas as directorias\n";
    cout << "3. Pesquisar todos os ficheiros\n";
    cout << "4. Voltar\n";
    cout << "Escolha: ";

    if (!(cin >> op)) {
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      cout << "Entrada inválida! Insira um número.\n";
      Utils::esperarEnter();
      continue;
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    switch (op) {
    case 1: {
      Utils::limparEcra();
      cout << "===== PESQUISAR FICHEIRO/DIRETORIA =====\n";
      // Nota: Removeu-se o cin.ignore() daqui, pois o global já tratou disso.
      cout << "Nome a pesquisar: ";
      getline(cin, nome);

      cout << "Tipo (0 = ficheiro, 1 = diretoria): ";
      int tipoOp; // Usar variável diferente para não confundir com o op do menu
      if (cin >> tipoOp) {
        cin.ignore(numeric_limits<streamsize>::max(),
                   '\n'); // Limpar após ler int
        string *r = SF.Search(nome, tipoOp);
        if (r != nullptr) {
          cout << "Encontrado: " << *r << endl;
          delete r;
        } else {
          cout << "Não encontrado.\n";
        }
      } else {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Tipo inválido.\n";
      }

      Utils::esperarEnter();
      break;
    }
    case 2:
      Utils::limparEcra();
      cout << "===== PESQUISAR TODAS AS DIRECTORIAS =====\n";
      cout << "Nome da diretoria: ";
      getline(cin, nome);
      SF.PesquisarAllDirectorias(lista, nome);
      Utils::PrintListaString(lista);
      if (lista.empty()) {
        cout << "Nenhuma diretoria encontrada com esse nome.\n";
      }
      Utils::esperarEnter();
      break;

    case 3:
      Utils::limparEcra();
      cout << "===== PESQUISAR TODOS OS FICHEIROS =====\n";
      cout << "Nome do ficheiro: ";
      getline(cin, nome);
      SF.PesquisarAllFicheiros(lista, nome);
      Utils::PrintListaString(lista);
      if (lista.empty()) {
        cout << "Nenhum ficheiro encontrado com esse nome.\n";
      }
      Utils::esperarEnter();
      break;

    case 4:
      break;

    default:
      cout << "Opção inválida. Tente novamente.\n";
      Utils::esperarEnter();
      break;
    }
  } while (op != 4);
}

void Menu::MenuMovimentos(SistemaFicheiros &SF) {
  int op;
  string nome, dest;

  do {
    Utils::limparEcra();
    cout << "\n===== MOVIMENTAÇÕES =====\n";
    cout << "1. Mover ficheiro\n";
    cout << "2. Mover diretoria\n";
    cout << "3. Voltar\n";
    cout << "Escolha: ";

    if (!(cin >> op)) {
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      cout << "Entrada inválida! Insira um número.\n";
      Utils::esperarEnter();
      continue;
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    switch (op) {
    case 1:
      cout << "Ficheiro: ";
      getline(cin, nome);
      cout << "Diretoria destino: ";
      getline(cin, dest);
      if (SF.MoveFicheiro(nome, dest)) {
        cout << "Ficheiro movido com sucesso.\n";
      } else {
        cout << "Erro ao mover ficheiro.\n";
      }
      Utils::esperarEnter();
      break;

    case 2:
      cout << "Nome da diretoria: ";
      getline(cin, nome);
      cout << "Diretoria nova: ";
      getline(cin, dest);
      if (SF.MoverDirectoria(nome, dest)) {
        cout << "Diretoria movida com sucesso.\n";
      } else {
        cout << "Erro ao mover diretoria.\n";
      }
      Utils::esperarEnter();
      break;

    case 3:
      break;

    default:
      cout << "Opção inválida. Tente novamente.\n";
      Utils::esperarEnter();
      break;
    }
  } while (op != 3);
}

void Menu::MenuXML(SistemaFicheiros &SF) {
  int op;
  string xml;
  do {
    Utils::limparEcra();
    cout << "\n===== XML =====\n";
    cout << "1. Exportar XML\n";
    cout << "2. Importar XML\n";
    cout << "3. Voltar\n";
    cout << "Escolha: ";

    if (!(cin >> op)) {
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      cout << "Entrada inválida! Insira um número.\n";
      Utils::esperarEnter();
      continue;
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    switch (op) {
    case 1:
      cout << "Nome do ficheiro XML: ";
      // Mudei para getline para ser consistente e permitir espaços
      getline(cin, xml);
      SF.Escrever_XML(xml);
      cout << "Ficheiro XML exportado com sucesso.\n";
      Utils::esperarEnter();
      break;

    case 2:
      cout << "Nome do ficheiro XML: ";
      getline(cin, xml);
      if (SF.Ler_XML(xml)) {
        cout << "Ficheiro XML carregado com sucesso.\n";
      } else {
        cout << "Erro ao carregar ficheiro XML.\n";
      }
      Utils::esperarEnter();
      break;

    case 3:
      break;

    default:
      cout << "Opção inválida. Tente novamente.\n";
      Utils::esperarEnter();
      break;
    }

  } while (op != 3);
}

void Menu::MenuAvancado(SistemaFicheiros &SF) {
  int op;
  string nome, padrao, dir1, dir2;
  do {
    Utils::limparEcra();
    cout << "\n===== OPERAÇÕES AVANÇADAS =====\n";
    cout << "1. Ficheiros duplicados\n";
    cout << "2. CopyBatch\n";
    cout << "3. Data de ficheiro\n";
    cout << "4. Tree\n";
    cout << "5. Carregar nova diretoria\n";
    cout << "6. Remover Tudo\n";
    cout << "7. Voltar\n";
    cout << "Escolha: ";

    if (!(cin >> op)) {
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      cout << "Entrada inválida! Insira um número.\n";
      Utils::esperarEnter();
      continue;
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    switch (op) {
    case 1: // Verifica se existem ficheiros duplicados
      cout << (SF.FicheiroDuplicados() ? "Existem duplicados.\n"
                                       : "Não existem duplicados.\n");
      Utils::esperarEnter();
      break;

    case 2: // Copiar batch de ficheiros
      // cin.ignore() removido
      cout << "Padrão: ";
      getline(cin, padrao);
      cout << "Diretoria origem: ";
      getline(cin, dir1);
      cout << "Diretoria destino: ";
      getline(cin, dir2);
      if (SF.CopyBatch(padrao, dir1, dir2)) {
        cout << "Cópia concluída com sucesso.\n";
      } else {
        cout << "Erro na cópia.\n";
      }
      Utils::esperarEnter();
      break;

    case 3: // Mostrar data de ficheiro
      cout << "Nome do ficheiro: ";
      getline(cin, nome);
      if (string *d = SF.DataFicheiro(nome)) {
        cout << "Data: " << *d << endl;
        delete d;
      } else
        cout << "Ficheiro não encontrado.\n";
      Utils::esperarEnter();
      break;

    case 4: // Imprime a árvore de diretórios
      Utils::limparEcra();
      cout << "============TREE=============\n";
      SF.Tree();
      Utils::esperarEnter();
      break;

    case 5: {
      Utils::limparEcra();
      cout << "===== CARREGAR NOVA DIRETORIA =====\n";
      cout << "Caminho da nova diretoria: ";
      string path;
      getline(cin, path);
      if (SF.Load(path)) {
        cout << "Diretoria carregada com sucesso.\n";
      } else {
        cout << "Erro ao carregar diretoria.\n";
      }
      Utils::esperarEnter();
    } break;

    case 6: { // Remover tudo
      cout << "Nome: ";
      getline(cin, nome);

      int opTipo;
      cout << "Tipo (0: DIR / 1: FICH): ";
      // Pequena proteção extra para leitura interna
      if (cin >> opTipo) {
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        string tipo = (opTipo == 0) ? "DIR" : "FICH";
        cout << (SF.RemoverAll(nome, tipo) ? "Remoção concluída.\n"
                                           : "Erro na remoção.\n");
      } else {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Tipo inválido.\n";
      }

      Utils::esperarEnter();
      break;
    }

    case 7:
      break;

    default:
      cout << "Opção inválida. Tente novamente.\n";
      Utils::esperarEnter();
      break;
    }

  } while (op != 7);
}

bool Menu::MenuInicializacao(SistemaFicheiros &SF) {
  string path;
  int op;

  while (true) {
    Utils::limparEcra();
    cout << "\n===== INICIALIZAÇÃO =====\n";
    cout << "1. Carregar diretoria\n";
    cout << "2. Carregar XML\n";
    cout << "3. Encerrar programa\n";
    cout << "Opção: ";

    if (!(cin >> op)) {
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      cout << "Entrada inválida! Insira um número.\n";
      Utils::esperarEnter();
      continue;
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    switch (op) {
    case 1: {
      cout << "Caminho: ";
      getline(cin, path);

      bool ok = SF.Load(path);
      if (ok) {
        cout << "Diretoria carregada com sucesso.\n";
        Utils::esperarEnter();
        return true;
      } else {
        cout << "Erro ao carregar diretoria.\n";
        Utils::esperarEnter();
      }
      break;
    }

    case 2: {
      cout << "Nome do ficheiro XML: ";
      getline(cin, path);

      if (SF.Ler_XML(path)) {
        cout << "Ficheiro XML carregado com sucesso.\n";
        Utils::esperarEnter();
        return true;
      } else {
        cout << "Erro ao carregar ficheiro XML.\n";
        Utils::esperarEnter();
      }
      break;
    }

    case 3:
      return false;

    default:
      cout << "Opção inválida. Tente novamente.\n";
      Utils::esperarEnter();
      break;
    }
  }
}

void Menu::MostrarMenu() {
  cout << "\n==================== MENU PRINCIPAL ====================\n";
  cout << "1. Estatísticas\n";
  cout << "2. Pesquisas\n";
  cout << "3. Operações de Modificação\n";
  cout << "4. Movimentações\n";
  cout << "5. XML\n";
  cout << "6. Avançado\n";
  cout << "7. Sair\n";
  cout << "========================================================\n";
  cout << "Escolha a opção: ";
}

bool Menu::ExecutarOpcao(SistemaFicheiros &SF) {
  while (true) {
    Utils::limparEcra();
    MostrarMenu();
    int op;

    if (!(cin >> op)) {
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      cout << "Entrada inválida! Insira um número.\n";
      Utils::esperarEnter();
      continue;
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    switch (op) {
    case 1:
      MenuEstatisticas(SF);
      break;

    case 2:
      MenuPesquisas(SF);
      break;

    case 3: {
      string n1, n2;
      // cin.ignore() removido
      Utils::limparEcra();
      cout << "===== RENOMEAR FICHEIROS =====\n";
      cout << "Nome atual: ";
      getline(cin, n1);
      cout << "Novo nome: ";
      getline(cin, n2);
      SF.RenomearFicheiros(n1, n2);
      Utils::esperarEnter();
      break;
    }
    case 4:
      MenuMovimentos(SF);
      break;

    case 5:
      MenuXML(SF);
      break;

    case 6:
      MenuAvancado(SF);
      break;

    case 7:
      Utils::limparEcra();
      cout << "Programa terminado.\n";
      return false;

    default:
      cout << "Opção inválida. Tente novamente.\n";
      Utils::esperarEnter();
      break;
    }
  }
}
