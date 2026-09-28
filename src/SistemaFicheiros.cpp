#include "SistemaFicheiros.h"
#include "Ficheiro.h"
#include "Logger.h"
#include "Utils.h"
#include "XML.h"

using namespace std;
namespace fs = std::filesystem;

//==================== Construtor e Destrutor ====================
SistemaFicheiros::SistemaFicheiros()
    : raiz(nullptr), importacao_diretoria(false) {}

//==================== Métodos Privados ====================
/**
 * Resumo:
 * Percorre recursivamente o sistema de ficheiros a partir do caminho da
 * diretoria fornecida. Para cada entrada encontrada (ficheiro ou pasta), cria o
 * respetivo objeto ('Ficheiro' ou 'Diretoria') e adiciona-o à estrutura em
 * memória. As exceções são propagadas para `Load`, que só substitui a árvore
 * ativa depois de concluir todo o carregamento.
 *
 * Parâmetros:
 * - diretoria (Diretoria*): Ponteiro para o objeto diretoria que será
 * preenchido com o conteúdo encontrado no disco.
 *
 * Retorno:
 * - void (Não retorna valor).
 */
void SistemaFicheiros::carregarConteudo(Diretoria *diretoria) {
  for (const auto &entry : fs::directory_iterator(diretoria->getCaminho())) {

    string nome = entry.path().filename().string();
    string caminhoCompleto = entry.path().string();

    if (entry.is_directory()) {
      auto sub = make_unique<Diretoria>(nome, caminhoCompleto);
      carregarConteudo(sub.get());
      diretoria->adicionar(move(sub));
    } else if (entry.is_regular_file()) {
      diretoria->adicionar(make_unique<Ficheiro>(nome, caminhoCompleto));
    }
  }
}

/**
 * Resumo:
 * Percorre recursivamente uma diretoria e todas as suas subdiretorias para
 * contabilizar o número total de ficheiros (excluindo pastas da contagem).
 * Se o item for um ficheiro, incrementa o contador; se for uma diretoria,
 * invoca-se a si mesma para somar os ficheiros contidos nela.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria raiz onde a contagem deve
 * começar.
 *
 * Retorno:
 * - int: O número total de ficheiros encontrados na hierarquia. Retorna 0 se a
 * diretoria for nula.
 */
int SistemaFicheiros::ContarFicheirosRec(Diretoria *dir) {
  if (dir == nullptr)
    return 0;
  int total = 0;
  for (const auto &item : dir->getConteudo()) {
    if (item->getIsFicheiro()) {
      total++;
    } else {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item.get());
      if (subdir) {
        total += ContarFicheirosRec(subdir);
      }
    }
  }
  return total;
}

/**
 * Resumo:
 * Calcula recursivamente o número total de diretorias na hierarquia.
 * A contagem inicia-se em 1 para incluir a própria diretoria atual (`dir`).
 * Em seguida, percorre o conteúdo e, para cada subdiretoria encontrada,
 * soma o resultado da chamada recursiva ao total.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria raiz da contagem atual.
 *
 * Retorno:
 * - int: O número total de diretorias (incluindo a própria raiz).
 * Retorna 0 se o ponteiro `dir` for nulo.
 */
int SistemaFicheiros::ContarDirectoriasRec(Diretoria *dir) {
  if (dir == nullptr)
    return 0;
  int total = 1; // conta a diretoria atual
  for (const auto &item : dir->getConteudo()) {
    if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item.get());
      if (subdir) {
        total += ContarDirectoriasRec(subdir);
      }
    }
  }
  return total;
}

/**
 * Resumo:
 * Percorre recursivamente a diretoria e subdiretorias para encontrar o ficheiro
 * com o maior tamanho em bytes. Compara cada ficheiro encontrado com o
 * valor atual de 'tamMax' e atualiza as referências se encontrar um maior.
 *
 * Parâmetros:
 * - dir (Diretoria*): A diretoria atual onde a pesquisa está a ser feita.
 * - tamMax (uintmax_t&): Referência para o tamanho máximo encontrado até agora.
 * É atualizada automaticamente durante a recursão.
 * - strMax (string&): Referência para o caminho do ficheiro que possui o
 * 'tamMax'.
 *
 * Retorno:
 * - string*: Ponteiro para a string 'strMax' contendo o caminho do maior
 * ficheiro. Retorna nullptr se a diretoria inicial for nula.
 */
void SistemaFicheiros::ficheiroMaiorRec(Diretoria *dir, uintmax_t &tamMax,
                                        string &strMax) {
  if (!dir)
    return;

  for (const auto &item : dir->getConteudo()) {
    if (item->getIsFicheiro()) {
      if (item->getTamanho() > tamMax) {
        tamMax = item->getTamanho();
        strMax = item->getCaminho();
      }
    } else {
      // é diretoria → recursão
      Diretoria *diretoria = dynamic_cast<Diretoria *>(item.get());
      if (diretoria) {
        ficheiroMaiorRec(diretoria, tamMax, strMax);
      }
    }
  }

}

/**
 * Resumo:
 * Calcula recursivamente a quantidade total de memória ocupada pelos ficheiros
 * dentro de uma diretoria e suas subdiretorias.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria raiz onde a contagem deve
 * começar.
 *
 * Retorno:
 * - int: A quantidade total de memória ocupada pelos ficheiros.
 * Retorna 0 se a diretoria for nula.
 */
uintmax_t SistemaFicheiros::memoriaRec(Diretoria *dir) {
  if (!dir)
    return 0;

  uintmax_t total = 0;

  for (const auto &item : dir->getConteudo()) {
    if (item->getIsFicheiro()) {
      total += item->getTamanho();
    } else {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item.get());
      if (subdir) {
        total += memoriaRec(subdir);
      }
    }
  }

  return total;
}

/**
 * Resumo:
 * Percorre recursivamente a hierarquia de diretorias para encontrar a diretoria
 * com o maior número de itens (ficheiros e subdiretorias).
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria atual na recursão.
 * - maior (int*): Ponteiro para um inteiro que armazenará o maior número de
 * itens encontrados até o momento.
 *
 * Retorno:
 * - string*: Ponteiro para uma string contendo o nome da diretoria com mais
 * itens.
 */
string SistemaFicheiros::maiorDiretoriaRec(Diretoria *dir, size_t &maior) {
  size_t localMax = dir->getNItens();
  string localName = dir->getNome();

  for (const auto &item : dir->getConteudo()) {
    if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item.get());
      size_t nItem = subdir->getNItens();
      if (nItem > localMax) {
        localMax = nItem;
        localName = subdir->getNome();
      }

      size_t subMax = 0;
      string nomeSub = maiorDiretoriaRec(subdir, subMax);

      if (subMax > localMax) {
        localMax = subMax;
        localName = move(nomeSub);
      }
    }
  }

  maior = localMax;

  return localName;
}

/**
 * Resumo:
 * Percorre recursivamente a hierarquia de diretorias para encontrar a diretoria
 * com o menor número de itens (ficheiros e subdiretorias).
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria atual na recursão.
 * - menor (int*): Ponteiro para um inteiro que armazenará o menor número de
 * itens encontrados até o momento.
 *
 * Retorno:
 * - string*: Ponteiro para uma string contendo o nome da diretoria com menos
 * itens.
 */
