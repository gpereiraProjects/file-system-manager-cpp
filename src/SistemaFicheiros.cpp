#include "SistemaFicheiros.h"
#include "Ficheiro.h"
#include "Logger.h"
#include "Utils.h"
#include "XML.h"

#include <cctype>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

using namespace std;
namespace fs = std::filesystem;

namespace {

bool linkSimbolicoOuReparsePoint(const fs::directory_entry &entry,
                                 error_code &error) {
#ifdef _WIN32
  const DWORD attributes = GetFileAttributesW(entry.path().c_str());
  if (attributes == INVALID_FILE_ATTRIBUTES) {
    error = error_code(static_cast<int>(GetLastError()), system_category());
    return false;
  }

  error.clear();
  return (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
  const fs::file_status linkStatus = entry.symlink_status(error);
  return !error && fs::is_symlink(linkStatus);
#endif
}

string identidadeDiretoria(const fs::path &path) {
  error_code error;
  fs::path canonicalPath = fs::canonical(path, error);
  if (error)
    throw fs::filesystem_error("Não foi possível identificar a diretoria",
                               path, error);

  string identity = canonicalPath.lexically_normal().generic_string();
#ifdef _WIN32
  transform(identity.begin(), identity.end(), identity.begin(),
            [](const char character) {
              return static_cast<char>(
                  tolower(static_cast<unsigned char>(character)));
            });
#endif
  return identity;
}

} // namespace

//==================== Construtor e Destrutor ====================
SistemaFicheiros::SistemaFicheiros()
    : raiz(nullptr), importacao_diretoria(false) {}

//==================== Métodos Privados ====================
/**
 * Resumo:
 * Percorre recursivamente o sistema de ficheiros a partir do caminho da
 * diretoria fornecida. Para cada entrada encontrada (ficheiro ou pasta), cria o
 * respetivo objeto ('Ficheiro' ou 'Diretoria') e adiciona-o à estrutura em
 * memória. Links simbólicos são ignorados e a identidade canónica de cada
 * diretoria é registada para impedir ciclos através de aliases do sistema de
 * ficheiros. As exceções são propagadas para `Load`, que só substitui a árvore
 * ativa depois de concluir todo o carregamento.
 *
 * Parâmetros:
 * - diretoria (Diretoria*): Objeto que será preenchido com o conteúdo.
 * - diretoriasVisitadas: Identidades canónicas das diretorias já percorridas.
 *
 * Retorno:
 * - void (Não retorna valor).
 */
void SistemaFicheiros::carregarConteudo(
    Diretoria *diretoria, unordered_set<string> &diretoriasVisitadas) {
  for (const auto &entry : fs::directory_iterator(diretoria->getCaminho())) {
    string nome = entry.path().filename().string();
    string caminhoCompleto = entry.path().string();

    error_code statusError;
    const bool ignorarLink =
        linkSimbolicoOuReparsePoint(entry, statusError);
    if (statusError)
      throw fs::filesystem_error("Não foi possível consultar uma entrada",
                                 entry.path(), statusError);

    if (ignorarLink) {
      Logger::log(Logger::Level::INFO,
                  "Link simbólico ou reparse point ignorado durante o "
                  "carregamento: " +
                      caminhoCompleto);
      continue;
    }

    const fs::file_status targetStatus = entry.status(statusError);
    if (statusError)
      throw fs::filesystem_error("Não foi possível consultar uma entrada",
                                 entry.path(), statusError);

    if (fs::is_directory(targetStatus)) {
      const string identity = identidadeDiretoria(entry.path());
      if (!diretoriasVisitadas.insert(identity).second) {
        Logger::log(Logger::Level::INFO,
                    "Diretoria já visitada ignorada durante o carregamento: " +
                        caminhoCompleto);
        continue;
      }

      auto sub = make_unique<Diretoria>(nome, caminhoCompleto);
      carregarConteudo(sub.get(), diretoriasVisitadas);
      diretoria->adicionar(std::move(sub));
    } else if (fs::is_regular_file(targetStatus)) {
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
 * - size_t: O número total de ficheiros. Retorna 0 se a diretoria for nula.
 */
size_t SistemaFicheiros::ContarFicheirosRec(const Diretoria *dir) const {
  if (dir == nullptr)
    return 0;
  size_t total = 0;
  for (const auto &item : dir->getConteudo()) {
    if (item->getIsFicheiro()) {
      total++;
    } else {
      const auto *subdir = dynamic_cast<const Diretoria *>(item.get());
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
 * - size_t: O número total de diretorias (incluindo a própria raiz).
 * Retorna 0 se o ponteiro `dir` for nulo.
 */
size_t SistemaFicheiros::ContarDirectoriasRec(const Diretoria *dir) const {
  if (dir == nullptr)
    return 0;
  size_t total = 1; // conta a diretoria atual
  for (const auto &item : dir->getConteudo()) {
    if (!item->getIsFicheiro()) {
      const auto *subdir = dynamic_cast<const Diretoria *>(item.get());
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
 * - void: Atualiza `tamMax` e `strMax` quando encontra um ficheiro maior.
 */
void SistemaFicheiros::ficheiroMaiorRec(const Diretoria *dir,
                                        uintmax_t &tamMax,
                                        string &strMax) const {
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
      const auto *diretoria = dynamic_cast<const Diretoria *>(item.get());
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
 * - uintmax_t: A quantidade total de bytes ocupada pelos ficheiros.
 * Retorna 0 se a diretoria for nula.
 */
uintmax_t SistemaFicheiros::memoriaRec(const Diretoria *dir) const {
  if (!dir)
    return 0;

  uintmax_t total = 0;

  for (const auto &item : dir->getConteudo()) {
    if (item->getIsFicheiro()) {
      total += item->getTamanho();
    } else {
      const auto *subdir = dynamic_cast<const Diretoria *>(item.get());
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
 * - string: Nome da diretoria com mais itens.
 */
string SistemaFicheiros::maiorDiretoriaRec(const Diretoria *dir,
                                           size_t &maior) const {
  size_t localMax = dir->getNItens();
  string localName = dir->getNome();

  for (const auto &item : dir->getConteudo()) {
    if (!item->getIsFicheiro()) {
      const auto *subdir = dynamic_cast<const Diretoria *>(item.get());
      size_t nItem = subdir->getNItens();
      if (nItem > localMax) {
        localMax = nItem;
        localName = subdir->getNome();
      }

      size_t subMax = 0;
      string nomeSub = maiorDiretoriaRec(subdir, subMax);

      if (subMax > localMax) {
        localMax = subMax;
        localName = std::move(nomeSub);
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
 * - string: Nome da diretoria com menos itens.
 */
string SistemaFicheiros::menorDiretoriaRec(const Diretoria *dir,
                                           size_t &menor) const {
  size_t localMin = dir->getNItens();
  string localName = dir->getNome();

  for (const auto &item : dir->getConteudo()) {
    if (!item->getIsFicheiro()) {
      const auto *subdir = dynamic_cast<const Diretoria *>(item.get());
      size_t nItem = subdir->getNItens();
      if (nItem < localMin) {
        localMin = nItem;
        localName = subdir->getNome();
      }

      size_t subMin = numeric_limits<size_t>::max();
      string nomeSub = menorDiretoriaRec(subdir, subMin);

      if (subMin < localMin) {
        localMin = subMin;
        localName = std::move(nomeSub);
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
 * - void: Atualiza `tamMax` e `strMax` quando encontra uma diretoria maior.
 */
void SistemaFicheiros::diretoriaMaisEspaco(const Diretoria *dir,
                                           uintmax_t &tamMax,
                                           string &strMax) const {
  for (const auto &c : dir->getConteudo()) {
    if (!c->getIsFicheiro()) {
      if (c->getTamanho() > tamMax) {
        tamMax = c->getTamanho();
        strMax = c->getCaminho();
      }
      if (const auto *subdiretoria =
              dynamic_cast<const Diretoria *>(c.get()))
        diretoriaMaisEspaco(subdiretoria, tamMax, strMax);
    }
  }
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
 * - tipo: Tipo de item a procurar.
 *
 * Retorno:
 * - void (Não retorna valor, os resultados são armazenados em 'lres').
 */
void SistemaFicheiros::pesquisarItensComNomeIgualRec(
    const Diretoria *dir, list<string> &lres, const string &n,
    TipoItem tipo) const {
  const bool procurarFicheiros = tipo == TipoItem::Ficheiro;
  if (dir->getNome() == n && !procurarFicheiros)
    lres.push_back(dir->getCaminho());

  for (const auto &item : dir->getConteudo()) {
    if (!item->getIsFicheiro()) {
      const auto *subdir = dynamic_cast<const Diretoria *>(item.get());
      if (subdir)
        pesquisarItensComNomeIgualRec(subdir, lres, n, tipo);
    } else if (item->getNome() == n && procurarFicheiros) {
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
 * - tipo: Tipo de item a remover.
 * - removerNoDisco: Indica se o item deve ser removido do sistema de ficheiros.
 * - importacao_diretoria (bool): Indica se a estrutura foi carregada a partir
 * de uma diretoria (true) ou de um XML (false).
 *
 * Retorno:
 * - bool: Verdadeiro se pelo menos um item foi removido, falso caso contrário.
 */
bool SistemaFicheiros::RemovePorNome(Diretoria *dir, const string &s,
                                     TipoItem tipo, bool removerNoDisco,
                                     bool operarNoDisco) {
  const bool removerFicheiros = tipo == TipoItem::Ficheiro;

  list<string> lres;
  pesquisarItensComNomeIgualRec(dir, lres, s, tipo);

  if (!removerFicheiros) {
    const string caminhoRaiz = Utils::NormalizarCaminho(dir->getCaminho());
    lres.remove_if([&caminhoRaiz](const string &caminho) {
      return Utils::NormalizarCaminho(caminho) == caminhoRaiz;
    });
  }

  if (lres.empty()) {
    return false;
  }

  if (!removerFicheiros)
    lres.sort([](const string &left, const string &right) {
      return fs::path(left).lexically_normal().native().size() >
             fs::path(right).lexically_normal().native().size();
    });

  for (const string &caminho : lres) {
    if (removerNoDisco && operarNoDisco) {
      error_code error;
      bool removidoNoDisco = false;
      if (removerFicheiros)
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
void SistemaFicheiros::escreverXMLRec(const Diretoria *dir, XML *XML) const {
  XML->WriteStartDirectory(dir->getNome(), dir->getTamanho());

  for (const auto &item : dir->getConteudo()) {
    if (item->getIsFicheiro()) {
      const auto *fich = dynamic_cast<const Ficheiro *>(item.get());
      if (fich) {
        XML->WriteFile(fich->getNome(), fich->getTamanho(), fich->getExtensao(),
                       fich->getDataModificacao());
      }
    } else {
      const auto *subdir = dynamic_cast<const Diretoria *>(item.get());
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
 * especificado dentro da diretoria fornecida e do tipo pedido.
 *
 * Parâmetros:
 * - dir (Diretoria*): Ponteiro para a diretoria onde a busca inicia.
 * - nomeProcurado (const string&): O nome do ficheiro ou pasta que se pretende
 * encontrar.
 * - tipo: Tipo de item a procurar.
 *
 * Retorno:
 * - Item*: Ponteiro para o item encontrado (ficheiro ou diretoria), ou nullptr
 * se não encontrado.
 */
Item *SistemaFicheiros::procurarItemRec(Diretoria *dir,
                                        const string &nomeProcurado,
                                        TipoItem tipo) {
  if (!dir)
    return nullptr;

  for (const auto &item : dir->getConteudo()) {
    // 1. Verifica se é o que procuramos
    const bool tipoCorreto =
        tipo == TipoItem::Ficheiro ? item->getIsFicheiro()
                                   : !item->getIsFicheiro();
    if (tipoCorreto && item->getNome() == nomeProcurado) {
      return item.get();
    }

    if (!item->getIsFicheiro()) {
      Diretoria *subdir = dynamic_cast<Diretoria *>(item.get());
      Item *res = procurarItemRec(subdir, nomeProcurado, tipo);
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
 * Resolve um caminho absoluto ou relativo à raiz carregada. O caminho "."
 * representa a própria raiz. Componentes que escapem da raiz são sempre
 * rejeitados.
 */
Item *SistemaFicheiros::resolverCaminho(const string &caminho) {
  return const_cast<Item *>(
      static_cast<const SistemaFicheiros *>(this)->resolverCaminho(caminho));
}

const Item *SistemaFicheiros::resolverCaminho(const string &caminho) const {
  if (!raiz || caminho.empty())
    return nullptr;

  fs::path pedido = fs::path(caminho).lexically_normal();
  fs::path relativo;
  if (pedido.is_absolute()) {
    error_code pedidoError;
    error_code raizError;
    const fs::path pedidoCanonico =
        fs::weakly_canonical(pedido, pedidoError);
    const fs::path caminhoRaizCanonico = fs::weakly_canonical(
        fs::path(raiz->getCaminho()), raizError);
    if (pedidoError || raizError)
      return nullptr;

    relativo = pedidoCanonico.lexically_relative(caminhoRaizCanonico);
    if (relativo.empty())
      return nullptr;
  } else {
    relativo = pedido;
  }

  vector<string> componentes;
  for (const fs::path &componente : relativo) {
    const string valor = componente.string();
    if (valor.empty() || valor == ".")
      continue;
    if (valor == "..")
      return nullptr;
    componentes.push_back(valor);
  }

  const Item *atual = raiz.get();
  for (const string &componente : componentes) {
    const auto *diretoriaAtual = dynamic_cast<const Diretoria *>(atual);
    if (!diretoriaAtual)
      return nullptr;

    const Item *seguinte = nullptr;
    for (const auto &item : diretoriaAtual->getConteudo()) {
      if (item->getNome() == componente) {
        seguinte = item.get();
        break;
      }
    }
    if (!seguinte)
      return nullptr;
    atual = seguinte;
  }

  return atual;
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
    string novoCaminho =
        (fs::path(caminhoDir) / item->getNome()).string();
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
void SistemaFicheiros::ShowRec(const Diretoria *dir, size_t &nTabs,
                               ostream &out) const {
  out << Utils::Tabulacao(nTabs) << "<D> " << dir->getNome() << " <"
      << dir->getTamanho() << " bytes>" << endl;

  nTabs++;

  for (const auto &item : dir->getConteudo())
    if (item->getIsFicheiro())
      out << Utils::Tabulacao(nTabs) << "<F> " << item->getNome() << " <"
          << item->getTamanho() << " bytes>" << endl;
    else {
      const auto *subdir = dynamic_cast<const Diretoria *>(item.get());
      if (subdir)
        ShowRec(subdir, nTabs, out);
    }

  nTabs--;
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
bool SistemaFicheiros::VerificarDuplicadosRec(
    const Diretoria *dir, unordered_set<string> &nomes) const {
  for (const auto &item : dir->getConteudo()) {
    if (!item->getIsFicheiro()) {
      const auto *subdir = dynamic_cast<const Diretoria *>(item.get());
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
             << (fs::path(destino->getCaminho()) / f->getNome()).string()
             << ": " << e.what();

          Logger::log(Logger::Level::ERROR_, ss.str());
          sucesso = false;
          continue;
        }
      }

      copia->setCaminho(caminhoDestino.string());
      destino->adicionar(std::move(copia));
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
    Logger::log(Logger::Level::ERROR_, "Caminho inválido: " + path);
    return false;
  }

  const string nome = caminho.filename().string();
  if (nome.empty())
    return false;

  try {
    auto novaRaiz = make_unique<Diretoria>(nome, caminho.string());
    unordered_set<string> diretoriasVisitadas;
    diretoriasVisitadas.insert(identidadeDiretoria(caminho));
    carregarConteudo(novaRaiz.get(), diretoriasVisitadas);
    novaRaiz->recalcularTamanho();
    raiz = std::move(novaRaiz);
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
 * - size_t: O número total de ficheiros encontrados. Retorna 0 se o sistema
 * não tiver sido carregado (raiz nula).
 */
size_t SistemaFicheiros::ContarFicheiros() const {
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
 * - size_t: O número total de diretorias encontradas. Retorna 0 se o sistema
 * não tiver sido carregado (raiz nula).
 */
size_t SistemaFicheiros::ContarDirectorias() const {
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
 * - uintmax_t: A quantidade total de bytes. Retorna 0 se o sistema
 * não tiver sido carregado (raiz nula).
 */
uintmax_t SistemaFicheiros::Memoria() const { return memoriaRec(raiz.get()); }

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
 * - optional<string>: Nome da diretoria, ou vazio se não existir árvore.
 */
optional<string> SistemaFicheiros::DirectoriaMaisElementos() const {
  if (!raiz)
    return nullopt;

  size_t maior = 0;
  return maiorDiretoriaRec(raiz.get(), maior);
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
 * - optional<string>: Nome da diretoria, ou vazio se não existir árvore.
 */
optional<string> SistemaFicheiros::DirectoriaMenosElementos() const {
  if (!raiz)
    return nullopt;

  size_t menor = 0;
  return menorDiretoriaRec(raiz.get(), menor);
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
 * - optional<string>: Caminho do ficheiro, ou vazio se não existir ficheiro.
 */
optional<string> SistemaFicheiros::FicheiroMaior() const {
  if (raiz == nullptr)
    return nullopt;

  uintmax_t tamMax = 0;

  string strMax;
  ficheiroMaiorRec(raiz.get(), tamMax, strMax);
  if (strMax.empty())
    return nullopt;
  return strMax;
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
 * - optional<string>: Caminho da diretoria, ou vazio se não houver resultado.
 */
optional<string> SistemaFicheiros::DirectoriaMaisEspaco() const {
  if (raiz == nullptr)
    return nullopt;

  uintmax_t tamMax = 0;

  string strMax;
  diretoriaMaisEspaco(raiz.get(), tamMax, strMax);
  if (strMax.empty())
    return nullopt;
  return strMax;
}

/**
 * Resumo:
 * Resolve um ficheiro ou diretoria através de um caminho inequívoco, absoluto
 * ou relativo à raiz carregada.
 *
 * Parâmetros:
 * - const string &s: Caminho a resolver.
 * - tipo: Tipo de item a pesquisar.
 *
 * Retorno:
 * - optional<string>: Caminho do item, ou vazio se não for encontrado.
 */
optional<string> SistemaFicheiros::Search(const string &s,
                                          TipoItem tipo) const {
  if (!raiz)
    return nullopt;

  const Item *item = resolverCaminho(s);
  if (!item)
    return nullopt;
  const bool tipoCorreto = tipo == TipoItem::Ficheiro
                               ? item->getIsFicheiro()
                               : !item->getIsFicheiro();
  if (!tipoCorreto)
    return nullopt;
  return item->getCaminho();
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
 * - tipo: O tipo de item a remover.
 *
 * Retorno:
 * - bool: Retorna 'true' se a operação de remoção foi bem-sucedida (ou se itens
 * foram encontrados e removidos), ou 'false' caso contrário.
 */
bool SistemaFicheiros::RemoverAll(const string &s, TipoItem tipo) {
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
 * - bool: Verdadeiro quando o documento completo é publicado com sucesso.
 * Em caso de falha, preserva o ficheiro anterior sempre que este exista.
 */
bool SistemaFicheiros::Escrever_XML(const string &s) const {
  if (!raiz) {
    Logger::log(Logger::Level::ERROR_,
                "Não é possível exportar um sistema não carregado.");
    return false;
  }

  const fs::path destination(s);
  const string uniqueSuffix = to_string(
      chrono::high_resolution_clock::now().time_since_epoch().count());
  fs::path temporary = destination;
  temporary += ".tmp-" + uniqueSuffix;
  fs::path backup = destination;
  backup += ".bak-" + uniqueSuffix;
  bool originalMoved = false;

  try {
    if (fs::exists(destination) && fs::is_directory(destination))
      throw runtime_error("O destino indicado é uma diretoria.");

    XML xmlWriter;
    xmlWriter.WriteStartDocument(temporary.string());
    escreverXMLRec(raiz.get(), &xmlWriter);
    xmlWriter.WriteEndDocument();

    if (fs::exists(destination)) {
      fs::rename(destination, backup);
      originalMoved = true;
    }
    fs::rename(temporary, destination);
    if (originalMoved) {
      error_code backupError;
      fs::remove(backup, backupError);
      if (backupError) {
        Logger::log(Logger::Level::WARNING,
                    "O XML foi exportado, mas não foi possível remover a "
                    "cópia de segurança: " + backupError.message());
      }
    }
    return true;
  } catch (const exception &e) {
    error_code cleanupError;
    fs::remove(temporary, cleanupError);

    if (originalMoved && !fs::exists(destination)) {
      error_code rollbackError;
      fs::rename(backup, destination, rollbackError);
      if (rollbackError) {
        Logger::log(Logger::Level::ERROR_,
                    "Falha ao restaurar o XML anterior: " +
                        rollbackError.message());
      }
    }

    Logger::log(Logger::Level::ERROR_,
                "Erro ao exportar XML '" + s + "': " + e.what());
    return false;
  }
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

  try {
    auto novaRaiz = xmlParser.ReadDocument(ficheiro);
    raiz = std::move(novaRaiz);
    importacao_diretoria = false;
    return true;
  } catch (const exception &e) {
    Logger::log(Logger::Level::ERROR_,
                "Erro ao interpretar XML '" + s + "': " + e.what());
    return false;
  }

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
 * - Fich (const string&): Caminho do ficheiro que se pretende mover.
 * - DirNova (const string&): Caminho da diretoria de destino.
 *
 * Retorno:
 * - bool: Retorna 'true' se o movimento for bem-sucedido. Retorna 'false' e
 * regista erro no Logger caso falhe a validação ou ocorra erro de I/O.
 */
bool SistemaFicheiros::MoveFicheiro(const string &Fich, const string &DirNova) {
  if (!raiz)
    return false;

  Item *item = resolverCaminho(Fich);
  if (!item || !item->getIsFicheiro()) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: ficheiro '" + Fich + "' não encontrado.");
    return false;
  }

  auto *dirNova = dynamic_cast<Diretoria *>(resolverCaminho(DirNova));
  if (!dirNova) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: diretoria '" + DirNova + "' não encontrada.");
    return false;
  }

  // Verificação de duplicados na diretoria destino
  for (const auto &conteudo : dirNova->getConteudo()) {
    if (Utils::nomesItemEquivalentes(conteudo->getNome(), item->getNome())) {
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
  auto itemMovido = diretoriaAntiga->extrair(item);
  if (!itemMovido) {
    if (importacao_diretoria) {
      error_code rollbackError;
      fs::rename(novoCaminho, caminhoAntigo, rollbackError);
    }
    return false;
  }

  // adicionar o item à nova diretoria
  Item *itemMovidoPtr = itemMovido.get();
  dirNova->adicionar(std::move(itemMovido));

  itemMovidoPtr->setCaminho(novoCaminho.string());
  raiz->recalcularTamanho();

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
 * - DirOld (const string&): Caminho da diretoria de origem.
 * - DirNew (const string&): Caminho da diretoria de destino.
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

  auto *itemDir = dynamic_cast<Diretoria *>(resolverCaminho(DirOld));
  if (!itemDir || itemDir == raiz.get()) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: diretoria '" + DirOld + "' não encontrada.");
    return false;
  }

  auto *dirNova = dynamic_cast<Diretoria *>(resolverCaminho(DirNew));
  if (!dirNova) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: diretoria '" + DirNew + "' não encontrada.");
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
    if (Utils::nomesItemEquivalentes(conteudo->getNome(),
                                     itemDir->getNome())) {
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
  auto diretoriaMovida = diretoriaAntiga->extrair(itemDir);
  if (!diretoriaMovida) {
    if (importacao_diretoria) {
      error_code rollbackError;
      fs::rename(novoCaminho, caminhoAntigo, rollbackError);
    }
    return false;
  }
  Diretoria *diretoriaMovidaPtr =
      dynamic_cast<Diretoria *>(diretoriaMovida.get());
  dirNova->adicionar(std::move(diretoriaMovida));

  // atualizar caminhos internos
  string novoCaminhoString = novoCaminho.string();
  setCaminhoRec(diretoriaMovidaPtr, novoCaminhoString);
  raiz->recalcularTamanho();

  return true;
}

/**
 * Resumo:
 * Obtém a data de modificação de um ficheiro identificado pelo seu caminho.
 *
 * Parâmetros:
 * - ficheiro (const string&): Caminho absoluto ou relativo à raiz.
 *
 * Retorno:
 * - optional<string>: Data do ficheiro, ou vazio se não for encontrado.
 */
optional<string> SistemaFicheiros::DataFicheiro(const string &ficheiro) const {
  if (!raiz)
    return nullopt;

  const auto *item =
      dynamic_cast<const Ficheiro *>(resolverCaminho(ficheiro));
  if (!item)
    return nullopt;
  return item->getDataModificacao();
}

/**
 * Resumo:
 * Devolve a representação textual da árvore sem produzir saída de consola.
 */
string SistemaFicheiros::Tree() const {
  if (!raiz)
    return {};

  size_t nivel = 0;
  ostringstream output;
  ShowRec(raiz.get(), nivel, output);
  return output.str();
}

bool SistemaFicheiros::EscreverArvore(const string &ficheiro) const {
  if (!raiz)
    return false;
  ofstream f(ficheiro);
  if (!f)
    return false;
  f << Tree();
  return static_cast<bool>(f);
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
 * - list<string>: Caminhos de todas as diretorias encontradas.
 */
list<string>
SistemaFicheiros::PesquisarAllDirectorias(const string &dir) const {
  list<string> resultados;
  if (!raiz)
    return resultados;
  pesquisarItensComNomeIgualRec(raiz.get(), resultados, dir,
                                 TipoItem::Diretoria);
  return resultados;
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
 * - list<string>: Caminhos de todos os ficheiros encontrados.
 */
list<string> SistemaFicheiros::PesquisarAllFicheiros(const string &file) const {
  list<string> resultados;
  if (!raiz)
    return resultados;
  pesquisarItensComNomeIgualRec(raiz.get(), resultados, file,
                                 TipoItem::Ficheiro);
  return resultados;
}

/**
 * Resumo:
 * Renomeia em massa os ficheiros com o nome indicado. Antes de alterar o disco,
 * valida o novo nome e todos os destinos. Se uma alteração física falhar, tenta
 * repor as anteriores; o modelo interno só é atualizado depois de todas as
 * alterações físicas concluírem com sucesso.
 *
 * Parâmetros:
 * - fich_old (const string&): O nome atual do ficheiro a ser substituído.
 * - fich_new (const string&): O novo nome que será atribuído.
 *
 * Retorno:
 * - ResultadoRenomeacao: Estado final e quantidade de ficheiros alterados.
 */
SistemaFicheiros::ResultadoRenomeacao
SistemaFicheiros::RenomearFicheiros(const string &fich_old,
                                    const string &fich_new) {
  if (!raiz) {
    Logger::log(Logger::Level::ERROR_, "Erro: sistema não carregado.");
    return {EstadoRenomeacao::SistemaNaoCarregado, 0};
  }

  if (!Utils::nomeItemPortatilValido(fich_new)) {
    Logger::log(Logger::Level::ERROR_,
                "Erro: o novo nome não é um nome de ficheiro portátil válido.");
    return {EstadoRenomeacao::NomeInvalido, 0};
  }

  if (fich_old == fich_new)
    return {EstadoRenomeacao::SemAlteracoes, 0};

  struct RenameTarget {
    Diretoria *parent;
    Ficheiro *file;
    fs::path oldPath;
    fs::path newPath;
  };

  vector<RenameTarget> targets;
  const auto collectTargets =
      [&targets, &fich_old,
       &fich_new](auto &&self, Diretoria *directory) -> void {
    for (const auto &item : directory->getConteudo()) {
      if (item->getIsFicheiro()) {
        if (item->getNome() == fich_old) {
          auto *file = dynamic_cast<Ficheiro *>(item.get());
          if (file) {
            const fs::path oldPath(file->getCaminho());
            targets.push_back(
                {directory, file, oldPath, oldPath.parent_path() / fich_new});
          }
        }
      } else if (auto *subdirectory =
                     dynamic_cast<Diretoria *>(item.get())) {
        self(self, subdirectory);
      }
    }
  };
  collectTargets(collectTargets, raiz.get());

  if (targets.empty())
    return {EstadoRenomeacao::NaoEncontrado, 0};

  for (const RenameTarget &target : targets) {
    const bool modelCollision = any_of(
        target.parent->getConteudo().begin(),
        target.parent->getConteudo().end(),
        [&target, &fich_new](const unique_ptr<Item> &item) {
          return item.get() != target.file &&
                 Utils::nomesItemEquivalentes(item->getNome(), fich_new);
        });
    if (modelCollision) {
      Logger::log(Logger::Level::ERROR_,
                  "Erro: o destino já contém um item chamado '" + fich_new +
                      "'.");
      return {EstadoRenomeacao::Colisao, 0};
    }

    if (importacao_diretoria) {
      error_code existsError;
      const bool destinationExists = fs::exists(target.newPath, existsError);
      if (existsError) {
        Logger::log(Logger::Level::ERROR_,
                    "Erro ao validar o destino da renomeação: " +
                        existsError.message());
        return {EstadoRenomeacao::ErroSistemaFicheiros, 0};
      }

      if (destinationExists) {
        error_code equivalentError;
        const bool sameFile =
            fs::equivalent(target.oldPath, target.newPath, equivalentError);
        if (equivalentError || !sameFile) {
          Logger::log(Logger::Level::ERROR_,
                      "Erro: o caminho de destino já existe: " +
                          target.newPath.string());
          return {EstadoRenomeacao::Colisao, 0};
        }
      }
    }
  }

  size_t renamedOnDisk = 0;
  if (importacao_diretoria) {
    for (const RenameTarget &target : targets) {
      try {
        fs::rename(target.oldPath, target.newPath);
        ++renamedOnDisk;
      } catch (const fs::filesystem_error &error) {
        Logger::log(Logger::Level::ERROR_,
                    "Erro ao renomear ficheiro: " + string(error.what()));

        for (size_t index = renamedOnDisk; index > 0; --index) {
          const RenameTarget &completed = targets[index - 1];
          error_code rollbackError;
          fs::rename(completed.newPath, completed.oldPath, rollbackError);
          if (rollbackError) {
            Logger::log(Logger::Level::ERROR_,
                        "Falha no rollback da renomeação de '" +
                            completed.newPath.string() + "': " +
                            rollbackError.message());
          }
        }

        return {EstadoRenomeacao::ErroSistemaFicheiros, 0};
      }
    }
  }

  for (RenameTarget &target : targets) {
    target.file->setNome(fich_new);
    target.file->setCaminho(target.newPath.string());
  }

  return {EstadoRenomeacao::Sucesso, targets.size()};
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
bool SistemaFicheiros::FicheiroDuplicados() const {
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
  Item *diretoriaOrigem =
      procurarItemRec(raiz.get(), DirOrigem, TipoItem::Diretoria);
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

  Item *diretoriaDestino =
      procurarItemRec(raiz.get(), DirDestino, TipoItem::Diretoria);
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
