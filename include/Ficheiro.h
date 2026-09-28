#ifndef FICHEIRO_H
#define FICHEIRO_H

#include "IncludesGerais.h"
#include "Item.h"

class Ficheiro : public Item {
public:
  Ficheiro(const std::string &nome, const std::string &caminho);
  Ficheiro(const std::string &nome, const std::string &caminho,
           std::uintmax_t tamanho, const std::string &extensao,
           const std::string &dataModificacao);
  ~Ficheiro() override;

  const std::string &getExtensao() const;
  const std::string &getDataModificacao() const;
  bool getIsFicheiro() const override { return true; }

private:
  std::string extensao;
  std::string dataModificacao;
};

#endif // FICHEIRO_H