string SistemaFicheiros::menorDiretoriaRec(Diretoria *dir, size_t &menor) {
  size_t localMin = dir->getNItens();
  string localName = dir->getNome();

  for (const auto &item : dir->getConteudo()) {
    if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item.get());
      size_t nItem = subdir->getNItens();
      if (nItem < localMin) {
        localMin = nItem;
        localName = subdir->getNome();
      }

      size_t subMin = numeric_limits<size_t>::max();
      string nomeSub = menorDiretoriaRec(subdir, subMin);

      if (subMin < localMin) {
        localMin = subMin;
        localName = move(nomeSub);
      }
    }
  }

  menor = localMin;

  return localName;
}

/**
 * Resumo:
 * Percorre o conteúdo imediato da diretoria fornecida (sem recursão) para
 * encontrar a subdiretoria com o maior tamanho. Ignora ficheiros durante a
 * verificação. Se encontrar uma subdiretoria maior que o valor 'tamMax',
 * atualiza a string 'strMax' com o novo caminho.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria onde a pesquisa será feita.
 * - tamMax (uintmax_t): O valor de tamanho a ser superado (passado por valor).
 * - strMax (string&): Referência para a string onde será guardado o caminho da
 * diretoria vencedora.
 *
 * Retorno:
 * - string*: Ponteiro para a variável 'strMax' (contendo o caminho da maior
 * diretoria encontrada).
 */
void SistemaFicheiros::diretoriaMaisEspaco(Diretoria *dir, uintmax_t &tamMax,
                                           string &strMax) {
  for (const auto &c : dir->getConteudo()) {
    if (!c->getIsFicheiro()) {
      if (c->getTamanho() > tamMax) {
        tamMax = c->getTamanho();
        strMax = c->getCaminho();
      }
      if (auto *subdiretoria = dynamic_cast<Diretoria *>(c.get()))
        diretoriaMaisEspaco(subdiretoria, tamMax, strMax);
    }
  }
}

/**
 * Resumo: Percorre recursivamente a hierarquia de diretorias para encontrar a
 * diretoria com o nome especificado.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria atual na recursão.
 * - s (const string&): Nome da diretoria a ser pesquisada.
 *
 * Retorno:
 * - string*: Ponteiro para uma string contendo o caminho da diretoria
 * encontrada, ou nullptr se não encontrada.
 */
optional<string> SistemaFicheiros::pesquisarDiretoriaRec(Diretoria *dir,
                                                         const string &s) {
  if (dir->getNome() == s) {
    return dir->getCaminho();
  }

  for (const auto &item : dir->getConteudo()) {

    if (!item->getIsFicheiro() && item->getNome() == s) {
      return item->getCaminho();
    }

    if (!item->getIsFicheiro()) {
      Diretoria *d = dynamic_cast<Diretoria *>(item.get());
      if (d) {
        optional<string> res = pesquisarDiretoriaRec(d, s);
        if (res)
          return res;
      }
    }
  }

  return nullopt;
}

/**
 * Resumo: Percorre recursivamente a hierarquia de diretorias para encontrar o
 * ficheiro com o nome especificado.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria atual na recursão.
 * - s (const string&): Nome do ficheiro a ser pesquisado.
 *
 * Retorno:
 * - string*: Ponteiro para uma string contendo o caminho do ficheiro
 * encontrado, ou nullptr se não encontrado.
 */
optional<string> SistemaFicheiros::pesquisarFicheiroRec(Diretoria *dir,
                                                        const string &s) {
  for (const auto &item : dir->getConteudo()) {

    if (item->getIsFicheiro() && item->getNome() == s) {
      return item->getCaminho();
    }

    if (!item->getIsFicheiro()) {
      Diretoria *d = dynamic_cast<Diretoria *>(item.get());
      if (d) {
        optional<string> res = pesquisarFicheiroRec(d, s);
        if (res)
          return res;
      }
    }
  }

  return nullopt;
}

/**
 * Resumo:
 * Percorre recursivamente a hierarquia de diretorias para encontrar todos os
 * itens (ficheiros ou diretorias) que correspondam ao nome fornecido.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria atual na recursão.
 * - lres (list<string>&): Referência para a lista onde serão armazenados os
 * caminhos dos itens encontrados.
 * - n (const string&): Nome do item a ser pesquisado.
 * - procFich (bool): Indica se deve procurar ficheiros (true) ou diretorias
 * (false).
 *
 * Retorno:
 * - void (Não retorna valor, os resultados são armazenados em 'lres').
 */
void SistemaFicheiros::pesquisarItensComNomeIgualRec(Diretoria *dir,
                                                     list<string> &lres,
                                                     const string &n,
                                                     bool procFich) {
  if (dir->getNome() == n && !procFich)
    lres.push_back(dir->getCaminho());

  for (const auto &item : dir->getConteudo()) {
    if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item.get());
      if (subdir)
        pesquisarItensComNomeIgualRec(subdir, lres, n, procFich);
    } else if (item->getNome() == n && procFich) {
      lres.push_back(item->getCaminho());
    }
  }
}

/**
 * Resumo:
 * Remove todos os itens (ficheiros ou diretorias) com o nome especificado
 * da hierarquia de diretorias, tanto da estrutura em memória quanto do sistema
 * de ficheiros, se indicado.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria raiz onde a remoção deve
 * começar.
 * - s (const string&): Nome do item a ser removido.
 * - tipo (const string&): Tipo do item a ser removido ("DIR" para diretorias,
 * outro valor para ficheiros).
 * - fs (bool): Indica se o item deve ser removido do sistema de ficheiros.
 * - importacao_diretoria (bool): Indica se a estrutura foi carregada a partir
 * de uma diretoria (true) ou de um XML (false).
 *
 * Retorno:
 * - bool: Verdadeiro se pelo menos um item foi removido, falso caso contrário.
 */
bool SistemaFicheiros::RemovePorNome(Diretoria *dir, const string &s,
                                     const string &tipo, bool fs,
                                     bool operarNoDisco) {
  bool t = (tipo == "DIR") ? false : true;

  list<string> lres;
  pesquisarItensComNomeIgualRec(dir, lres, s, t);

  if (!t) {
    const string caminhoRaiz = Utils::NormalizarCaminho(dir->getCaminho());
    lres.remove_if([&caminhoRaiz](const string &caminho) {
      return Utils::NormalizarCaminho(caminho) == caminhoRaiz;
    });
  }

  Utils::PrintListaString(lres);

  if (lres.empty()) {
    return false;
  }

  if (!t)
    lres.sort([](const string &left, const string &right) {
      return fs::path(left).lexically_normal().native().size() >
             fs::path(right).lexically_normal().native().size();
    });

  for (const string &caminho : lres) {
    if (fs && operarNoDisco) {
      error_code error;
      bool removidoNoDisco = false;
      if (t)
        removidoNoDisco = fs::remove(fs::path(caminho), error);
      else
        removidoNoDisco = fs::remove_all(fs::path(caminho), error) > 0;
      if (error || !removidoNoDisco)
        return false;
    }

    if (!removerPorCaminho(dir, caminho))
      return false;
  }

  return true;
}

/**
 * Resumo:
 * Procura recursivamente um item (ficheiro ou diretoria) com o caminho
 * especificado dentro da diretoria fornecida. Se o item for encontrado
 * (comparando caminhos normalizados), liberta a memória alocada, remove-o da
 * lista de conteúdos e retorna verdadeiro.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria onde a busca inicia.
 * - caminho (const string&): O caminho do ficheiro ou pasta que se pretende
 * remover.
 *
 * Retorno:
 * - bool: Retorna 'true' se o item foi encontrado e removido com sucesso,
 * ou 'false' caso contrário.
 */
