#include "Menu.h"
#include "SistemaFicheiros.h"
#include "Utils.h"

using namespace std;

/// ===================== SUBMENUS ==========================

Menu::InputStatus Menu::LerInteiro(int &valor) {
  string linha;
  if (!getline(cin, linha))
    return InputStatus::EndOfInput;

  istringstream input(linha);
  input >> ws;
  if (!(input >> valor))
    return InputStatus::Invalid;

  input >> ws;
  return input.eof() ? InputStatus::Success : InputStatus::Invalid;
}

bool Menu::LerTexto(string &valor) {
  return static_cast<bool>(getline(cin, valor));
}

bool Menu::MenuEstatisticas(SistemaFicheiros &SF) {
  int op = 0;
  do {
    Utils::limparEcra();
    cout << "\n===== ESTATÍSTICAS =====\n";
    cout << "1. Estatísticas de Ficheiros\n";
    cout << "2. Estatísticas de Directorias\n";
    cout << "3. Memória Total\n";
    cout << "4. Voltar\n";
    cout << "Escolha: ";

    const InputStatus status = LerInteiro(op);
    if (status == InputStatus::EndOfInput)
      return false;
    if (status == InputStatus::Invalid) {
      cout << "Entrada inválida! Insira um número.\n";
      Utils::esperarEnter();
      continue;
    }

    switch (op) {
    case 1: { // Estatísticas de ficheiros
      Utils::limparEcra();
      cout << "\n--- FICHEIROS ---\n";
      cout << "Total de ficheiros: " << SF.ContarFicheiros() << endl;
      if (unique_ptr<string> r =
              unique_ptr<string>(SF.FicheiroMaior())) {
        cout << "Ficheiro maior: " << *r << endl;
      } else
        cout << "Nenhum ficheiro encontrado.\n";
      Utils::esperarEnter();
      break;
    }

    case 2: { // Estatísticas de diretórios
      Utils::limparEcra();
      cout << "\n--- DIRECTORIAS ---\n";
      cout << "Total de directorias: " << SF.ContarDirectorias() << endl;
      unique_ptr<string> r(SF.DirectoriaMaisElementos());
      if (r) {
        cout << "Mais elementos: " << *r << endl;
      }
      r.reset(SF.DirectoriaMenosElementos());
      if (r) {
        cout << "Menos elementos: " << *r << endl;
      }
      r.reset(SF.DirectoriaMaisEspaco());
      if (r) {
        cout << "Mais espaço: " << *r << endl;
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
  return true;
}

bool Menu::MenuPesquisas(SistemaFicheiros &SF) {
  int op = 0;
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

    const InputStatus status = LerInteiro(op);
    if (status == InputStatus::EndOfInput)
      return false;
    if (status == InputStatus::Invalid) {
      cout << "Entrada inválida! Insira um número.\n";
      Utils::esperarEnter();
      continue;
    }

    switch (op) {
    case 1: {
      Utils::limparEcra();
      cout << "===== PESQUISAR FICHEIRO/DIRETORIA =====\n";
      cout << "Caminho relativo à raiz: ";
      if (!LerTexto(nome))
        return false;

      cout << "Tipo (0 = ficheiro, 1 = diretoria): ";
      int tipoOp = 0;
      const InputStatus tipoStatus = LerInteiro(tipoOp);
      if (tipoStatus == InputStatus::EndOfInput)
        return false;
      if (tipoStatus == InputStatus::Success &&
          (tipoOp == 0 || tipoOp == 1)) {
        unique_ptr<string> r(SF.Search(nome, tipoOp));
        if (r) {
          cout << "Encontrado: " << *r << endl;
        } else {
          cout << "Não encontrado.\n";
        }
      } else {
        cout << "Tipo inválido.\n";
      }

      Utils::esperarEnter();
      break;
    }
    case 2:
      Utils::limparEcra();
      cout << "===== PESQUISAR TODAS AS DIRECTORIAS =====\n";
      cout << "Nome da diretoria: ";
      if (!LerTexto(nome))
        return false;
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
      if (!LerTexto(nome))
        return false;
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
  return true;
}

bool Menu::MenuMovimentos(SistemaFicheiros &SF) {
  int op = 0;
  string nome, dest;

  do {
    Utils::limparEcra();
    cout << "\n===== MOVIMENTAÇÕES =====\n";
    cout << "1. Mover ficheiro\n";
    cout << "2. Mover diretoria\n";
    cout << "3. Voltar\n";
    cout << "Escolha: ";

    const InputStatus status = LerInteiro(op);
    if (status == InputStatus::EndOfInput)
      return false;
    if (status == InputStatus::Invalid) {
      cout << "Entrada inválida! Insira um número.\n";
      Utils::esperarEnter();
      continue;
    }

    switch (op) {
    case 1:
      cout << "Caminho do ficheiro: ";
      if (!LerTexto(nome))
        return false;
      cout << "Caminho da diretoria destino ('.' para a raiz): ";
      if (!LerTexto(dest))
        return false;
      if (SF.MoveFicheiro(nome, dest)) {
        cout << "Ficheiro movido com sucesso.\n";
      } else {
        cout << "Erro ao mover ficheiro.\n";
      }
      Utils::esperarEnter();
      break;

    case 2:
      cout << "Caminho da diretoria: ";
      if (!LerTexto(nome))
        return false;
      cout << "Caminho da diretoria destino ('.' para a raiz): ";
      if (!LerTexto(dest))
        return false;
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
  return true;
}

bool Menu::MenuXML(SistemaFicheiros &SF) {
  int op = 0;
  string xml;
  do {
    Utils::limparEcra();
    cout << "\n===== XML =====\n";
    cout << "1. Exportar XML\n";
    cout << "2. Importar XML\n";
    cout << "3. Voltar\n";
    cout << "Escolha: ";

    const InputStatus status = LerInteiro(op);
    if (status == InputStatus::EndOfInput)
      return false;
    if (status == InputStatus::Invalid) {
      cout << "Entrada inválida! Insira um número.\n";
      Utils::esperarEnter();
      continue;
    }

    switch (op) {
    case 1:
      cout << "Nome do ficheiro XML: ";
      if (!LerTexto(xml))
        return false;
      if (SF.Escrever_XML(xml))
        cout << "Ficheiro XML exportado com sucesso.\n";
      else
        cout << "Erro ao exportar ficheiro XML.\n";
      Utils::esperarEnter();
      break;

    case 2:
      cout << "Nome do ficheiro XML: ";
      if (!LerTexto(xml))
        return false;
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
  return true;
}

bool Menu::MenuAvancado(SistemaFicheiros &SF) {
  int op = 0;
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

    const InputStatus status = LerInteiro(op);
    if (status == InputStatus::EndOfInput)
      return false;
    if (status == InputStatus::Invalid) {
      cout << "Entrada inválida! Insira um número.\n";
      Utils::esperarEnter();
      continue;
    }

    switch (op) {
    case 1: // Verifica se existem ficheiros duplicados
      cout << (SF.FicheiroDuplicados() ? "Existem duplicados.\n"
                                       : "Não existem duplicados.\n");
      Utils::esperarEnter();
      break;

    case 2: // Copiar batch de ficheiros
      cout << "Padrão: ";
      if (!LerTexto(padrao))
        return false;
      cout << "Diretoria origem: ";
      if (!LerTexto(dir1))
        return false;
      cout << "Diretoria destino: ";
      if (!LerTexto(dir2))
        return false;
      if (SF.CopyBatch(padrao, dir1, dir2)) {
        cout << "Cópia concluída com sucesso.\n";
      } else {
        cout << "Erro na cópia.\n";
      }
      Utils::esperarEnter();
      break;

    case 3: // Mostrar data de ficheiro
      cout << "Caminho do ficheiro: ";
      if (!LerTexto(nome))
        return false;
      if (unique_ptr<string> d =
              unique_ptr<string>(SF.DataFicheiro(nome))) {
        cout << "Data: " << *d << endl;
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
      if (!LerTexto(path))
        return false;
      if (SF.Load(path)) {
        cout << "Diretoria carregada com sucesso.\n";
      } else {
        cout << "Erro ao carregar diretoria.\n";
      }
      Utils::esperarEnter();
    } break;

    case 6: { // Remover tudo
      cout << "Nome: ";
      if (!LerTexto(nome))
        return false;

      int opTipo = 0;
      cout << "Tipo (0: DIR / 1: FICH): ";
      const InputStatus tipoStatus = LerInteiro(opTipo);
      if (tipoStatus == InputStatus::EndOfInput)
        return false;
      if (tipoStatus == InputStatus::Success &&
          (opTipo == 0 || opTipo == 1)) {
        string tipo = (opTipo == 0) ? "DIR" : "FICH";
        cout << (SF.RemoverAll(nome, tipo) ? "Remoção concluída.\n"
                                           : "Erro na remoção.\n");
      } else {
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
  return true;
}

bool Menu::MenuInicializacao(SistemaFicheiros &SF) {
  string path;
  int op = 0;

  while (true) {
    Utils::limparEcra();
    cout << "\n===== INICIALIZAÇÃO =====\n";
    cout << "1. Carregar diretoria\n";
    cout << "2. Carregar XML\n";
    cout << "3. Encerrar programa\n";
    cout << "Opção: ";

    const InputStatus status = LerInteiro(op);
    if (status == InputStatus::EndOfInput)
      return false;
    if (status == InputStatus::Invalid) {
      cout << "Entrada inválida! Insira um número.\n";
      Utils::esperarEnter();
      continue;
    }

    switch (op) {
    case 1: {
      cout << "Caminho: ";
      if (!LerTexto(path))
        return false;

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
      if (!LerTexto(path))
        return false;

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
    int op = 0;

    const InputStatus status = LerInteiro(op);
    if (status == InputStatus::EndOfInput)
      return false;
    if (status == InputStatus::Invalid) {
      cout << "Entrada inválida! Insira um número.\n";
      Utils::esperarEnter();
      continue;
    }

    switch (op) {
    case 1:
      if (!MenuEstatisticas(SF))
        return false;
      break;

    case 2:
      if (!MenuPesquisas(SF))
        return false;
      break;

    case 3: {
      string n1, n2;
      Utils::limparEcra();
      cout << "===== RENOMEAR FICHEIROS =====\n";
      cout << "Nome atual: ";
      if (!LerTexto(n1))
        return false;
      cout << "Novo nome: ";
      if (!LerTexto(n2))
        return false;
      SF.RenomearFicheiros(n1, n2);
      Utils::esperarEnter();
      break;
    }
    case 4:
      if (!MenuMovimentos(SF))
        return false;
      break;

    case 5:
      if (!MenuXML(SF))
        return false;
      break;

    case 6:
      if (!MenuAvancado(SF))
        return false;
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
