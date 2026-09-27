#include "SistemaFicheiros.h"
#include "Ficheiro.h"
#include "Logger.h"
#include "Utils.h"
#include "XML.h"

//==================== Construtor e Destrutor ====================
SistemaFicheiros::SistemaFicheiros()
    : raiz(nullptr), importacao_diretoria(false) {}

SistemaFicheiros::~SistemaFicheiros() { delete raiz; }

//==================== Métodos Privados ====================
/**
 * Resumo:
 * Percorre recursivamente o sistema de ficheiros a partir do caminho da
 * diretoria fornecida. Para cada entrada encontrada (ficheiro ou pasta), cria o
 * respetivo objeto ('Ficheiro' ou 'Diretoria') e adiciona-o à estrutura em
 * memória. Em caso de falha (ex: permissões negadas), captura a exceção e
 * regista o erro no Logger.
 *
 * Parâmetros:
 * - diretoria (Diretoria*): Ponteiro para o objeto diretoria que será
 * preenchido com o conteúdo encontrado no disco.
 *
 * Retorno:
 * - void (Não retorna valor).
 */
void SistemaFicheiros::carregarConteudo(Diretoria *diretoria) {
  try {
    // Usamos o caminho da diretoria recebida
    for (auto &entry : fs::directory_iterator(diretoria->getCaminho())) {

      string nome = entry.path().filename().string();
      string caminhoCompleto = entry.path().string();

      if (fs::is_directory(entry.path())) {
        Diretoria *sub = new Diretoria(nome, caminhoCompleto);
        carregarConteudo(sub);     // chamada recursiva com o parâmetro
        diretoria->adicionar(sub); // adicionamos à diretoria recebida
      } else if (fs::is_regular_file(entry.path())) {
        Ficheiro *f = new Ficheiro(nome, caminhoCompleto);
        diretoria->adicionar(f);
      }
    }
  } catch (const exception &e) {
    ostringstream ss;
    ss << "Erro ao carregar diretoria " << diretoria->getCaminho() << ": "
       << e.what();

    Logger::log(Logger::Level::ERROR_, ss.str());
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
  for (auto item : dir->getConteudoConst()) {
    if (item->getIsFicheiro()) {
      total++;
    } else {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item);
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
  for (auto item : dir->getConteudoConst()) {
    if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item);
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
string *SistemaFicheiros::ficheiroMaiorRec(Diretoria *dir, uintmax_t &tamMax,
                                           string &strMax) {
  if (!dir)
    return nullptr;

  for (auto item : dir->getConteudo()) {
    if (item->getIsFicheiro()) {
      if (item->getTamanho() > tamMax) {
        tamMax = item->getTamanho();
        strMax = item->getCaminho();
      }
    } else {
      // é diretoria → recursão
      Diretoria *diretoria = dynamic_cast<Diretoria *>(item);
      if (diretoria) {
        ficheiroMaiorRec(diretoria, tamMax, strMax);
      }
    }
  }

  return &strMax;
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
int SistemaFicheiros::memoriaRec(Diretoria *dir) {
  if (!dir)
    return 0;

  int total = 0;

  for (auto item : dir->getConteudo()) {
    if (item->getIsFicheiro()) {
      total += sizeof(*item);
    } else {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item);
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
string *SistemaFicheiros::maiorDiretoriaRec(Diretoria *dir, int *maior) {
  int localMax = dir->getNItens();
  string localName = dir->getNome();

  for (const auto &item : dir->getConteudo()) {
    if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item);
      int nItem = dir->getNItens();
      if (nItem > localMax) {
        localMax = nItem;
        localName = subdir->getNome();
      }

      int subMax = 0;
      string *nomeSub = maiorDiretoriaRec(subdir, &subMax);

      if (nomeSub) {
        if (subMax > localMax) {
          localMax = subMax;
          localName = *nomeSub;
        }
        delete nomeSub;
      }
    }
  }

  if (maior)
    *maior = localMax;

  return new string(localName);
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
string *SistemaFicheiros::menorDiretoriaRec(Diretoria *dir, int *menor) {
  int localMin = dir->getNItens();
  string localName = dir->getNome();

  for (const auto &item : dir->getConteudo()) {
    if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item);
      int nItem = subdir->getNItens();
      if (nItem < localMin) {
        localMin = nItem;
        localName = subdir->getNome();
      }

      int subMin = INT_MAX;
      string *nomeSub = menorDiretoriaRec(subdir, &subMin);

      if (nomeSub) {
        if (subMin < localMin) {
          localMin = subMin;
          localName = *nomeSub;
        }
        delete nomeSub;
      }
    }
  }

  if (menor)
    *menor = localMin;

  return new string(localName);
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
string *SistemaFicheiros::diretoriaMaisEspaco(Diretoria *dir, uintmax_t tamMax,
                                              string &strMax) {
  for (auto c : dir->getConteudo()) {
    if (!(c->getIsFicheiro())) {
      if (c->getTamanho() > tamMax) {
        tamMax = c->getTamanho();
        strMax = c->getCaminho();
      }
    }
  }
  return &strMax;
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
string *SistemaFicheiros::pesquisarDiretoriaRec(Diretoria *dir,
                                                const string &s) {
  if (dir->getNome() == s) {
    return new string(dir->getCaminho());
  }

  for (Item *item : dir->getConteudoConst()) {

    if (!item->getIsFicheiro() && item->getNome() == s) {
      return new string(item->getCaminho());
    }

    if (!item->getIsFicheiro()) {
      Diretoria *d = dynamic_cast<Diretoria *>(item);
      if (d) {
        string *res = pesquisarDiretoriaRec(d, s);
        if (res)
          return res;
      }
    }
  }

  return nullptr;
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
string *SistemaFicheiros::pesquisarFicheiroRec(Diretoria *dir,
                                               const string &s) {
  for (Item *item : dir->getConteudoConst()) {

    if (item->getIsFicheiro() && item->getNome() == s) {
      return new string(item->getCaminho());
    }

    if (!item->getIsFicheiro()) {
      Diretoria *d = dynamic_cast<Diretoria *>(item);
      if (d) {
        string *res = pesquisarFicheiroRec(d, s);
        if (res)
          return res;
      }
    }
  }

  return nullptr;
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

  for (auto item : dir->getConteudo()) {
    if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item);
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
                                     bool importacao_diretoria) {
  bool t = (tipo == "DIR") ? false : true;

  list<string> lres;
  pesquisarItensComNomeIgualRec(dir, lres, s, t);

  Utils::PrintListaString(lres);

  if (lres.empty()) {
    return false;
  }

  for (auto itLista = lres.begin(); itLista != lres.end(); ++itLista) {
    if (!removerPorCaminho(dir, *itLista)) {
      return false;
    }

    if (fs && importacao_diretoria) {
      try {
        fs::remove(Utils::NormalizarCaminho(*itLista));
      } catch (...) {
        return false;
      }
    }
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
  for (auto it = dir->getConteudo().begin(); it != dir->getConteudo().end();
       ++it) {
    Item *item = *it;

    if (Utils::NormalizarCaminho(item->getCaminho()) ==
        Utils::NormalizarCaminho(caminho)) {
      delete item;
      dir->getConteudo().erase(it);
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

  for (auto item : dir->getConteudo()) {
    if (item->getIsFicheiro()) {
      Ficheiro *fich = dynamic_cast<Ficheiro *>(item);
      if (fich) {
        XML->WriteFile(fich->getNome(), fich->getTamanho(), fich->getExtensao(),
                       fich->getDataModificacao());
      }
    } else {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item);
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

  for (Item *item : dir->getConteudo()) {
    // 1. Verifica se é o que procuramos
    if ((!item->getIsFicheiro() == procurarDiretorias) &&
        (item->getNome() == nomeProcurado)) {
      return item;
    }

    if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item);
      Item *res = procurarItemRec(subdir, nomeProcurado, procurarDiretorias);
      if (res)
        return res;
    }
  }

  return nullptr;
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
bool SistemaFicheiros::removerItemPorNome(Diretoria *dir, const string &nome) {
  for (auto item : dir->getConteudo()) {
    if (item->getNome() == nome) {
      dir->getConteudo().remove(item);
      return true;
    }
  }
  return false;
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

  for (auto item : dir->getConteudo()) {
    string novoCaminho = caminhoDir + "/" + item->getNome();
    item->setCaminho(novoCaminho);

    if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item);
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
string *SistemaFicheiros::DataFicheiroRec(Diretoria *dir,
                                          const string &ficheiro) {
  for (auto item : dir->getConteudo()) {
    if (item->getIsFicheiro() && item->getNome() == ficheiro) {
      Ficheiro *f = dynamic_cast<Ficheiro *>(item);
      if (f) {
        return new string(f->getDataModificacao());
      }
    } else if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item);
      if (subdir) {
        string *res = DataFicheiroRec(subdir, ficheiro);
        if (res)
          return res;
      }
    }
  }
  return nullptr;
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
void SistemaFicheiros::ShowRec(Diretoria *dir, int &nTabs, ostream &out) {
  out << Utils::Tabulacao(nTabs) << "<D> " << dir->getNome() << " <"
      << dir->getTamanho() << " bytes>" << endl;

  nTabs++;

  for (auto item : dir->getConteudo())
    if (item->getIsFicheiro())
      out << Utils::Tabulacao(nTabs) << "<F> " << item->getNome() << " <"
          << item->getTamanho() << " bytes>" << endl;
    else {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item);
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
                                           bool importacao_diretoria) {
  int renomeados = 0;

  for (auto item : dir->getConteudo()) {
    if (item->getIsFicheiro()) {
      if (item->getNome() == fich_old) {

        fs::path oldPath(item->getCaminho());
        fs::path parent = oldPath.parent_path();
        fs::path newPath = parent / fich_new;

        try {
          // --- Se importacao_diretoria == 1 > mexe no filesystem ---
          if (importacao_diretoria == 1) {
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
      Diretoria *sub = dynamic_cast<Diretoria *>(item);
      if (sub)
        renomeados +=
            renomearFicheirosRec(sub, fich_old, fich_new, importacao_diretoria);
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
      Diretoria *subdir = dynamic_cast<Diretoria *>(item);
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
  for (auto item : dir->getConteudo()) {
    if (contador[item->getNome()] == 0) {
      contador[item->getNome()] = 1;
    } else {
      // se ja existe, gera um nome com sufixo
      int n = contador[item->getNome()]++;
      if (item->getIsFicheiro()) {
        Ficheiro *f = dynamic_cast<Ficheiro *>(item);
        item->setNome(
            Utils::alterarNomeDuplicado(item->getNome(), n, f->getExtensao()));
      } else {
        item->setNome(Utils::alterarNomeDuplicado(item->getNome(), n, ""));
      }
    }
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
void SistemaFicheiros::copiarItemRec(Diretoria *dirOrigem, Diretoria *destino,
                                     const string &padrao, bool disco,
                                     bool importacao_diretoria) {
  for (auto item : dirOrigem->getConteudo()) {
    if (item->getIsFicheiro()) {
      Ficheiro *f = dynamic_cast<Ficheiro *>(item);
      if (!Utils::contemPalavra(f->getNome(), padrao)) {
        continue; // não corresponde ao padrão
      }

      // 1. Criar objeto em memória
      Ficheiro *copia = new Ficheiro(*f);

      destino->adicionar(copia);

      // 2. Copiar para disco na diretoria destino existente
      if (disco && importacao_diretoria) {
        try {
          fs::copy_file(f->getCaminho(),
                        destino->getCaminho() + "/" +
                            Utils::nomeAteParentese(f->getNome()) +
                            f->getExtensao());
          fs::rename(destino->getCaminho() + "/" +
                         Utils::nomeAteParentese(f->getNome()) +
                         f->getExtensao(),
                     destino->getCaminho() + "/" + f->getNome());
        } catch (const exception &e) {
          ostringstream ss;
          ss << "Erro ao copiar ficheiro " << f->getCaminho() << " para "
             << destino->getCaminho() + "/" + f->getNome() << ": " << e.what();

          Logger::log(Logger::Level::ERROR_, ss.str());
        }
      }
      // Atualizar o caminho do ficheiro copiado
      copia->setCaminho(destino->getCaminho() + "/" + f->getNome());
    } else {
      // Recursão: percorrer subdiretórios
      Diretoria *d = dynamic_cast<Diretoria *>(item);
      copiarItemRec(d, destino, padrao, disco, importacao_diretoria);
    }
  }
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

  if (!fs::exists(path) || !fs::is_directory(path)) {
    cerr << "Erro: caminho inválido -> " << path << endl;
    return false;
  }

  string nome = Utils::extrairNome(path);

  if (nome == path) // se deu erro na função Utils::extrairNome
    return false;

  if (raiz) {
    delete raiz;
    raiz = nullptr;
    Logger::log(
        Logger::Level::INFO,
        "Memória do sistema de ficheiros limpa para novo carregamento.");
  }

  try {
    raiz = new Diretoria(nome, path);

    carregarConteudo(raiz);
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
  return ContarFicheirosRec(raiz);
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
  return ContarDirectoriasRec(raiz);
}

/* Resumo:
 * Método público que retorna a quantidade total de memória (em bytes)
 * ocupada por todos os ficheiros e diretorias carregados no sistema.
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
  // return raiz->getTamanho();
  return memoriaRec(raiz);
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
  int maior;
  return maiorDiretoriaRec(raiz, &maior);
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
  int menor;
  return menorDiretoriaRec(raiz, &menor);
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

  string *strMax = new string("");

  return ficheiroMaiorRec(raiz, tamMax, *strMax);
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

  string *strMax = new string("");

  diretoriaMaisEspaco(raiz, tamMax, *strMax);

  return strMax;
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
  if (Tipo == 1)
    return pesquisarDiretoriaRec(raiz, s);

  if (Tipo == 0)
    return pesquisarFicheiroRec(raiz, s);

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
  return RemovePorNome(raiz, s, tipo, false, importacao_diretoria);
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
  XML XML;
  XML.WriteStartDocument(s);
  escreverXMLRec(raiz, &XML);
  XML.WriteEndDocument();
}

/**
 * Resumo:
 * Importa a estrutura do sistema de ficheiros a partir de um ficheiro XML.
 * Se já existir uma estrutura carregada, esta é apagada antes da importação.
 * Utiliza a classe XML para ler e interpretar o conteúdo do ficheiro XML.
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
  if (raiz != nullptr) {
    delete raiz;
    raiz = nullptr;
  }

  XML xmlParser;
  ifstream *ficheiro = xmlParser.ImportDocument(s);

  if (!ficheiro) {
    Logger::log(Logger::Level::ERROR_, "Erro ao importar documento XML: " + s);
    return false;
  }

  string linha;
  regex regexDiretoriaAberta(
      "<diretoria\\s+nome=\"([^\"]+)\"\\s+tamanho=\"([^\"]+)\">");
  smatch match;

  bool raizEncontrada = false; // Flag de controlo

  while (getline(*ficheiro, linha)) {
    // Remove CR do Windows (Cross-platform fix)
    if (!linha.empty() && linha.back() == '\r') {
      linha.pop_back();
    }

    if (regex_search(linha, match, regexDiretoriaAberta)) {
      string nome = match[1];
      // int tamanho = stoi(match[2]);

      raiz = new Diretoria(nome, "./" + nome);

      xmlParser.ReadDirectory(*ficheiro, raiz);

      raizEncontrada = true;
      break; // Já lemos a árvore toda (recursivamente), podemos sair
    }
  }

  ficheiro->close();
  delete ficheiro; // Importante: liberta a memória do ifstream alocado no
                   // ImportDocument

  // 3. Verificação Final (Correção do Bug)
  if (!raizEncontrada) {
    Logger::log(Logger::Level::ERROR_,
                "XML inválido: Nenhuma diretoria raiz encontrada em " + s);
    return false;
  }

  return true;
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
  Item *item =
      procurarItemRec(raiz, Fich, false); // procurar ficheiro pretendido
  if (!item) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: ficheiro '" + Fich + "' não encontrado.");
    return false;
  }

  Item *dirNovaItem;
  if (DirNova == raiz->getNome()) {
    dirNovaItem = raiz;
  } else {
    dirNovaItem = procurarItemRec(raiz, DirNova,
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
  for (Item *conteudo : dirNova->getConteudo()) {
    if (conteudo->getNome() == item->getNome()) {
      Logger::log(Logger::Level::ERROR_,
                  "Erro: Já existe um item com o nome '" + item->getNome() +
                      "' na diretoria destino.");
      return false; // Aborta a função sem apagar nada
    }
  }

  string nomeDirItem = Utils::NomeDiretoriadoItem(
      item->getCaminho(),
      raiz->getNome()); // localizar diretoria atual do ficheiro

  Diretoria *diretoriaAntiga = nullptr;

  // Se a diretoria antiga for a raiz
  if (nomeDirItem == raiz->getNome()) {
    diretoriaAntiga = raiz;
  } else {
    // procura recursivamente
    Item *dirAntigaItem = procurarItemRec(raiz, nomeDirItem, true);
    diretoriaAntiga = dynamic_cast<Diretoria *>(dirAntigaItem);
  }
  if (!diretoriaAntiga) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: diretoria antiga '" + nomeDirItem + "' não encontrada.");
    return false;
  }

  if (diretoriaAntiga->getCaminho() == dirNova->getCaminho()) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: o ficheiro já se encontra na diretoria '" + DirNova +
                    "'.");
    return false;
  }

  string caminhoAntigo = item->getCaminho(); // guardar caminho antigo
  string novoCaminho =
      dirNova->getCaminho() + "/" + item->getNome(); // construir novo caminho

  if (importacao_diretoria) {
    try {
      fs::rename(caminhoAntigo,
                 novoCaminho); // mover no filesystem ANTES de mudar em memória
    } catch (const fs::filesystem_error &e) {
      Logger::log(Logger::Level::ERROR_,
                  "Erro ao mover ficheiro: " + string(e.what()));
      return false;
    }
  }

  // remover o item da diretoria antiga
  if (!removerItemPorNome(diretoriaAntiga, item->getNome()))
    return false;

  // adicionar o item à nova diretoria
  dirNova->adicionar(item);

  item->setCaminho(novoCaminho); // atualizar caminho APÓS o rename

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

  Item *item = procurarItemRec(raiz, DirOld, true);
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
    dirNovaItem = raiz;
  } else {
    dirNovaItem = procurarItemRec(raiz, DirNew,
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

  // impedir mover para si própria ou para subdiretoria
  if (itemDir->getCaminho() == dirNova->getCaminho() ||
      Utils::contemPalavra(dirNova->getCaminho(),
                           itemDir->getCaminho() + "/")) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: não é possível mover uma diretoria para ela mesma ou "
                "para uma subdiretoria dela.");
    return false;
  }

  string nomeDirItem =
      Utils::NomeDiretoriadoItem(itemDir->getCaminho(), raiz->getNome());

  Diretoria *diretoriaAntiga = nullptr;

  if (nomeDirItem == raiz->getNome()) {
    diretoriaAntiga = raiz;
  } else {
    Item *dirAntigaItem = procurarItemRec(raiz, nomeDirItem, true);
    diretoriaAntiga = dynamic_cast<Diretoria *>(dirAntigaItem);
  }

  if (!diretoriaAntiga) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: diretoria antiga '" + nomeDirItem + "' não encontrada.");
    return false;
  }

  string caminhoAntigo = itemDir->getCaminho();
  string novoCaminho = dirNova->getCaminho() + "/" + itemDir->getNome();

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
  removerItemPorNome(diretoriaAntiga, itemDir->getNome());
  dirNova->adicionar(itemDir);

  // atualizar caminhos internos
  setCaminhoRec(itemDir, novoCaminho);

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
  return DataFicheiroRec(raiz, ficheiro);
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
  int nivel = 0;

  ShowRec(raiz, nivel, cout);

  nivel = 0;

  ofstream f(*fich);

  ShowRec(raiz, nivel, f);

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
  pesquisarItensComNomeIgualRec(raiz, lres, dir, false);
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
  pesquisarItensComNomeIgualRec(raiz, lres, file, true);
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
      renomearFicheirosRec(raiz, fich_old, fich_new, importacao_diretoria);

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
  unordered_set<string> nomes; // guarda nomes únicos~
  return VerificarDuplicadosRec(raiz, nomes);
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
  Item *diretoriaOrigem = procurarItemRec(raiz, DirOrigem, true);
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

  Item *diretoriaDestino = procurarItemRec(raiz, DirDestino, true);
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
  Diretoria *temp = new Diretoria("temp", dirOrigem->getCaminho());

  copiarItemRec(dirOrigem, temp, padrao, false, importacao_diretoria);
  AlterarNomeDuplicado(dirDestino, contador);
  AlterarNomeDuplicado(temp, contador);
  copiarItemRec(temp, dirDestino, padrao, true, importacao_diretoria);

  delete temp;
  return true;
}