bool SistemaFicheiros::removerPorCaminho(Diretoria *dir,
                                         const string &caminho) {
  for (const auto &ownedItem : dir->getConteudo()) {
    Item *item = ownedItem.get();

    if (Utils::NormalizarCaminho(item->getCaminho()) ==
        Utils::NormalizarCaminho(caminho)) {
      dir->extrair(item);
      return true;
    }

    if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item);
      if (subdir) {
        bool removido = removerPorCaminho(subdir, caminho);
        if (removido)
          return true;
      }
    }
  }

  return false;
}

/**
 * Resumo:
 * Percorre recursivamente a estrutura de ficheiros a partir da diretoria
 * fornecida para exportar os dados para um formato XML. Escreve a tag de
 * abertura da diretoria, regista os atributos de todos os ficheiros contidos e
 * invoca-se a si mesma para processar subdiretorias, fechando a tag da
 * diretoria no final.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria atual que está a ser convertida
 * para XML.
 * - XML (XML*): Ponteiro para o objeto utilitário responsável pela escrita das
 * tags no ficheiro XML.
 *
 * Retorno:
 * - void (Não retorna valor).
 */
void SistemaFicheiros::escreverXMLRec(Diretoria *dir, XML *XML) {
  XML->WriteStartDirectory(dir->getNome(), dir->getTamanho());

  for (const auto &item : dir->getConteudo()) {
    if (item->getIsFicheiro()) {
      Ficheiro *fich = dynamic_cast<Ficheiro *>(item.get());
      if (fich) {
        XML->WriteFile(fich->getNome(), fich->getTamanho(), fich->getExtensao(),
                       fich->getDataModificacao());
      }
    } else {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item.get());
      if (subdir) {
        escreverXMLRec(subdir, XML);
      }
    }
  }
  XML->WriteEndDirectory();
}

/**
 * Resumo:
 * Procura recursivamente um item (ficheiro ou diretoria) com o nome
 * especificado dentro da diretoria fornecida. Dependendo do parâmetro
 * 'procurarDiretorias', a busca é feita apenas em diretorias ou apenas em
 * ficheiros.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria onde a busca inicia.
 * - nomeProcurado (const string&): O nome do ficheiro ou pasta que se pretende
 * encontrar.
 * - procurarDiretorias (bool): Indica se a busca deve ser feita em diretorias
 * (true) ou em ficheiros (false).
 *
 * Retorno:
 * - Item*: Ponteiro para o item encontrado (ficheiro ou diretoria), ou nullptr
 * se não encontrado.
 */
Item *SistemaFicheiros::procurarItemRec(Diretoria *dir,
                                        const string &nomeProcurado,
                                        bool procurarDiretorias) {
  if (!dir)
    return nullptr;

  for (const auto &item : dir->getConteudo()) {
    // 1. Verifica se é o que procuramos
    if ((!item->getIsFicheiro() == procurarDiretorias) &&
        (item->getNome() == nomeProcurado)) {
      return item.get();
    }

    if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item.get());
      Item *res = procurarItemRec(subdir, nomeProcurado, procurarDiretorias);
      if (res)
        return res;
    }
  }

  return nullptr;
}

Diretoria *SistemaFicheiros::procurarDiretoriaPai(Diretoria *dir,
                                                   const Item *item) {
  if (!dir || !item)
    return nullptr;

  for (const auto &conteudo : dir->getConteudo()) {
    if (conteudo.get() == item)
      return dir;

    if (auto *subdiretoria = dynamic_cast<Diretoria *>(conteudo.get())) {
      if (Diretoria *pai = procurarDiretoriaPai(subdiretoria, item))
        return pai;
    }
  }
  return nullptr;
}

bool SistemaFicheiros::contemDiretoria(Diretoria *origem,
                                       const Diretoria *procurada) {
  if (!origem || !procurada)
    return false;
  if (origem == procurada)
    return true;

  for (const auto &item : origem->getConteudo()) {
    if (auto *subdiretoria = dynamic_cast<Diretoria *>(item.get())) {
      if (contemDiretoria(subdiretoria, procurada))
        return true;
    }
  }
  return false;
}

/**
 * Resumo:
 * Remove um item (ficheiro ou diretoria) com o nome especificado da diretoria
 * fornecida. Percorre a lista de conteúdos e, se encontrar um item com o nome
 * correspondente, remove-o da lista e retorna verdadeiro.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria onde a remoção será feita.
 * - nome (const string&): Nome do item a ser removido.
 *
 * Retorno:
 * - bool: Verdadeiro se o item foi encontrado e removido, falso caso contrário.
 */
unique_ptr<Item> SistemaFicheiros::extrairItemPorNome(Diretoria *dir,
                                                       const string &nome) {
  for (const auto &item : dir->getConteudo()) {
    if (item->getNome() == nome)
      return dir->extrair(item.get());
  }
  return nullptr;
}

/**
 * Resumo:
 * Atualiza recursivamente o caminho de uma diretoria e de todos os seus itens
 * (ficheiros e subdiretorias) para refletir uma mudança de localização.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria cuja estrutura de caminhos será
 * atualizada.
 * - caminhoDir (string&): Referência para o novo caminho base da diretoria.
 *
 * Retorno:
 * - void (Não retorna valor).
 */
void SistemaFicheiros::setCaminhoRec(Diretoria *dir, string &caminhoDir) {
  dir->setCaminho(caminhoDir);

  for (const auto &item : dir->getConteudo()) {
    string novoCaminho = caminhoDir + "/" + item->getNome();
    item->setCaminho(novoCaminho);

    if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item.get());
      if (subdir) {
        setCaminhoRec(subdir, novoCaminho);
      }
    }
  }
}

/**
 * Resumo:
 * Procura recursivamente a data de modificação de um ficheiro com o nome
 * especificado dentro da diretoria fornecida.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria onde a busca inicia.
 * - ficheiro (const string&): O nome do ficheiro cuja data de modificação se
 * pretende encontrar.
 *
 * Retorno:
 * - string*: Ponteiro para uma string contendo a data de modificação do
 * ficheiro, ou nullptr se não encontrado.
 */
optional<string> SistemaFicheiros::DataFicheiroRec(Diretoria *dir,
                                                   const string &ficheiro) {
  for (const auto &item : dir->getConteudo()) {
    if (item->getIsFicheiro() && item->getNome() == ficheiro) {
      Ficheiro *f = dynamic_cast<Ficheiro *>(item.get());
      if (f) {
        return f->getDataModificacao();
      }
    } else if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item.get());
      if (subdir) {
        optional<string> res = DataFicheiroRec(subdir, ficheiro);
        if (res)
          return res;
      }
    }
  }
  return nullopt;
}

/**
 * Resumo:
 * Exibe recursivamente a estrutura de diretoria e ficheiros, formatando a saída
 * com tabulações para indicar níveis hierárquicos.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria atual que está a ser exibida.
 * - nTabs (int&): Referência para o número atual de tabulações (nível de
 * profundidade).
 * - out (ostream&): Referência para o fluxo de saída onde a informação será
 * escrita.
 *
 * Retorno:
 * - void (Não retorna valor).
 */
