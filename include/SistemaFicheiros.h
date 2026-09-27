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

  bool Load(const string &path);
  int ContarFicheiros();
  int ContarDirectorias();
  int Memoria();
  string *DirectoriaMaisElementos();
  string *DirectoriaMenosElementos();
  string *FicheiroMaior();
  string *DirectoriaMaisEspaco();
  string *Search(const string &s, int Tipo);
  bool RemoverAll(const string &s, const string &tipo);
  void Escrever_XML(const string &s);
  bool Ler_XML(const string &s);
  bool MoveFicheiro(const string &Fich, const string &DirNova);
  bool MoverDirectoria(const string &DirOld, const string &DirNew);
  string *DataFicheiro(const string &ficheiro);
  void Tree(const string *fich = nullptr);
  void PesquisarAllDirectorias(list<string> &lres, const string &dir);
  void PesquisarAllFicheiros(list<string> &lres, const string &file);
  void RenomearFicheiros(const string &fich_old, const string &fich_new);
  bool FicheiroDuplicados();
  bool CopyBatch(const string &padrao, const string &DirOrigem,
                 const string &DirDestino);

protected:
private:
  unique_ptr<Diretoria> raiz;
  bool importacao_diretoria; // 1-diretoria, 0-xml

  void carregarConteudo(Diretoria *diretoria); // utilizada no Load
  int ContarFicheirosRec(Diretoria *dir);      // utilizada no ContarFicheiros
  int ContarDirectoriasRec(Diretoria *dir);    // utilizada no ContarDirectorias
  void ficheiroMaiorRec(Diretoria *dir, uintmax_t &tamMax,
                        string &strMax); // utilizada no FicheiroMaior
  int memoriaRec(Diretoria *dir);           // utilizada no Memoria
  string maiorDiretoriaRec(Diretoria *dir,
                           size_t &maior); // utilizada no DirectoriaMaisElementos
  string menorDiretoriaRec(
      Diretoria *dir, size_t &menor); // utilizada no DirectoriaMenosElementos
  void diretoriaMaisEspaco(Diretoria *dir, uintmax_t &tamMax,
                           string &strMax); // utilizada no DirectoriaMaisEspaco
  optional<string> pesquisarDiretoriaRec(
      Diretoria *dir, const string &s); // utilizada no Search
  optional<string> pesquisarFicheiroRec(
      Diretoria *dir, const string &s); // utilizada no Search
  void pesquisarItensComNomeIgualRec(
      Diretoria *dir, list<string> &lres, const string &n,
      bool procFich); // utilizada no RemoverAll, PesquisarAllFicheiros,
                      // PesquisarAllDirectorias
  bool RemovePorNome(Diretoria *dir, const string &s, const string &tipo,
                     bool fs,
                     bool operarNoDisco); // utilizada no RemoverAll
  bool removerPorCaminho(Diretoria *dir,
                         const string &caminho); // utilizada no RemoverAll
  void escreverXMLRec(Diretoria *dir, XML *XML); // utilizada no Escrever_XML
  Item *procurarItemRec(
      Diretoria *dir, const string &nomeProcurado,
      bool procurarDiretorias); // utilizada no MoveFicheiro e MoverDirectoria
  unique_ptr<Item> extrairItemPorNome(
      Diretoria *dir,
      const string &nome); // utilizada no MoveFicheiro e MoverDirectoria
  void setCaminhoRec(Diretoria *dir,
                     string &caminhoDir); // utilizada no MoverDirectoria
  optional<string> DataFicheiroRec(
      Diretoria *dir, const string &ficheiro); // utilizada no DataFicheiro
  void ShowRec(Diretoria *dir, size_t &nTabs, ostream &out); // utilizada no Tree
  int renomearFicheirosRec(
      Diretoria *dir, const string &fich_old, const string &fich_new,
      bool operarNoDisco); // utilizada no RenomearFicheiros
  bool VerificarDuplicadosRec(
      Diretoria *dir,
      unordered_set<string> &nomes); // utilizada no FicheiroDuplicados
  void AlterarNomeDuplicado(
      Diretoria *dir,
      unordered_map<string, int> &contador); // utilizada no CopyBatch
  void copiarItemRec(Diretoria *dirOrigem, Diretoria *destino,
                     const string &padrao, bool disco,
                     bool operarNoDisco); // utilizada no CopyBatch
};

#endif // SISTEMAFICHEIROS_H
