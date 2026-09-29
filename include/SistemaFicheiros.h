#ifndef SISTEMAFICHEIROS_H
#define SISTEMAFICHEIROS_H

#include "Diretoria.h"
#include "IncludesGerais.h"

class XML; // declaração antecipada

class SistemaFicheiros {
public:
  enum class TipoItem { Ficheiro, Diretoria };

  SistemaFicheiros();
  virtual ~SistemaFicheiros() = default;

  SistemaFicheiros(const SistemaFicheiros &) = delete;
  SistemaFicheiros &operator=(const SistemaFicheiros &) = delete;
  SistemaFicheiros(SistemaFicheiros &&) noexcept = default;
  SistemaFicheiros &operator=(SistemaFicheiros &&) noexcept = default;

  bool Load(const std::string &path);
  int ContarFicheiros();
  int ContarDirectorias();
  int Memoria();
  std::optional<std::string> DirectoriaMaisElementos();
  std::optional<std::string> DirectoriaMenosElementos();
  std::optional<std::string> FicheiroMaior();
  std::optional<std::string> DirectoriaMaisEspaco();
  std::optional<std::string> Search(const std::string &s, TipoItem tipo);
  bool RemoverAll(const std::string &s, TipoItem tipo);
  bool Escrever_XML(const std::string &s);
  bool Ler_XML(const std::string &s);
  bool MoveFicheiro(const std::string &Fich, const std::string &DirNova);
  bool MoverDirectoria(const std::string &DirOld,
                       const std::string &DirNew);
  std::optional<std::string> DataFicheiro(const std::string &ficheiro);
  void Tree(const std::string &ficheiro = "tree.txt");
  std::list<std::string> PesquisarAllDirectorias(const std::string &dir);
  std::list<std::string> PesquisarAllFicheiros(const std::string &file);
  void RenomearFicheiros(const std::string &fich_old,
                         const std::string &fich_new);
  bool FicheiroDuplicados();
  bool CopyBatch(const std::string &padrao, const std::string &DirOrigem,
                 const std::string &DirDestino);

protected:
private:
  std::unique_ptr<Diretoria> raiz;
  bool importacao_diretoria; // 1-diretoria, 0-xml

  void carregarConteudo(
      Diretoria *diretoria,
      std::unordered_set<std::string> &diretoriasVisitadas); // utilizada no Load
  int ContarFicheirosRec(Diretoria *dir);      // utilizada no ContarFicheiros
  int ContarDirectoriasRec(Diretoria *dir);    // utilizada no ContarDirectorias
  void ficheiroMaiorRec(Diretoria *dir, std::uintmax_t &tamMax,
                        std::string &strMax); // utilizada no FicheiroMaior
  std::uintmax_t memoriaRec(Diretoria *dir); // utilizada no Memoria
  std::string maiorDiretoriaRec(
      Diretoria *dir,
      std::size_t &maior); // utilizada no DirectoriaMaisElementos
  std::string menorDiretoriaRec(
      Diretoria *dir,
      std::size_t &menor); // utilizada no DirectoriaMenosElementos
  void diretoriaMaisEspaco(
      Diretoria *dir, std::uintmax_t &tamMax,
      std::string &strMax); // utilizada no DirectoriaMaisEspaco
  Item *resolverCaminho(const std::string &caminho);
  void pesquisarItensComNomeIgualRec(
      Diretoria *dir, std::list<std::string> &lres, const std::string &n,
      TipoItem tipo); // utilizada no RemoverAll, PesquisarAllFicheiros,
                      // PesquisarAllDirectorias
  bool RemovePorNome(Diretoria *dir, const std::string &s, TipoItem tipo,
                     bool removerNoDisco,
                     bool operarNoDisco); // utilizada no RemoverAll
  bool removerPorCaminho(Diretoria *dir,
                         const std::string &caminho); // utilizada no RemoverAll
  void escreverXMLRec(Diretoria *dir, XML *XML); // utilizada no Escrever_XML
  Item *procurarItemRec(
      Diretoria *dir, const std::string &nomeProcurado,
      TipoItem tipo); // utilizada no CopyBatch
  Diretoria *procurarDiretoriaPai(Diretoria *dir, const Item *item);
  bool contemDiretoria(Diretoria *origem, const Diretoria *procurada);
  void setCaminhoRec(Diretoria *dir,
                     std::string &caminhoDir); // utilizada no MoverDirectoria
  void ShowRec(Diretoria *dir, std::size_t &nTabs,
               std::ostream &out); // utilizada no Tree
  bool VerificarDuplicadosRec(
      Diretoria *dir,
      std::unordered_set<std::string>
          &nomes); // utilizada no FicheiroDuplicados
  void AlterarNomeDuplicado(
      Diretoria *dir,
      std::unordered_map<std::string, int>
          &contador); // utilizada no CopyBatch
  bool copiarItemRec(Diretoria *dirOrigem, Diretoria *destino,
                     const std::string &padrao, bool disco,
                     bool operarNoDisco); // utilizada no CopyBatch
};

#endif // SISTEMAFICHEIROS_H