void SistemaFicheiros::ShowRec(Diretoria *dir, size_t &nTabs, ostream &out) {
  out << Utils::Tabulacao(nTabs) << "<D> " << dir->getNome() << " <"
      << dir->getTamanho() << " bytes>" << endl;

  nTabs++;

  for (const auto &item : dir->getConteudo())
    if (item->getIsFicheiro())
      out << Utils::Tabulacao(nTabs) << "<F> " << item->getNome() << " <"
          << item->getTamanho() << " bytes>" << endl;
    else {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item.get());
      if (subdir)
        ShowRec(subdir, nTabs, out);
    }

  nTabs--;
}

/**
 * Resumo:
 * Percorre recursivamente a diretoria e as suas subdiretorias para localizar e
 * renomear todas as ocorrências de ficheiros com o nome 'fich_old' para
 * 'fich_new'. A função atualiza sempre a estrutura interna em memória.
 * Adicionalmente, se a flag 'importacao_diretoria' estiver ativa, tenta efetuar
 * a renomeação física no sistema de ficheiros do SO, lidando com conflitos de
 * nomes existentes.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria onde a pesquisa inicia.
 * - fich_old (const string&): O nome atual do ficheiro que se pretende alterar.
 * - fich_new (const string&): O novo nome a ser atribuído.
 * - importacao_diretoria (bool): Flag de controlo. Se 'true', altera o ficheiro
 * fisicamente no disco. Se 'false', altera apenas os metadados na memória do
 * programa.
 *
 * Retorno:
 * - int: O número total de ficheiros que foram renomeados com sucesso.
 */
int SistemaFicheiros::renomearFicheirosRec(Diretoria *dir,
                                           const string &fich_old,
                                           const string &fich_new,
                                           bool operarNoDisco) {
  int renomeados = 0;

  for (const auto &item : dir->getConteudo()) {
    if (item->getIsFicheiro()) {
      if (item->getNome() == fich_old) {

        fs::path oldPath(item->getCaminho());
        fs::path parent = oldPath.parent_path();
        fs::path newPath = parent / fich_new;

        try {
          // --- Se importacao_diretoria == 1 > mexe no filesystem ---
          if (operarNoDisco) {
            if (fs::exists(newPath)) {
              ostringstream ss;
              ss << "Aviso: destino já existe, saltando: " << newPath.string();
              Logger::log(Logger::Level::INFO, ss.str());
              continue;
            }

            fs::rename(oldPath, newPath);

            cout << "Renomeado no filesystem: " << oldPath.string() << " -> "
                 << newPath.string() << endl;
          }
          // --- Se importacao_diretoria == 0 > NÃO mexe no Filesystem ---
          else {
            cout << "Alterado internamente (sem mexer no Filesystem): "
                 << oldPath.string() << " -> " << newPath.string() << endl;
          }

          // Atualiza sempre os dados internos
          item->setNome(fich_new);
          item->setCaminho(newPath.string());

          renomeados++;

        } catch (const exception &e) {
          ostringstream ss;
          ss << "Erro ao renomear ficheiro " << item->getCaminho() << ": "
             << e.what();

          Logger::log(Logger::Level::ERROR_, ss.str());
        }
      }

    } else {
      Diretoria *sub = dynamic_cast<Diretoria *>(item.get());
      if (sub)
        renomeados +=
            renomearFicheirosRec(sub, fich_old, fich_new, operarNoDisco);
    }
  }

  return renomeados;
}

/**
 * Resumo:
 * Verifica recursivamente se existem ficheiros com nomes duplicados na
 * hierarquia de diretorias a partir da diretoria fornecida. Utiliza um
 * unordered_set para rastrear os nomes já encontrados.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria raiz onde a verificação deve
 * começar.
 * - nomes (unordered_set<string>&): Referência para o conjunto que armazena os
 * nomes dos ficheiros encontrados.
 *
 * Retorno:
 * - bool: Retorna 'true' se forem encontrados ficheiros com nomes duplicados,
 * ou 'false' caso contrário.
 */
bool SistemaFicheiros::VerificarDuplicadosRec(Diretoria *dir,
                                              unordered_set<string> &nomes) {
  for (const auto &item : dir->getConteudo()) {
    if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item.get());
      // Verifica recursivamente; se encontrar duplicado, retorna true
      if (VerificarDuplicadosRec(subdir, nomes)) {
        return true;
      }
    } else {
      // se e um ficheiro; tenta inserir no unordered_set
      if (!nomes.insert(item->getNome()).second) {
        return true; // duplicado encontrado
      }
    }
  }
  return false; // nenhum duplicado encontrado
}

/**
 * Resumo:
 * Percorre os itens de uma diretoria para detetar e corrigir nomes duplicados.
 * Utiliza um mapa auxiliar para controlar a frequência de cada nome. Caso um
 * nome já tenha sido registado anteriormente, gera um novo nome único para o
 * item atual (adicionando um sufixo numérico via Utils) e atualiza o objeto,
 * distinguindo entre lógica de ficheiros (com extensão) e diretorias.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria cujo conteúdo será verificado.
 * - contador (unordered_map<string, int>&): Referência para o mapa que armazena
 * a contagem de ocorrências de cada nome, permitindo identificar duplicados.
 *
 * Retorno:
 * - void (Não retorna valor).
 */
void SistemaFicheiros::AlterarNomeDuplicado(
    Diretoria *dir, unordered_map<string, int> &contador) {
  for (const auto &item : dir->getConteudo()) {
    const string nomeOriginal = item->getNome();
    int &proximoNumero = contador[nomeOriginal];
    if (proximoNumero == 0) {
      proximoNumero = 1;
      continue;
    }

    string candidato;
    do {
      if (item->getIsFicheiro()) {
        const auto *ficheiro = dynamic_cast<const Ficheiro *>(item.get());
        candidato = Utils::alterarNomeDuplicado(
            nomeOriginal, proximoNumero++, ficheiro->getExtensao());
      } else {
        candidato = nomeOriginal + Utils::gerarSufixo(proximoNumero++);
      }
    } while (contador[candidato] != 0);

    contador[candidato] = 1;
    item->setNome(candidato);
  }
}

/**
 * Resumo:
 * Copia recursivamente os ficheiros de uma diretoria de origem para uma
 * diretoria de destino, filtrando-os com base num padrão de nome. Para cada
 * ficheiro que corresponde ao padrão, cria uma cópia em memória e, se indicado,
 * copia o ficheiro fisicamente para o sistema de ficheiros na diretoria
 * destino.
 *
 * Parâmetros:
 * - dirOrigem (Diretoria*): Ponteiro para a diretoria de onde os ficheiros
 * serão copiados.
 * - destino (Diretoria*): Ponteiro para a diretoria onde os ficheiros serão
 * colados.
 * - padrao (const string&): Padrão de nome usado para filtrar quais ficheiros
 * serão copiados.
 * - disco (bool): Indica se os ficheiros devem ser copiados fisicamente no
 * sistema de ficheiros.
 * - importacao_diretoria (bool): Indica se a estrutura foi carregada a partir
 * de uma diretoria (true) ou de um XML (false).
 *
 * Retorno:
 * - void (Não retorna valor).
 */
