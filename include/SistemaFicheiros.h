#ifndef SISTEMAFICHEIROS_H
#define SISTEMAFICHEIROS_H

#include "Diretoria.h"
#include "IncludesGerais.h"

class XML; // declaração antecipada

class SistemaFicheiros {
public:
  enum class TipoItem { Ficheiro, Diretoria };
  enum class EstadoRenomeacao {
    Sucesso,
    SistemaNaoCarregado,
    NomeInvalido,
    SemAlteracoes,
    NaoEncontrado,
    Colisao,
    ErroSistemaFicheiros
  };

  struct ResultadoRenomeacao {
    EstadoRenomeacao estado;
    std::size_t quantidade;
  };

  SistemaFicheiros();
  virtual ~SistemaFicheiros() = default;

  SistemaFicheiros(const SistemaFicheiros &) = delete;
  SistemaFicheiros &operator=(const SistemaFicheiros &) = delete;
  SistemaFicheiros(SistemaFicheiros &&) noexcept = default;
  SistemaFicheiros &operator=(SistemaFicheiros &&) noexcept = default;

  bool Load(const std::string &path);
  std::size_t ContarFicheiros() const;
  std::size_t ContarDirectorias() const;
  std::uintmax_t Memoria() const;
  std::optional<std::string> DirectoriaMaisElementos() const;
  std::optional<std::string> DirectoriaMenosElementos() const;
  std::optional<std::string> FicheiroMaior() const;
  std::optional<std::string> DirectoriaMaisEspaco() const;
  std::optional<std::string> Search(const std::string &s,
                                    TipoItem tipo) const;
  bool RemoverAll(const std::string &s, TipoItem tipo);
  bool Escrever_XML(const std::string &s) const;
  bool Ler_XML(const std::string &s);
  bool MoveFicheiro(const std::string &Fich, const std::string &DirNova);
  bool MoverDirectoria(const std::string &DirOld,
                       const std::string &DirNew);
  std::optional<std::string> DataFicheiro(
      const std::string &ficheiro) const;
  std::string Tree() const;
  bool EscreverArvore(const std::string &ficheiro = "tree.txt") const;
  std::list<std::string> PesquisarAllDirectorias(
      const std::string &dir) const;
  std::list<std::string> PesquisarAllFicheiros(
      const std::string &file) const;
  ResultadoRenomeacao RenomearFicheiros(const std::string &fich_old,
                                        const std::string &fich_new);
  bool FicheiroDuplicados() const;
  bool CopyBatch(const std::string &padrao, const std::string &DirOrigem,
                 const std::string &DirDestino);

protected:
private:
  std::unique_ptr<Diretoria> raiz;
  bool importacao_diretoria; // 1-diretoria, 0-xml

  void carregarConteudo(
      Diretoria *diretoria,
      std::unordered_set<std::string> &diretoriasVisitadas); // utilizada no Load
  std::size_t ContarFicheirosRec(
      const Diretoria *dir) const; // utilizada no ContarFicheiros
  std::size_t ContarDirectoriasRec(
      const Diretoria *dir) const; // utilizada no ContarDirectorias
  void ficheiroMaiorRec(const Diretoria *dir, std::uintmax_t &tamMax,
                        std::string &strMax) const; // utilizada no FicheiroMaior
  std::uintmax_t memoriaRec(const Diretoria *dir) const; // utilizada no Memoria
  std::string maiorDiretoriaRec(
      const Diretoria *dir,
      std::size_t &maior) const; // utilizada no DirectoriaMaisElementos
  std::string menorDiretoriaRec(
      const Diretoria *dir,
      std::size_t &menor) const; // utilizada no DirectoriaMenosElementos
  void diretoriaMaisEspaco(
      const Diretoria *dir, std::uintmax_t &tamMax,
      std::string &strMax) const; // utilizada no DirectoriaMaisEspaco
  Item *resolverCaminho(const std::string &caminho);
  const Item *resolverCaminho(const std::string &caminho) const;
  void pesquisarItensComNomeIgualRec(
      const Diretoria *dir, std::list<std::string> &lres,
      const std::string &n, TipoItem tipo) const; // utilizada nas pesquisas
  bool RemovePorNome(Diretoria *dir, const std::string &s, TipoItem tipo,
                     bool removerNoDisco,
                     bool operarNoDisco); // utilizada no RemoverAll
  bool removerPorCaminho(Diretoria *dir,
                         const std::string &caminho); // utilizada no RemoverAll
  void escreverXMLRec(const Diretoria *dir,
                      XML *XML) const; // utilizada no Escrever_XML
  Item *procurarItemRec(
      Diretoria *dir, const std::string &nomeProcurado,
      TipoItem tipo); // utilizada no CopyBatch
  Diretoria *procurarDiretoriaPai(Diretoria *dir, const Item *item);
  bool contemDiretoria(Diretoria *origem, const Diretoria *procurada);
  void setCaminhoRec(Diretoria *dir,
                     std::string &caminhoDir); // utilizada no MoverDirectoria
  void ShowRec(const Diretoria *dir, std::size_t &nTabs,
               std::ostream &out) const; // utilizada no Tree
  bool VerificarDuplicadosRec(
      const Diretoria *dir,
      std::unordered_set<std::string>
          &nomes) const; // utilizada no FicheiroDuplicados
  void AlterarNomeDuplicado(
      Diretoria *dir,
      std::unordered_map<std::string, int>
          &contador); // utilizada no CopyBatch
  bool copiarItemRec(Diretoria *dirOrigem, Diretoria *destino,
                     const std::string &padrao, bool disco,
                     bool operarNoDisco); // utilizada no CopyBatch
};

#endif // SISTEMAFICHEIROS_H
