#ifndef SISTEMAFICHEIROS_H
#define SISTEMAFICHEIROS_H

#include "Diretoria.h"
#include "IncludesGerais.h"

class XML; // declaração antecipada

class SistemaFicheiros {
public:
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
  std::string *DirectoriaMaisElementos();
  std::string *DirectoriaMenosElementos();
  std::string *FicheiroMaior();
  std::string *DirectoriaMaisEspaco();
  std::string *Search(const std::string &s, int Tipo);
  bool RemoverAll(const std::string &s, const std::string &tipo);
  bool Escrever_XML(const std::string &s);
  bool Ler_XML(const std::string &s);
  bool MoveFicheiro(const std::string &Fich, const std::string &DirNova);
  bool MoverDirectoria(const std::string &DirOld,
                       const std::string &DirNew);
  std::string *DataFicheiro(const std::string &ficheiro);
  void Tree(const std::string *fich = nullptr);
  void PesquisarAllDirectorias(std::list<std::string> &lres,
                               const std::string &dir);
  void PesquisarAllFicheiros(std::list<std::string> &lres,
                             const std::string &file);
  void RenomearFicheiros(const std::string &fich_old,
                         const std::string &fich_new);
  bool FicheiroDuplicados();
  bool CopyBatch(const std::string &padrao, const std::string &DirOrigem,
                 const std::string &DirDestino);

protected:
private:
  std::unique_ptr<Diretoria> raiz;
  bool importacao_diretoria; // 1-diretoria, 0-xml

  void carregarConteudo(Diretoria *diretoria); // utilizada no Load
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
  std::optional<std::string> pesquisarDiretoriaRec(
      Diretoria *dir, const std::string &s); // utilizada no Search
  std::optional<std::string> pesquisarFicheiroRec(
      Diretoria *dir, const std::string &s); // utilizada no Search
  void pesquisarItensComNomeIgualRec(
      Diretoria *dir, std::list<std::string> &lres, const std::string &n,
      bool procFich); // utilizada no RemoverAll, PesquisarAllFicheiros,
                      // PesquisarAllDirectorias
  bool RemovePorNome(Diretoria *dir, const std::string &s,
                     const std::string &tipo,
                     bool fs,
                     bool operarNoDisco); // utilizada no RemoverAll
  bool removerPorCaminho(Diretoria *dir,
                         const std::string &caminho); // utilizada no RemoverAll
  void escreverXMLRec(Diretoria *dir, XML *XML); // utilizada no Escrever_XML
  Item *procurarItemRec(
      Diretoria *dir, const std::string &nomeProcurado,
      bool procurarDiretorias); // utilizada no MoveFicheiro e MoverDirectoria
  Diretoria *procurarDiretoriaPai(Diretoria *dir, const Item *item);
  bool contemDiretoria(Diretoria *origem, const Diretoria *procurada);
  std::unique_ptr<Item> extrairItemPorNome(
      Diretoria *dir,
      const std::string &nome); // utilizada no MoveFicheiro e MoverDirectoria
  void setCaminhoRec(Diretoria *dir,
                     std::string &caminhoDir); // utilizada no MoverDirectoria
  std::optional<std::string> DataFicheiroRec(
      Diretoria *dir, const std::string &ficheiro); // utilizada no DataFicheiro
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