bool SistemaFicheiros::copiarItemRec(Diretoria *dirOrigem, Diretoria *destino,
                                     const string &padrao, bool disco,
                                     bool operarNoDisco) {
  bool sucesso = true;
  for (const auto &item : dirOrigem->getConteudo()) {
    if (item->getIsFicheiro()) {
      Ficheiro *f = dynamic_cast<Ficheiro *>(item.get());
      if (!Utils::contemPalavra(f->getNome(), padrao)) {
        continue; // não corresponde ao padrão
      }

      // 1. Criar objeto em memória
      auto copia = make_unique<Ficheiro>(*f);
      const fs::path caminhoDestino =
          fs::path(destino->getCaminho()) / copia->getNome();

      if (disco && operarNoDisco) {
        try {
          fs::copy_file(f->getCaminho(), caminhoDestino);
        } catch (const exception &e) {
          ostringstream ss;
          ss << "Erro ao copiar ficheiro " << f->getCaminho() << " para "
             << destino->getCaminho() + "/" + f->getNome() << ": " << e.what();

          Logger::log(Logger::Level::ERROR_, ss.str());
          sucesso = false;
          continue;
        }
      }

      copia->setCaminho(caminhoDestino.string());
      destino->adicionar(move(copia));
    } else {
      // Recursão: percorrer subdiretórios
      Diretoria *d = dynamic_cast<Diretoria *>(item.get());
      if (!copiarItemRec(d, destino, padrao, disco, operarNoDisco))
        sucesso = false;
    }
  }
  return sucesso;
}

//==================== Métodos Públicos ====================
/**
 * Resumo:
 * Inicializa a estrutura do sistema de ficheiros a partir de um caminho físico
 * no disco. Valida se o caminho existe e é uma diretoria. Cria o nó raiz da
 * árvore e invoca o carregamento recursivo de todo o conteúdo (ficheiros e
 * subpastas). Em caso de sucesso, ativa a flag 'importacao_diretoria'. Em caso
 * de erro, regista a ocorrência no Logger.
 *
 * Parâmetros:
 * - path (const string&): O caminho (absoluto ou relativo) para a diretoria
 * raiz que se pretende carregar.
 *
 * Retorno:
 * - bool: Retorna 'true' se o carregamento for concluído com sucesso,
 * ou 'false' se o caminho for inválido ou ocorrer uma exceção.
 */
bool SistemaFicheiros::Load(const string &path) {
  error_code error;
  const fs::path caminho = fs::weakly_canonical(fs::path(path), error);
  if (error || !fs::is_directory(caminho, error) || error) {
    cerr << "Erro: caminho inválido -> " << path << endl;
    return false;
  }

  const string nome = caminho.filename().string();
  if (nome.empty())
    return false;

  try {
    auto novaRaiz = make_unique<Diretoria>(nome, caminho.string());
    carregarConteudo(novaRaiz.get());
    novaRaiz->recalcularTamanho();
    raiz = move(novaRaiz);
  } catch (const exception &e) {
    ostringstream ss;
    ss << "Erro ao carregar sistema de ficheiros: " << e.what();

    Logger::log(Logger::Level::ERROR_, ss.str());
    return false;
  }

  importacao_diretoria = true;
  return true;
}

/**
 * Resumo:
 * Método público que retorna o número total de ficheiros carregados no sistema.
 * Verifica se a estrutura (raiz) está inicializada e delega a contagem para
 * a função recursiva interna 'ContarFicheirosRec'.
 *
 * Parâmetros:
 * - Nenhum (utiliza o membro interno 'raiz').
 *
 * Retorno:
 * - int: O número total de ficheiros encontrados. Retorna 0 se o sistema
 * não tiver sido carregado (raiz nula).
 */
int SistemaFicheiros::ContarFicheiros() {
  if (raiz == nullptr)
    return 0;
  return ContarFicheirosRec(raiz.get());
}

/**
 * Resumo:
 * Método público que retorna o número total de diretorias carregadas no
 * sistema. Verifica se a estrutura (raiz) está inicializada e delega a contagem
 * para a função recursiva interna 'ContarDirectoriasRec'.
 *
 * Parâmetros:
 * - Nenhum (utiliza o membro interno 'raiz').
 *
 * Retorno:
 * - int: O número total de diretorias encontradas. Retorna 0 se o sistema
 * não tiver sido carregado (raiz nula).
 */
int SistemaFicheiros::ContarDirectorias() {
  if (raiz == nullptr)
    return 0;
  return ContarDirectoriasRec(raiz.get());
}

/* Resumo:
 * Método público que retorna a quantidade total de espaço (em bytes)
 * ocupada pelos ficheiros carregados no sistema.
 * Delega o cálculo para a função recursiva interna 'memoriaRec'.
 *
 * Parâmetros:
 * - Nenhum (utiliza o membro interno 'raiz').
 *
 * Retorno:
 * - int: A quantidade total de memória em bytes. Retorna 0 se o sistema
 * não tiver sido carregado (raiz nula).
 */
int SistemaFicheiros::Memoria() {
  const uintmax_t total = memoriaRec(raiz.get());
  const auto limite = static_cast<uintmax_t>(numeric_limits<int>::max());
  return static_cast<int>(min(total, limite));
}

/**
 * Resumo:
 * Método público que retorna a diretoria com mais elementos (ficheiros e
 * subdiretórios). Delega a busca para a função recursiva interna
 * 'maiorDiretoriaRec'.
 *
 * Parâmetros:
 * - Nenhum (utiliza o membro interno 'raiz').
 *
 * Retorno:
 * - string*: Ponteiro para o nome da diretoria com mais elementos. Retorna
 * nullptr se o sistema não tiver sido carregado (raiz nula).
 */
string *SistemaFicheiros::DirectoriaMaisElementos() {
  if (!raiz)
    return nullptr;

  size_t maior = 0;
  return new string(maiorDiretoriaRec(raiz.get(), maior));
}

/**
 * Resumo:
 * Método público que retorna a diretoria com menos elementos (ficheiros e
 * subdiretórios). Delega a busca para a função recursiva interna
 * 'menorDiretoriaRec'.
 *
 * Parâmetros:
 * - Nenhum (utiliza o membro interno 'raiz').
 *
 * Retorno:
 * - string*: Ponteiro para o nome da diretoria com menos elementos. Retorna
 * nullptr se o sistema não tiver sido carregado (raiz nula).
 */
string *SistemaFicheiros::DirectoriaMenosElementos() {
  if (!raiz)
    return nullptr;

  size_t menor = 0;
  return new string(menorDiretoriaRec(raiz.get(), menor));
}

/**
 * Resumo:
 * Método público que retorna o ficheiro com o maior tamanho.
 * Delega a busca para a função recursiva interna 'ficheiroMaiorRec'.
 *
 * Parâmetros:
 * - Nenhum (utiliza o membro interno 'raiz').
 *
 * Retorno:
 * - string*: Ponteiro para o nome do ficheiro com maior tamanho. Retorna
 * nullptr se o sistema não tiver sido carregado (raiz nula).
 */
string *SistemaFicheiros::FicheiroMaior() {
  if (raiz == nullptr)
    return nullptr;

  uintmax_t tamMax = 0;

  string strMax;
  ficheiroMaiorRec(raiz.get(), tamMax, strMax);
  return new string(move(strMax));
}

/**
 * Resumo:
 * Método público que retorna a diretoria que ocupa mais espaço em memória.
 * Delega a busca para a função recursiva interna 'diretoriaMaisEspaco'.
 *
 * Parâmetros:
 * - Nenhum (utiliza o membro interno 'raiz').
 *
 * Retorno:
 * - string*: Ponteiro para o nome da diretoria com mais espaço ocupado. Retorna
 * nullptr se o sistema não tiver sido carregado (raiz nula).
 */
