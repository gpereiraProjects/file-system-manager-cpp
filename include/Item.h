#ifndef ITEM_H
#define ITEM_H

#include "IncludesGerais.h"
class Item {
public:
  Item(const std::string &nomeInicial, const std::string &caminhoInicial);
  Item(const std::string &nomeInicial, const std::string &caminhoInicial,
       std::uintmax_t tamanhoInicial);
  virtual ~Item() = default;

  //============GET============
  const std::string &getNome() const;
  const std::string &getCaminho() const;
  std::uintmax_t getTamanho() const;
  virtual bool getIsFicheiro() const = 0;

  //============SET============
  void setCaminho(const std::string &novoCaminho);
  void setNome(const std::string &novoNome);

protected:
  std::string nome;
  std::string caminho;
  std::uintmax_t tamanho;
};

#endif // ITEM_H