string *SistemaFicheiros::DirectoriaMaisEspaco() {
  if (raiz == nullptr)
    return nullptr;

  uintmax_t tamMax = 0;

  string strMax;
  diretoriaMaisEspaco(raiz.get(), tamMax, strMax);
  return new string(move(strMax));
}

/**
 * Resumo:
 * Método público que realiza uma busca por nome, podendo ser por diretoria ou
 * ficheiro. Delega a busca para as funções recursivas internas
 * 'pesquisarDiretoriaRec' ou 'pesquisarFicheiroRec'.
 *
 * Parâmetros:
 * - const string &s: Nome a ser pesquisado.
 * - int Tipo: Tipo de item a ser pesquisado (1 para diretoria, 0 para
 * ficheiro).
 *
 * Retorno:
 * - string*: Ponteiro para o nome do item encontrado. Retorna nullptr se não
 * encontrado ou se o sistema não tiver sido carregado (raiz nula).
 */
string *SistemaFicheiros::Search(const string &s, int Tipo) {
  if (!raiz)
    return nullptr;

  optional<string> resultado;

  if (Tipo == 1)
    resultado = pesquisarDiretoriaRec(raiz.get(), s);
  else if (Tipo == 0)
    resultado = pesquisarFicheiroRec(raiz.get(), s);

  if (resultado)
    return new string(move(*resultado));

  return nullptr;
}

/**
 * Resumo:
 * Método público que inicia o processo de remoção de itens com base num nome e
 * tipo. Invoca a função interna 'RemovePorNome' a partir da raiz do sistema de
 * ficheiros. A operação respeita a flag global 'importacao_diretoria' para
 * decidir se a remoção deve ser refletida fisicamente no disco ou apenas na
 * memória.
 *
 * Parâmetros:
 * - s (const string&): O nome do ficheiro ou diretoria que se pretende remover.
 * - tipo (const string&): O tipo de item a filtrar na remoção (ex: "ficheiro",
 * "diretoria").
 *
 * Retorno:
 * - bool: Retorna 'true' se a operação de remoção foi bem-sucedida (ou se itens
 * foram encontrados e removidos), ou 'false' caso contrário.
 */
bool SistemaFicheiros::RemoverAll(const string &s, const string &tipo) {
  if (!raiz)
    return false;
  const bool removido =
      RemovePorNome(raiz.get(), s, tipo, true, importacao_diretoria);
  if (removido)
    raiz->recalcularTamanho();
  return removido;
}

/**
 * Resumo:
 * Exporta a estrutura do sistema de ficheiros para um ficheiro XML.
 * Inicia o documento XML, chama a função recursiva para escrever
 * o conteúdo da raiz e finaliza o documento.
 *
 * Parâmetros:
 * - s (const string&): O nome do ficheiro XML onde os dados serão exportados.
 *
 * Retorno:
 * - void (Não retorna valor).
 */
void SistemaFicheiros::Escrever_XML(const string &s) {
  if (!raiz) {
    Logger::log(Logger::Level::ERROR_,
                "Não é possível exportar um sistema não carregado.");
    return;
  }
  XML XML;
  XML.WriteStartDocument(s);
  escreverXMLRec(raiz.get(), &XML);
  XML.WriteEndDocument();
}

/**
 * Resumo:
 * Importa a estrutura do sistema de ficheiros a partir de um ficheiro XML.
 * A nova árvore é construída temporariamente e apenas substitui o estado atual
 * depois de o documento ser validado e lido por completo.
 *
 * Parâmetros:
 * - s (const string&): O nome do ficheiro XML de onde os dados serão
 * importados.
 *
 * Retorno:
 * - bool: Retorna 'true' se a importação foi bem-sucedida, 'false' caso
 * contrário.
 */
bool SistemaFicheiros::Ler_XML(const string &s) {
  XML xmlParser;
  ifstream ficheiro = xmlParser.ImportDocument(s);

  if (!ficheiro.is_open()) {
    Logger::log(Logger::Level::ERROR_, "Erro ao importar documento XML: " + s);
    return false;
  }

  string linha;
  regex regexDiretoriaAberta(
      "<diretoria\\s+nome=\"([^\"]+)\"\\s+tamanho=\"([^\"]+)\">");
  smatch match;

  try {
    while (getline(ficheiro, linha)) {
      if (!linha.empty() && linha.back() == '\r')
        linha.pop_back();

      if (regex_search(linha, match, regexDiretoriaAberta)) {
        const string nome = match[1];
        auto novaRaiz = make_unique<Diretoria>(nome, "./" + nome);

        if (!xmlParser.ReadDirectory(ficheiro, novaRaiz.get())) {
          Logger::log(Logger::Level::ERROR_,
                      "XML inválido ou incompleto: " + s);
          return false;
        }

        novaRaiz->recalcularTamanho();
        raiz = move(novaRaiz);
        importacao_diretoria = false;
        return true;
      }
    }
  } catch (const exception &e) {
    Logger::log(Logger::Level::ERROR_,
                "Erro ao interpretar XML '" + s + "': " + e.what());
    return false;
  }

  Logger::log(Logger::Level::ERROR_,
              "XML inválido: Nenhuma diretoria raiz encontrada em " + s);
  return false;
}

/**
 * Resumo:
 * Movimenta um ficheiro de uma diretoria para outra. O processo envolve:
 * 1. Localizar o ficheiro e a diretoria de destino na estrutura.
 * 2. Identificar a diretoria atual do ficheiro (pai).
 * 3. Validar a operação (ex: impedir mover para a mesma pasta).
 * 4. Se a flag 'importacao_diretoria' estiver ativa, executa o movimento físico
 * no disco.
 * 5. Atualiza a estrutura lógica interna: remove o registo da diretoria antiga,
 * adiciona à nova e atualiza o atributo 'caminho' do objeto.
 *
 * Parâmetros:
 * - Fich (const string&): Nome do ficheiro que se pretende mover.
 * - DirNova (const string&): Nome da diretoria de destino.
 *
 * Retorno:
 * - bool: Retorna 'true' se o movimento for bem-sucedido. Retorna 'false' e
 * regista erro no Logger caso falhe a validação ou ocorra erro de I/O.
 */
bool SistemaFicheiros::MoveFicheiro(const string &Fich, const string &DirNova) {
  if (!raiz)
    return false;

  Item *item =
      procurarItemRec(raiz.get(), Fich, false); // procurar ficheiro pretendido
  if (!item) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: ficheiro '" + Fich + "' não encontrado.");
    return false;
  }

  Item *dirNovaItem;
  if (DirNova == raiz->getNome()) {
    dirNovaItem = raiz.get();
  } else {
    dirNovaItem = procurarItemRec(raiz.get(), DirNova,
                                  true); // procurar diretoria nova pelo nome
  }

  if (!dirNovaItem || dirNovaItem->getIsFicheiro()) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: diretoria '" + DirNova + "' não encontrada.");
    return false;
  }

  Diretoria *dirNova = dynamic_cast<Diretoria *>(
      dirNovaItem); // transforma em ponteiro para diretoria
  if (!dirNova) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: falha ao converter para diretoria.");
    return false;
  }

  // Verificação de duplicados na diretoria destino
  for (const auto &conteudo : dirNova->getConteudo()) {
    if (conteudo->getNome() == item->getNome()) {
      Logger::log(Logger::Level::ERROR_,
                  "Erro: Já existe um item com o nome '" + item->getNome() +
                      "' na diretoria destino.");
      return false; // Aborta a função sem apagar nada
    }
  }

  Diretoria *diretoriaAntiga = procurarDiretoriaPai(raiz.get(), item);
  if (!diretoriaAntiga) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: diretoria de origem do ficheiro não encontrada.");
    return false;
  }

  if (diretoriaAntiga->getCaminho() == dirNova->getCaminho()) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: o ficheiro já se encontra na diretoria '" + DirNova +
                    "'.");
    return false;
  }

  const fs::path caminhoAntigo(item->getCaminho());
  const fs::path novoCaminho = fs::path(dirNova->getCaminho()) / item->getNome();

  if (importacao_diretoria) {
    try {
      fs::rename(caminhoAntigo, novoCaminho);
    } catch (const fs::filesystem_error &e) {
      Logger::log(Logger::Level::ERROR_,
                  "Erro ao mover ficheiro: " + string(e.what()));
      return false;
    }
  }

  // remover o item da diretoria antiga
  auto itemMovido = extrairItemPorNome(diretoriaAntiga, item->getNome());
  if (!itemMovido) {
    if (importacao_diretoria) {
      error_code rollbackError;
      fs::rename(novoCaminho, caminhoAntigo, rollbackError);
    }
    return false;
  }

  // adicionar o item à nova diretoria
  Item *itemMovidoPtr = itemMovido.get();
  dirNova->adicionar(move(itemMovido));

  itemMovidoPtr->setCaminho(novoCaminho.string());
  raiz->recalcularTamanho();

  cout << "Ficheiro '" << Fich << "' movido com sucesso para '" << DirNova
       << "'.\n";

  return true;
}

/**
 * Resumo:
 * Move uma diretoria completa (incluindo todo o seu conteúdo) para uma nova
 * diretoria de destino. Realiza validações críticas para impedir ciclos (mover
 * uma pasta para dentro de si própria ou de uma subdiretoria sua) e erros de
 * referência. Executa a movimentação física no disco (se a importação estiver
 * ativa) e atualiza a árvore em memória, ajustando recursivamente os caminhos
 * de todos os itens movidos através de 'setCaminhoRec'.
 *
 * Parâmetros:
 * - DirOld (const string&): Nome da diretoria de origem que se pretende mover.
 * - DirNew (const string&): Nome da diretoria de destino onde a 'DirOld' será
 * colocada.
 *
 * Retorno:
 * - bool: Retorna 'true' se a movimentação for bem-sucedida, ou 'false' caso
 * ocorra algum erro de validação (ex: destino inválido, ciclo detetado) ou de
 * I/O.
 */
bool SistemaFicheiros::MoverDirectoria(const string &DirOld,
                                       const string &DirNew) {
  if (!raiz)
    return false;

  Item *item = procurarItemRec(raiz.get(), DirOld, true);
  if (!item) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: diretoria '" + DirOld + "' não encontrada.");
    return false;
  }

  Diretoria *itemDir = dynamic_cast<Diretoria *>(item);
  if (!itemDir) {
    Logger::log(Logger::Level::ERROR_, "Erro: o item não é uma diretoria.");
    return false;
  }

  Item *dirNovaItem;
  if (DirNew == raiz->getNome()) {
    dirNovaItem = raiz.get();
  } else {
    dirNovaItem = procurarItemRec(raiz.get(), DirNew,
                                  true); // procurar diretoria nova pelo nome
  }

  if (!dirNovaItem || dirNovaItem->getIsFicheiro()) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: diretoria '" + DirNew + "' não encontrada.");
    return false;
  }

  Diretoria *dirNova = dynamic_cast<Diretoria *>(dirNovaItem);
  if (!dirNova) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: falha ao converter para diretoria.");
    return false;
  }

  // impedir mover para si própria ou para uma subdiretoria
  if (contemDiretoria(itemDir, dirNova)) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: não é possível mover uma diretoria para ela mesma ou "
                "para uma subdiretoria dela.");
    return false;
  }

  for (const auto &conteudo : dirNova->getConteudo()) {
    if (conteudo->getNome() == itemDir->getNome()) {
      Logger::log(Logger::Level::ERROR_,
                  "Erro: já existe um item com o mesmo nome no destino.");
      return false;
    }
  }

  Diretoria *diretoriaAntiga = procurarDiretoriaPai(raiz.get(), itemDir);

  if (!diretoriaAntiga) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: diretoria de origem não encontrada.");
    return false;
  }

  const fs::path caminhoAntigo(itemDir->getCaminho());
  const fs::path novoCaminho = fs::path(dirNova->getCaminho()) / itemDir->getNome();

  // mover no sistema de ficheiros
  if (importacao_diretoria) {
    try {
      fs::rename(caminhoAntigo, novoCaminho);
    } catch (const fs::filesystem_error &e) {
      Logger::log(Logger::Level::ERROR_,
                  "Erro ao mover diretoria: " + string(e.what()));
      return false;
    }
  }

  // atualizar estrutura da árvore (REMOVER + ADICIONAR apenas uma vez!)
  auto diretoriaMovida = extrairItemPorNome(diretoriaAntiga, itemDir->getNome());
  if (!diretoriaMovida) {
    if (importacao_diretoria) {
      error_code rollbackError;
      fs::rename(novoCaminho, caminhoAntigo, rollbackError);
    }
    return false;
  }
  Diretoria *diretoriaMovidaPtr =
      dynamic_cast<Diretoria *>(diretoriaMovida.get());
  dirNova->adicionar(move(diretoriaMovida));

  // atualizar caminhos internos
  string novoCaminhoString = novoCaminho.string();
  setCaminhoRec(diretoriaMovidaPtr, novoCaminhoString);
  raiz->recalcularTamanho();

  cout << "Diretoria '" << DirOld << "' movida com sucesso!\n";
  return true;
}

/**
 * Resumo:
 * Método público que obtém a data de modificação de um ficheiro específico.
 * Atua como um ponto de entrada, delegando a pesquisa para a função recursiva
 * interna 'DataFicheiroRec', iniciando a busca a partir da diretoria raiz.
 *
 * Parâmetros:
 * - ficheiro (const string&): O nome do ficheiro que se pretende consultar.
 *
 * Retorno:
 * - string*: Ponteiro para a string contendo a data do ficheiro.
 * Retorna nullptr (dependendo da implementação interna) se o ficheiro não for
 * encontrado.
 */
string *SistemaFicheiros::DataFicheiro(const string &ficheiro) {
  if (!raiz)
    return nullptr;

  optional<string> resultado = DataFicheiroRec(raiz.get(), ficheiro);
  if (!resultado)
    return nullptr;
  return new string(move(*resultado));
}

/**
 * Resumo:
 * Gera e exibe a representação visual (estrutura em árvore) de todo o sistema
 * de ficheiros. A função realiza a operação em duas etapas: primeiro imprime a
 * árvore na consola (std::cout) para visualização imediata e, em seguida, grava
 * a mesma estrutura num ficheiro de texto especificado, permitindo guardar o
 * estado atual da hierarquia.
 *
 * Parâmetros:
 * - fich (const string*): Ponteiro para a string que contém o nome (ou caminho)
 * do ficheiro de texto onde a árvore será gravada.
 *
 * Retorno:
 * - void (Não retorna valor).
 */
void SistemaFicheiros::Tree(const string *fich) {
  if (!raiz)
    return;

  const string ficheiroPadrao = "tree.txt";
  const string &destino = fich ? *fich : ficheiroPadrao;
  size_t nivel = 0;

  ShowRec(raiz.get(), nivel, cout);

  nivel = 0;

  ofstream f(destino);

  ShowRec(raiz.get(), nivel, f);

  f.close();
}

/**
 * Resumo:
 * Método público que pesquisa todas as ocorrências de diretorias com um nome
 * específico em todo o sistema de ficheiros. Antes de iniciar a pesquisa, limpa
 * a lista 'lres' para garantir que não contém dados antigos. Invoca a função
 * recursiva interna 'pesquisarItensComNomeIgualRec', configurada para buscar
 * apenas diretorias (flag false).
 *
 * Parâmetros:
 * - lres (list<string>&): Referência para uma lista onde serão guardados os
 * caminhos ou nomes das diretorias encontradas.
 * - dir (const string&): O nome da diretoria que se pretende pesquisar.
 *
 * Retorno:
 * - void (Não retorna valor; os resultados são acumulados na lista passada por
 * referência).
 */
void SistemaFicheiros::PesquisarAllDirectorias(list<string> &lres,
                                               const string &dir) {
  lres.clear(); // limpa potenciais resultados antigos que possam estar na lista
  if (!raiz)
    return;
  pesquisarItensComNomeIgualRec(raiz.get(), lres, dir, false);
}

/**
 * Resumo:
 * Método público que pesquisa todas as ocorrências de ficheiros com um nome
 * específico em todo o sistema de ficheiros. Tal como na pesquisa de
 * diretorias, limpa a lista de resultados 'lres' antes de iniciar a operação
 * para evitar dados lixo. Invoca a função recursiva interna
 * 'pesquisarItensComNomeIgualRec', configurada com a flag 'true' para filtrar
 * apenas ficheiros.
 *
 * Parâmetros:
 * - lres (list<string>&): Referência para uma lista onde serão guardados os
 * caminhos dos ficheiros encontrados.
 * - file (const string&): O nome do ficheiro que se pretende pesquisar.
 *
 * Retorno:
 * - void (Não retorna valor; os resultados são acumulados na lista passada por
 * referência).
 */
void SistemaFicheiros::PesquisarAllFicheiros(list<string> &lres,
                                             const string &file) {
  lres.clear(); // limpa potenciais resultados antigos que possam estar na lista
  if (!raiz)
    return;
  pesquisarItensComNomeIgualRec(raiz.get(), lres, file, true);
}

/**
 * Resumo:
 * Método público responsável por iniciar a renomeação em massa de ficheiros.
 * Verifica se o sistema de ficheiros está carregado e, em caso afirmativo,
 * invoca a função recursiva interna 'renomearFicheirosRec' para processar
 * toda a árvore. No final, exibe uma mensagem na consola indicando quantos
 * ficheiros foram efetivamente alterados ou se nenhum foi encontrado.
 *
 * Parâmetros:
 * - fich_old (const string&): O nome atual do ficheiro a ser substituído.
 * - fich_new (const string&): O novo nome que será atribuído.
 *
 * Retorno:
 * - void (Não retorna valor, mas imprime o resultado da operação na consola).
 */
void SistemaFicheiros::RenomearFicheiros(const string &fich_old,
                                         const string &fich_new) {
  if (raiz == nullptr) {
    Logger::log(Logger::Level::ERROR_, "Erro: sistema não carregado.");
    return;
  }

  int total =
      renomearFicheirosRec(raiz.get(), fich_old, fich_new,
                           importacao_diretoria);

  if (total == 0)
    cout << "Nenhum ficheiro encontrado com o nome '" << fich_old << "'.\n";
  else
    cout << "Operação concluída. Ficheiros renomeados: " << total << "\n";
}

/**
 * Resumo:
 * Verifica se existem ficheiros com nomes duplicados em todo o sistema de
 * ficheiros. Este método serve como ponto de entrada, inicializando um conjunto
 * (Set) para armazenar os nomes já processados e invocando a função recursiva
 * 'VerificarDuplicadosRec' para percorrer a árvore e detetar colisões.
 *
 * Parâmetros:
 * - Nenhum.
 *
 * Retorno:
 * - bool: Retorna 'true' se forem encontrados ficheiros duplicados,
 * ou 'false' se todos os nomes forem únicos.
 */
bool SistemaFicheiros::FicheiroDuplicados() {
  if (!raiz)
    return false;
  unordered_set<string> nomes; // guarda nomes únicos~
  return VerificarDuplicadosRec(raiz.get(), nomes);
}

/**
 * Resumo:
 * Executa a cópia em lote de ficheiros que correspondam a um determinado
 * padrão, transferindo-os da diretoria de origem para a de destino. O processo
 * utiliza uma estrutura temporária em memória para filtrar os itens e resolver
 * conflitos de nomes (duplicados) antes de consolidar a cópia na diretoria
 * final.
 *
 * Parâmetros:
 * - padrao (const string&): O padrão de texto (ex: extensão ou parte do nome)
 * para filtrar os ficheiros a copiar.
 * - DirOrigem (const string&): Nome da diretoria onde estão os ficheiros
 * originais.
 * - DirDestino (const string&): Nome da diretoria para onde os ficheiros serão
 * copiados.
 *
 * Retorno:
 * - bool: Retorna 'true' se as diretorias forem válidas e o processo concluir,
 * ou 'false' se a origem/destino não forem encontrados.
 */
bool SistemaFicheiros::CopyBatch(const string &padrao, const string &DirOrigem,
                                 const string &DirDestino) {
  if (!raiz)
    return false;
  Item *diretoriaOrigem = procurarItemRec(raiz.get(), DirOrigem, true);
  if (!diretoriaOrigem) {
    Logger::log(Logger::Level::ERROR_, "Erro: diretoria de origem '" +
                                           DirOrigem + "' não encontrada.");
    return false;
  }

  Diretoria *dirOrigem = dynamic_cast<Diretoria *>(diretoriaOrigem);
  if (!dirOrigem) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: '" + DirOrigem + "' não é uma diretoria válida.");
    return false;
  }

  Item *diretoriaDestino = procurarItemRec(raiz.get(), DirDestino, true);
  if (!diretoriaDestino) {
    Logger::log(Logger::Level::ERROR_, "Erro: diretoria de destino '" +
                                           DirDestino + "' não encontrada.");
    return false;
  }

  Diretoria *dirDestino = dynamic_cast<Diretoria *>(diretoriaDestino);
  if (!dirDestino) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: '" + DirDestino + "' não é uma diretoria válida.");
    return false;
  }

  unordered_map<string, int> contador;
  auto temp = make_unique<Diretoria>("temp", dirOrigem->getCaminho());

  copiarItemRec(dirOrigem, temp.get(), padrao, false, importacao_diretoria);
  for (const auto &item : dirDestino->getConteudo())
    ++contador[item->getNome()];
  AlterarNomeDuplicado(temp.get(), contador);
  const bool sucesso = copiarItemRec(temp.get(), dirDestino, padrao, true,
                                     importacao_diretoria);

  raiz->recalcularTamanho();
  return sucesso;
}
